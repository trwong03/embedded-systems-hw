/* mydacq_display.c
 * PC-side display for myDACQ Phase 2.
 * Connects to the toy broker, subscribes to mydacq/data,
 * and decodes the MessagePack sensor readings from the board.
 *
 * COMPILE (WSL):
 *   gcc -Wall mydacq_display.c -o mydacq_display
 *
 * RUN (while broker is running):
 *   ./mydacq_display 127.0.0.1 1883
 *
 * MessagePack format from board:
 *   fixmap(2) { "addr": uint, "val": uint }
 *   82 A4 61 64 64 72 XX A3 76 61 6C YY
 *   where XX = address (usually 00), YY = sensor byte value
 *
 * Also listens on stdin for commands to send back to the board:
 *   s  - toggle sampling
 *   r  - report config
 *   +  - slower
 *   -  - faster
 *   q  - quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <time.h>

#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_SUBSCRIBE   0x80
#define MQTT_SUBACK      0x90
#define MQTT_DISCONNECT  0xE0

#define TOPIC_DATA  "mydacq/data"
#define TOPIC_CMD   "mydacq/cmd"
#define CLIENT_ID   "mydacq_display"

/* -------------------------------------------------------------------------
 * Minimal MessagePack decoder for our specific format:
 *   fixmap(2) { fixstr("addr"): uint, fixstr("val"): uint }
 * Returns 1 on success, 0 on parse failure.
 * ----------------------------------------------------------------------- */
static int msgpack_decode(const uint8_t *buf, int len,
                          uint16_t *addr_out, uint8_t *val_out)
{
    if (len < 4) return 0;

    /* byte 0: 0x82 = fixmap with 2 entries */
    if (buf[0] != 0x82) return 0;

    int i = 1;

    /* parse two key-value pairs */
    uint16_t addr = 0;
    uint8_t  val  = 0;
    int got_addr  = 0;
    int got_val   = 0;

    for (int pair = 0; pair < 2; pair++) {
        if (i >= len) return 0;

        /* key: fixstr */
        if ((buf[i] & 0xE0) != 0xA0) return 0;
        int klen = buf[i] & 0x1F;
        i++;
        if (i + klen > len) return 0;

        char key[16] = {0};
        memcpy(key, &buf[i], klen < 15 ? klen : 15);
        i += klen;

        /* value: positive fixint or uint8 or uint16 */
        if (i >= len) return 0;
        uint32_t uval = 0;
        if (buf[i] <= 0x7F) {          /* positive fixint */
            uval = buf[i++];
        } else if (buf[i] == 0xCC) {   /* uint8 */
            i++;
            if (i >= len) return 0;
            uval = buf[i++];
        } else if (buf[i] == 0xCD) {   /* uint16 */
            i++;
            if (i + 1 >= len) return 0;
            uval = ((uint16_t)buf[i] << 8) | buf[i+1];
            i += 2;
        } else if (buf[i] == 0xCE) {   /* uint32 */
            i++;
            if (i + 3 >= len) return 0;
            uval = ((uint32_t)buf[i]   << 24) | ((uint32_t)buf[i+1] << 16) |
                   ((uint32_t)buf[i+2] <<  8) |  (uint32_t)buf[i+3];
            i += 4;
        } else {
            return 0;
        }

        if (strcmp(key, "addr") == 0) { addr = (uint16_t)uval; got_addr = 1; }
        if (strcmp(key, "val")  == 0) { val  = (uint8_t)uval;  got_val  = 1; }
    }

    if (!got_addr || !got_val) return 0;
    *addr_out = addr;
    *val_out  = val;
    return 1;
}

/* -------------------------------------------------------------------------
 * MQTT helpers
 * ----------------------------------------------------------------------- */
static void put_u16(uint8_t *buf, uint16_t val)
{
    buf[0] = (val >> 8) & 0xFF;
    buf[1] =  val       & 0xFF;
}

static int do_connect(int fd, const char *client_id)
{
    uint8_t pkt[64];
    uint16_t id_len  = (uint16_t)strlen(client_id);
    uint16_t rem_len = 10 + 2 + id_len;
    uint8_t var_hdr[10] = {
        0x00,0x04,'M','Q','T','T', 0x04, 0x02, 0x00, 60
    };
    pkt[0] = MQTT_CONNECT;
    pkt[1] = (uint8_t)rem_len;
    memcpy(&pkt[2], var_hdr, 10);
    put_u16(&pkt[12], id_len);
    memcpy(&pkt[14], client_id, id_len);
    write(fd, pkt, 2 + rem_len);

    uint8_t connack[4] = {0};
    int n = read(fd, connack, 4);
    if (n < 4 || connack[0] != MQTT_CONNACK || connack[3] != 0x00) {
        fprintf(stderr, "[display] CONNACK failed\n");
        return -1;
    }
    printf("[display] connected to broker\n");
    return 0;
}

static int do_subscribe(int fd, const char *topic)
{
    uint8_t pkt[64];
    uint16_t topic_len = (uint16_t)strlen(topic);
    uint16_t rem_len   = 2 + 2 + topic_len + 1;
    pkt[0] = MQTT_SUBSCRIBE | 0x02;
    pkt[1] = (uint8_t)rem_len;
    put_u16(&pkt[2], 1);
    put_u16(&pkt[4], topic_len);
    memcpy(&pkt[6], topic, topic_len);
    pkt[6 + topic_len] = 0x00;
    write(fd, pkt, 2 + rem_len);

    uint8_t suback[5] = {0};
    read(fd, suback, 5);
    printf("[display] subscribed to '%s'\n", topic);
    return 0;
}

static int do_publish(int fd, const char *topic,
                      const uint8_t *payload, uint16_t payload_len)
{
    uint8_t pkt[256];
    uint16_t topic_len = (uint16_t)strlen(topic);
    uint16_t rem_len   = 2 + topic_len + payload_len;
    pkt[0] = MQTT_PUBLISH;
    pkt[1] = (uint8_t)rem_len;
    put_u16(&pkt[2], topic_len);
    memcpy(&pkt[4], topic, topic_len);
    memcpy(&pkt[4 + topic_len], payload, payload_len);
    write(fd, pkt, 2 + rem_len);
    return 0;
}

/* -------------------------------------------------------------------------
 * Handle one incoming PUBLISH from broker
 * ----------------------------------------------------------------------- */
static void handle_incoming(int fd)
{
    uint8_t hdr[2] = {0};
    int n = read(fd, hdr, 2);
    if (n < 2) return;

    uint8_t pkt_type = hdr[0] & 0xF0;
    uint8_t rem_len  = hdr[1];

    uint8_t body[512] = {0};
    if (rem_len > 0)
        read(fd, body, rem_len);

    if (pkt_type != MQTT_PUBLISH) return;

    /* parse topic */
    uint16_t topic_len = ((uint16_t)body[0] << 8) | body[1];
    char topic[128] = {0};
    if (topic_len > 127) topic_len = 127;
    memcpy(topic, &body[2], topic_len);

    uint16_t pay_offset = 2 + topic_len;
    uint16_t pay_len    = rem_len - pay_offset;
    uint8_t *payload    = &body[pay_offset];

    /* timestamp */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    if (strcmp(topic, TOPIC_DATA) == 0) {
        /* decode MessagePack */
        uint16_t addr = 0;
        uint8_t  val  = 0;
        if (msgpack_decode(payload, pay_len, &addr, &val)) {
            printf("[%02d:%02d:%02d] M24SR addr=0x%04X  val=0x%02X (%3d)  "
                   "pack(%d bytes): ",
                   t->tm_hour, t->tm_min, t->tm_sec,
                   addr, val, val, pay_len);
            for (int i = 0; i < pay_len; i++)
                printf("%02X", payload[i]);
            printf("\n");
            fflush(stdout);
        } else {
            printf("[%02d:%02d:%02d] raw payload (%d bytes): ",
                   t->tm_hour, t->tm_min, t->tm_sec, pay_len);
            for (int i = 0; i < pay_len; i++)
                printf("%02X", payload[i]);
            printf("\n");
        }
    } else {
        printf("[display] received on '%s' (%d bytes)\n", topic, pay_len);
    }
}

/* -------------------------------------------------------------------------
 * Send a command string to the board via mydacq/cmd topic
 * ----------------------------------------------------------------------- */
static void send_command(int fd, const char *cmd)
{
    do_publish(fd, TOPIC_CMD, (const uint8_t *)cmd, (uint16_t)strlen(cmd));
    printf("[display] sent command '%s' to board\n", cmd);
}

/* -------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <broker_ip> <port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 1883\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in serv = {0};
    serv.sin_family      = AF_INET;
    serv.sin_addr.s_addr = inet_addr(argv[1]);
    serv.sin_port        = htons((uint16_t)atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("connect"); exit(1);
    }

    if (do_connect(sockfd, CLIENT_ID) < 0) exit(1);
    do_subscribe(sockfd, TOPIC_DATA);

    printf("\n=== myDACQ Display ===\n");
    printf("Commands (type then Enter):\n");
    printf("  s = toggle sampling\n");
    printf("  r = report config\n");
    printf("  + = slower\n");
    printf("  - = faster\n");
    printf("  q = quit\n\n");

    /* main loop: select() on broker socket + stdin */
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
        int rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);

        if (rc < 0) { perror("select"); break; }

        /* data from broker */
        if (FD_ISSET(sockfd, &rfds))
            handle_incoming(sockfd);

        /* command from stdin */
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char line[32] = {0};
            if (fgets(line, sizeof(line), stdin) == NULL) break;
            char cmd = line[0];
            if (cmd == 'q') {
                printf("[display] quitting\n");
                break;
            }
            if (cmd == 's' || cmd == 'r' || cmd == '+' || cmd == '-') {
                char msg[4] = {cmd, '\r', '\n', 0};
                send_command(sockfd, msg);
            } else {
                printf("[display] unknown command '%c' (s r + - q)\n", cmd);
            }
        }
    }

    uint8_t disc[2] = { MQTT_DISCONNECT, 0x00 };
    write(sockfd, disc, 2);
    close(sockfd);
    return 0;
}
