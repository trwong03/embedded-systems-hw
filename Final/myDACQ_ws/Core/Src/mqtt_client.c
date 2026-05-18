/* mqtt_client.c
 * Minimal MQTT client for myDACQ Phase 2.
 *
 * Uses the ISM43362 socket abstraction from HW10 Problem B
 * (ism43362_socket.h) to send/receive MQTT packets over WiFi TCP.
 *
 * MQTT PACKET CONSTRUCTION:
 *   All packets follow the MQTT 3.1.1 fixed header format:
 *     Byte 0: packet type | flags
 *     Byte 1: remaining length (single byte, max 127 for our use)
 *     Bytes 2+: variable header + payload
 *
 * Only QoS 0 PUBLISH is implemented - no retry, no ACK needed.
 * This matches the toy broker which also only handles QoS 0.
 */

#include "mqtt_client.h"
#include "ism43362_socket.h"   /* socket, connect, write, read, close     */
#include "ism43362_config.h"   /* ISM_WIFI_SSID, ISM_WIFI_PASSWORD        */

#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * MQTT packet type constants
 * ----------------------------------------------------------------------- */
#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_SUBSCRIBE   0x80
#define MQTT_SUBACK      0x90
#define MQTT_PINGREQ     0xC0
#define MQTT_DISCONNECT  0xE0

/* -------------------------------------------------------------------------
 * Internal state
 * ----------------------------------------------------------------------- */
static int s_sockfd    = -1;   /* TCP socket to broker                    */
static int s_connected = 0;    /* 1 after successful MQTT CONNECT         */

/* -------------------------------------------------------------------------
 * Internal: encode a 16-bit big-endian length into buf
 * ----------------------------------------------------------------------- */
static void put_u16(uint8_t *buf, uint16_t val)
{
    buf[0] = (val >> 8) & 0xFF;
    buf[1] =  val       & 0xFF;
}

/* -------------------------------------------------------------------------
 * Internal: build and send MQTT CONNECT packet
 *
 * CONNECT variable header:
 *   Protocol name: "MQTT" (4 bytes prefixed with length 0x00 0x04)
 *   Protocol level: 4 (MQTT 3.1.1)
 *   Connect flags: 0x02 (clean session)
 *   Keep alive: MQTT_KEEPALIVE (2 bytes big-endian)
 *
 * CONNECT payload:
 *   Client ID: length-prefixed string
 * ----------------------------------------------------------------------- */
static int send_connect(void)
{
    uint8_t  pkt[128];
    uint16_t client_id_len = (uint16_t)strlen(MQTT_CLIENT_ID);

    /* variable header: 10 bytes */
    uint8_t var_hdr[10] = {
        0x00, 0x04,             /* protocol name length = 4               */
        'M', 'Q', 'T', 'T',    /* protocol name                          */
        0x04,                   /* protocol level = 4 (MQTT 3.1.1)       */
        0x02,                   /* connect flags: clean session           */
        (MQTT_KEEPALIVE >> 8) & 0xFF,
        MQTT_KEEPALIVE & 0xFF
    };

    /* payload: 2-byte length prefix + client ID string */
    uint16_t rem_len = 10 + 2 + client_id_len;

    pkt[0] = MQTT_CONNECT;
    pkt[1] = (uint8_t)rem_len;   /* simplified: fits in 1 byte           */
    memcpy(&pkt[2], var_hdr, 10);
    put_u16(&pkt[12], client_id_len);
    memcpy(&pkt[14], MQTT_CLIENT_ID, client_id_len);

    int total = 2 + (int)rem_len;
    if (write(s_sockfd, pkt, total) != total)
        return MQTT_ERR_SEND;

    /* read CONNACK: should be 4 bytes: 0x20 0x02 0x00 0x00 */
    uint8_t connack[4] = {0};
    int n = read(s_sockfd, connack, 4);
    if (n < 4 || connack[0] != MQTT_CONNACK || connack[3] != 0x00)
        return MQTT_ERR_CONNECT;

    return MQTT_OK;
}

/* -------------------------------------------------------------------------
 * Internal: send MQTT SUBSCRIBE for the command topic
 * ----------------------------------------------------------------------- */
static int send_subscribe(void)
{
    uint8_t  pkt[64];
    uint16_t topic_len = (uint16_t)strlen(MYDACQ_TOPIC_CMD);

    /* variable header: packet ID = 1 */
    /* payload: topic length + topic + QoS 0 */
    uint16_t rem_len = 2 + 2 + topic_len + 1;

    pkt[0] = MQTT_SUBSCRIBE | 0x02;   /* flags: must be 0x02 per spec    */
    pkt[1] = (uint8_t)rem_len;
    put_u16(&pkt[2], 1);               /* packet ID = 1                  */
    put_u16(&pkt[4], topic_len);
    memcpy(&pkt[6], MYDACQ_TOPIC_CMD, topic_len);
    pkt[6 + topic_len] = 0x00;         /* requested QoS = 0              */

    int total = 2 + (int)rem_len;
    if (write(s_sockfd, pkt, total) != total)
        return MQTT_ERR_SEND;

    /* read SUBACK: 5 bytes */
    uint8_t suback[5] = {0};
    read(s_sockfd, suback, 5);
    /* we don't check it strictly for this toy implementation */

    return MQTT_OK;
}

/* -------------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

int mqtt_init(void)
{
    /* Step 1: ism_wifi_init */
    if (ism_wifi_init() != 0)
        return MQTT_ERR_WIFI;

    /* Step 2: socket */
    s_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sockfd < 0)
        return MQTT_ERR_SOCKET;

    /* Step 3: connect */
    struct sockaddr_in broker_addr;
    broker_addr.sin_family      = AF_INET;
    broker_addr.sin_addr.s_addr = inet_addr(MQTT_BROKER_IP);
    broker_addr.sin_port        = htons(MQTT_BROKER_PORT);

    if (connect(s_sockfd, (struct sockaddr *)&broker_addr,
                sizeof(broker_addr)) < 0) {
        ism_close(s_sockfd);
        s_sockfd = -1;
        return MQTT_ERR_SOCKET;
    }

    /* Step 4: MQTT CONNECT */
    if (send_connect() != MQTT_OK) {
        ism_close(s_sockfd);
        s_sockfd = -1;
        return MQTT_ERR_CONNECT;
    }

    /* Step 5: SUBSCRIBE */
    send_subscribe();

    s_connected = 1;
    return MQTT_OK;
}

int mqtt_publish(const char *topic,
                 const uint8_t *payload, uint16_t payload_len)
{
    if (!s_connected || s_sockfd < 0)
        return MQTT_ERR_CONNECT;

    uint16_t topic_len = (uint16_t)strlen(topic);
    uint16_t rem_len   = 2 + topic_len + payload_len;  /* no packet ID for QoS 0 */

    /* build packet */
    uint8_t pkt[32 + 128 + 512];   /* header + topic + payload           */
    pkt[0] = MQTT_PUBLISH;          /* QoS 0, no retain, no dup          */
    pkt[1] = (uint8_t)rem_len;      /* simplified single-byte length     */
    put_u16(&pkt[2], topic_len);
    memcpy(&pkt[4], topic, topic_len);
    memcpy(&pkt[4 + topic_len], payload, payload_len);

    int total = 2 + (int)rem_len;
    if (write(s_sockfd, pkt, total) != total)
        return MQTT_ERR_SEND;

    return MQTT_OK;
}

int mqtt_receive(char    *topic_buf,    uint16_t topic_buf_len,
                 uint8_t *payload_buf,  uint16_t payload_buf_len,
                 uint16_t *payload_len_out)
{
    if (!s_connected || s_sockfd < 0)
        return MQTT_ERR_CONNECT;

    /* try to read fixed header */
    uint8_t hdr[2] = {0};
    int n = read(s_sockfd, hdr, 2);
    if (n <= 0)
        return MQTT_ERR_RECV;   /* nothing available                      */

    uint8_t  pkt_type = hdr[0] & 0xF0;
    uint8_t  rem_len  = hdr[1];

    /* read remaining bytes */
    uint8_t body[512] = {0};
    if (rem_len > 0) {
        n = read(s_sockfd, body, rem_len);
        if (n <= 0) return MQTT_ERR_RECV;
    }

    if (pkt_type != MQTT_PUBLISH)
        return MQTT_ERR_RECV;   /* not a PUBLISH, ignore                  */

    /* parse topic */
    uint16_t topic_len = ((uint16_t)body[0] << 8) | body[1];
    if (topic_len >= topic_buf_len) topic_len = topic_buf_len - 1;
    memcpy(topic_buf, &body[2], topic_len);
    topic_buf[topic_len] = '\0';

    /* parse payload */
    uint16_t pay_offset = 2 + topic_len;
    uint16_t pay_len    = rem_len - pay_offset;
    if (pay_len > payload_buf_len) pay_len = payload_buf_len;
    memcpy(payload_buf, &body[pay_offset], pay_len);
    *payload_len_out = pay_len;

    return MQTT_OK;
}

void mqtt_disconnect(void)
{
    if (s_sockfd >= 0) {
        uint8_t disc[2] = { MQTT_DISCONNECT, 0x00 };
        write(s_sockfd, disc, 2);
        ism_close(s_sockfd);
        s_sockfd = -1;
    }
    s_connected = 0;
}
