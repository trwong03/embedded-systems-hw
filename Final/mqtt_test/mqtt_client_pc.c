/* mqtt_client_pc.c
 * Toy MQTT client for ENEE452 HW10 Problem A.
 * Runs on WSL/Linux alongside mqtt_broker.c to demonstrate
 * toy MQTT publish and subscribe operations on localhost.
 *
 * COMPILE: gcc -Wall mqtt_client_pc.c -o mqtt_client_pc
 * RUN:     ./mqtt_client_pc 127.0.0.1 1883
 *
 * WHAT IT DEMONSTRATES:
 *   1. CONNECT to broker
 *   2. SUBSCRIBE to topic "test/data"
 *   3. PUBLISH "Hello from myDACQ" to topic "test/data"
 *   4. RECEIVE the echo back from broker (proving pub/sub round-trip)
 *   5. DISCONNECT
 *
 * This models the same conversation the B-L475E board will have
 * with the broker in Phase 2, just running entirely on the PC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_SUBSCRIBE   0x80
#define MQTT_SUBACK      0x90
#define MQTT_DISCONNECT  0xE0

#define BUFSIZE 512

static void put_u16(uint8_t *buf, uint16_t val)
{
    buf[0] = (val >> 8) & 0xFF;
    buf[1] =  val       & 0xFF;
}

static int send_connect(int sockfd, const char *client_id)
{
    uint8_t pkt[64];
    uint16_t id_len  = (uint16_t)strlen(client_id);
    uint16_t rem_len = 10 + 2 + id_len;

    uint8_t var_hdr[10] = {
        0x00, 0x04, 'M','Q','T','T',
        0x04,   /* protocol level 4 = MQTT 3.1.1 */
        0x02,   /* clean session                  */
        0x00, 60 /* keepalive = 60s               */
    };

    pkt[0] = MQTT_CONNECT;
    pkt[1] = (uint8_t)rem_len;
    memcpy(&pkt[2], var_hdr, 10);
    put_u16(&pkt[12], id_len);
    memcpy(&pkt[14], client_id, id_len);

    write(sockfd, pkt, 2 + rem_len);

    /* read CONNACK */
    uint8_t connack[4] = {0};
    int n = read(sockfd, connack, 4);
    if (n < 4 || connack[0] != MQTT_CONNACK) {
        printf("ERROR: bad CONNACK\n");
        return -1;
    }
    if (connack[3] != 0x00) {
        printf("ERROR: broker rejected CONNECT (rc=%d)\n", connack[3]);
        return -1;
    }
    printf("[client] CONNECT -> CONNACK OK\n");
    return 0;
}

static int send_subscribe(int sockfd, const char *topic)
{
    uint8_t  pkt[64];
    uint16_t topic_len = (uint16_t)strlen(topic);
    uint16_t rem_len   = 2 + 2 + topic_len + 1;

    pkt[0] = MQTT_SUBSCRIBE | 0x02;
    pkt[1] = (uint8_t)rem_len;
    put_u16(&pkt[2], 1);           /* packet ID = 1  */
    put_u16(&pkt[4], topic_len);
    memcpy(&pkt[6], topic, topic_len);
    pkt[6 + topic_len] = 0x00;    /* QoS 0          */

    write(sockfd, pkt, 2 + rem_len);

    /* read SUBACK */
    uint8_t suback[5] = {0};
    read(sockfd, suback, 5);
    printf("[client] SUBSCRIBE '%s' -> SUBACK OK\n", topic);
    return 0;
}

static int send_publish(int sockfd, const char *topic,
                        const char *payload)
{
    uint8_t  pkt[256];
    uint16_t topic_len   = (uint16_t)strlen(topic);
    uint16_t payload_len = (uint16_t)strlen(payload);
    uint16_t rem_len     = 2 + topic_len + payload_len;

    pkt[0] = MQTT_PUBLISH;
    pkt[1] = (uint8_t)rem_len;
    put_u16(&pkt[2], topic_len);
    memcpy(&pkt[4], topic, topic_len);
    memcpy(&pkt[4 + topic_len], payload, payload_len);

    write(sockfd, pkt, 2 + rem_len);
    printf("[client] PUBLISH '%s' payload='%s'\n", topic, payload);
    return 0;
}

static int recv_publish(int sockfd)
{
    uint8_t hdr[2] = {0};
    int n = read(sockfd, hdr, 2);
    if (n < 2) { printf("[client] no packet received\n"); return -1; }

    uint8_t pkt_type = hdr[0] & 0xF0;
    uint8_t rem_len  = hdr[1];

    uint8_t body[BUFSIZE] = {0};
    if (rem_len > 0)
        read(sockfd, body, rem_len);

    if (pkt_type != MQTT_PUBLISH) {
        printf("[client] received non-PUBLISH packet type=0x%02X\n", pkt_type);
        return -1;
    }

    uint16_t topic_len = ((uint16_t)body[0] << 8) | body[1];
    char topic[128] = {0};
    memcpy(topic, &body[2], topic_len);

    uint16_t pay_offset = 2 + topic_len;
    uint16_t pay_len    = rem_len - pay_offset;
    char payload[BUFSIZE] = {0};
    memcpy(payload, &body[pay_offset], pay_len);

    printf("[client] RECEIVED PUBLISH topic='%s' payload='%s' (%d bytes)\n",
           topic, payload, pay_len);
    return 0;
}

static void send_disconnect(int sockfd)
{
    uint8_t disc[2] = { MQTT_DISCONNECT, 0x00 };
    write(sockfd, disc, 2);
    printf("[client] DISCONNECT sent\n");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <broker_ip> <port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 1883\n", argv[0]);
        exit(1);
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    printf("[client] connecting to broker at %s:%d\n", ip, port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port        = htons((uint16_t)port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    printf("[client] TCP connected\n");

    /* Step 1: CONNECT */
    if (send_connect(sockfd, "pc_test_client") != 0)
        exit(1);

    /* Step 2: SUBSCRIBE to test topic */
    send_subscribe(sockfd, "test/data");

    /* Step 3: PUBLISH a message */
    send_publish(sockfd, "test/data", "Hello from myDACQ PC client");

    /* Step 4: RECEIVE the echo back from broker */
    printf("[client] waiting for echo from broker...\n");
    recv_publish(sockfd);

    /* Step 5: DISCONNECT */
    send_disconnect(sockfd);
    close(sockfd);

    printf("[client] done.\n");
    return 0;
}
