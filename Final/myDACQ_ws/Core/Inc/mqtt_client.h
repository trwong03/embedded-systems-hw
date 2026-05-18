/* mqtt_client.h
 * Minimal MQTT client for myDACQ Phase 2.
 * Wraps the ISM43362 socket layer (ism43362_socket.h) to provide
 * MQTT CONNECT, PUBLISH, and receive operations.
 *
 * Only QoS 0 is supported - fire and forget, no acknowledgement.
 * This matches the toy broker's capabilities.
 *
 * TOPICS:
 *   MYDACQ_TOPIC_DATA    : board publishes sensor readings here
 *   MYDACQ_TOPIC_CMD     : board subscribes for commands here
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>
#include <stddef.h>

/* MQTT broker connection parameters - match your PC's hotspot IP */
#define MQTT_BROKER_IP    "192.168.137.1"  /* PC hotspot default gateway  */
#define MQTT_BROKER_PORT  1883             /* standard MQTT port          */
#define MQTT_CLIENT_ID    "myDACQ_board"
#define MQTT_KEEPALIVE    60               /* seconds                     */

/* Topics */
#define MYDACQ_TOPIC_DATA  "mydacq/data"   /* board -> broker -> PC       */
#define MYDACQ_TOPIC_CMD   "mydacq/cmd"    /* PC -> broker -> board       */

/* Return codes */
#define MQTT_OK           0
#define MQTT_ERR_WIFI    -1
#define MQTT_ERR_SOCKET  -2
#define MQTT_ERR_CONNECT -3
#define MQTT_ERR_SEND    -4
#define MQTT_ERR_RECV    -5

/*
 * mqtt_init()
 * Initialize WiFi and connect to broker.
 * Call once from a FreeRTOS task before any publish/receive.
 * Returns MQTT_OK on success.
 */
int mqtt_init(void);

/*
 * mqtt_publish(topic, payload, payload_len)
 * Publish a binary payload to a topic (QoS 0).
 * payload can be raw MessagePack bytes.
 * Returns MQTT_OK on success.
 */
int mqtt_publish(const char *topic,
                 const uint8_t *payload, uint16_t payload_len);

/*
 * mqtt_receive(topic_buf, topic_buf_len, payload_buf, payload_buf_len,
 *              payload_len_out)
 * Non-blocking check for an incoming PUBLISH packet.
 * Returns MQTT_OK if a packet was received, MQTT_ERR_RECV if none ready.
 * topic_buf    : filled with null-terminated topic string
 * payload_buf  : filled with raw payload bytes
 * payload_len_out : set to number of payload bytes received
 */
int mqtt_receive(char    *topic_buf,    uint16_t topic_buf_len,
                 uint8_t *payload_buf,  uint16_t payload_buf_len,
                 uint16_t *payload_len_out);

/*
 * mqtt_disconnect()
 * Send MQTT DISCONNECT and close socket.
 */
void mqtt_disconnect(void);

#endif /* MQTT_CLIENT_H */
