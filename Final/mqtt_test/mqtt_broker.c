/* mqtt_broker.c
 * Toy MQTT broker for ENEE452 myDACQ Phase 2.
 *
 * Based on nweb.c socket pattern, adapted for MQTT protocol.
 *
 * COMPILE (WSL):
 *   gcc -Wall -pthread mqtt_broker.c -o mqtt_broker
 *
 * RUN:
 *   ./mqtt_broker 1883
 *
 * WHAT IT DOES:
 *   - Listens on TCP port 1883 (standard MQTT port)
 *   - Accepts CONNECT from clients, responds with CONNACK
 *   - Accepts PUBLISH from any client, echoes the same
 *     message back to ALL connected clients on the same topic
 *   - Accepts DISCONNECT gracefully
 *
 * LIMITATIONS (intentional for this toy):
 *   - No QoS 1/2, no persistence, no will, no retain
 *   - Max 8 simultaneous clients
 *   - Topics up to 128 bytes, payloads up to 512 bytes
 *   - Single-threaded with select() for multiplexing
 *
 * MQTT PACKET FORMAT (fixed header):
 *   Byte 0: packet type (upper 4 bits) + flags (lower 4 bits)
 *   Byte 1+: remaining length (variable encoding, we only handle 1 byte)
 *   Then: variable header + payload
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* -------------------------------------------------------------------------
 * MQTT packet type constants (upper nibble of first byte)
 * ----------------------------------------------------------------------- */
#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_PUBACK      0x40
#define MQTT_SUBSCRIBE   0x80
#define MQTT_SUBACK      0x90
#define MQTT_PINGREQ     0xC0
#define MQTT_PINGRESP    0xD0
#define MQTT_DISCONNECT  0xE0

/* -------------------------------------------------------------------------
 * Broker configuration
 * ----------------------------------------------------------------------- */
#define MAX_CLIENTS      8
#define MAX_TOPIC_LEN    128
#define MAX_PAYLOAD_LEN  512
#define BUFSIZE          1024

/* -------------------------------------------------------------------------
 * Connected client table
 * ----------------------------------------------------------------------- */
static int clients[MAX_CLIENTS];
static int num_clients = 0;

static void add_client(int fd)
{
    if (num_clients < MAX_CLIENTS)
        clients[num_clients++] = fd;
}

static void remove_client(int fd)
{
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == fd) {
            clients[i] = clients[--num_clients];
            return;
        }
    }
}

/* -------------------------------------------------------------------------
 * MQTT helpers
 * ----------------------------------------------------------------------- */

/* Send CONNACK: accepted, no session present */
static void send_connack(int fd)
{
    uint8_t connack[4] = {
        MQTT_CONNACK,   /* packet type                  */
        0x02,           /* remaining length = 2         */
        0x00,           /* session present = 0          */
        0x00            /* return code = 0 (accepted)   */
    };
    write(fd, connack, 4);
    printf("[broker] sent CONNACK to fd=%d\n", fd);
}

/* Send PINGRESP */
static void send_pingresp(int fd)
{
    uint8_t pingresp[2] = { MQTT_PINGRESP, 0x00 };
    write(fd, pingresp, 2);
}

/* Send SUBACK for a single topic, QoS 0 */
static void send_suback(int fd, uint16_t packet_id)
{
    uint8_t suback[5] = {
        MQTT_SUBACK,
        0x03,                           /* remaining length             */
        (packet_id >> 8) & 0xFF,        /* packet ID MSB                */
        packet_id & 0xFF,               /* packet ID LSB                */
        0x00                            /* granted QoS = 0              */
    };
    write(fd, suback, 5);
}

/*
 * Echo a PUBLISH packet to all connected clients except the sender.
 * The spec says the PC broker "immediately echoes them back" —
 * we echo to ALL clients so both outgoing and incoming paths work.
 */
static void echo_publish(int sender_fd, uint8_t *pkt, int pkt_len)
{
    /* parse topic for logging */
    if (pkt_len < 4) return;
    uint16_t topic_len = ((uint16_t)pkt[2] << 8) | pkt[3];
    char topic[MAX_TOPIC_LEN + 1] = {0};
    if (topic_len > MAX_TOPIC_LEN) topic_len = MAX_TOPIC_LEN;
    memcpy(topic, &pkt[4], topic_len);

    int payload_offset = 4 + topic_len;
    int payload_len    = pkt_len - payload_offset;

    printf("[broker] PUBLISH from fd=%d topic='%s' payload_len=%d\n",
           sender_fd, topic, payload_len);

    /* Echo to all clients EXCEPT the sender.
     * The board publishing on mydacq/data should not receive its own
     * data back — that would fill its receive buffer and cause a reset. */
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] != sender_fd) {
            write(clients[i], pkt, pkt_len);
            printf("[broker]   echoed to fd=%d\n", clients[i]);
        }
    }
}

/*
 * Read and decode one MQTT packet from fd.
 * Returns 0 on success, -1 on disconnect/error.
 */
static int handle_packet(int fd)
{
    uint8_t buf[BUFSIZE];

    /* read fixed header: type byte + remaining length byte */
    int n = read(fd, buf, 2);
    if (n <= 0) return -1;  /* disconnect */

    uint8_t  pkt_type   = buf[0] & 0xF0;
    uint8_t  pkt_flags  = buf[0] & 0x0F;
    uint8_t  rem_len    = buf[1];  /* simplified: single-byte length only */

    /* read remaining bytes */
    int total = 2;
    if (rem_len > 0) {
        n = read(fd, &buf[2], rem_len);
        if (n <= 0) return -1;
        total += n;
    }

    printf("[broker] pkt type=0x%02X flags=0x%02X rem_len=%d from fd=%d\n",
           pkt_type, pkt_flags, rem_len, fd);

    switch (pkt_type) {

    case MQTT_CONNECT:
        /* client greeting - just accept it */
        printf("[broker] CONNECT from fd=%d\n", fd);
        send_connack(fd);
        break;

    case MQTT_PUBLISH:
        echo_publish(fd, buf, total);
        break;

    case MQTT_SUBSCRIBE:
        /* parse packet ID and send SUBACK */
        if (total >= 4) {
            uint16_t pid = ((uint16_t)buf[2] << 8) | buf[3];
            send_suback(fd, pid);
            printf("[broker] SUBSCRIBE from fd=%d pid=%d\n", fd, pid);
        }
        break;

    case MQTT_PINGREQ:
        send_pingresp(fd);
        break;

    case MQTT_DISCONNECT:
        printf("[broker] DISCONNECT from fd=%d\n", fd);
        return -1;

    default:
        printf("[broker] unknown packet type 0x%02X from fd=%d\n",
               pkt_type, fd);
        break;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Main: listen loop using select() for multiple clients
 * ----------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: %s 1883\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("ERROR: invalid port %s\n", argv[1]);
        exit(1);
    }

    /* create listening socket */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }

    /* allow immediate reuse of port after restart */
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons((uint16_t)port);

    if (bind(listenfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0) { perror("bind"); exit(1); }

    if (listen(listenfd, 8) < 0) { perror("listen"); exit(1); }

    printf("[broker] toy MQTT broker listening on port %d\n", port);
    printf("[broker] waiting for connections...\n");

    /* main select() loop */
    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        int maxfd = listenfd;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i], &readfds);
            if (clients[i] > maxfd) maxfd = clients[i];
        }

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) { perror("select"); continue; }

        /* new connection? */
        if (FD_ISSET(listenfd, &readfds)) {
            struct sockaddr_in cli_addr = {0};
            socklen_t cli_len = sizeof(cli_addr);
            int newfd = accept(listenfd,
                               (struct sockaddr *)&cli_addr, &cli_len);
            if (newfd >= 0) {
                printf("[broker] new connection fd=%d from %s\n",
                       newfd, inet_ntoa(cli_addr.sin_addr));
                add_client(newfd);
            }
        }

        /* data from existing clients? */
        for (int i = 0; i < num_clients; i++) {
            if (FD_ISSET(clients[i], &readfds)) {
                int fd = clients[i];
                if (handle_packet(fd) < 0) {
                    printf("[broker] client fd=%d disconnected\n", fd);
                    close(fd);
                    remove_client(fd);
                    i--;  /* adjust index after removal */
                }
            }
        }
    }

    return 0;
}
