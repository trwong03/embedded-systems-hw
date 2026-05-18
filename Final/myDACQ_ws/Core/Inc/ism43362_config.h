#ifndef ISM43362_CONFIG_H
#define ISM43362_CONFIG_H

/* WiFi hotspot credentials */
#define ISM_WIFI_SSID           "rogflow"
#define ISM_WIFI_PASSWORD       "rogflowww"

/* MQTT broker - PC hotspot IP */
#define ISM_SERVER_IP           "192.168.137.1"
#define ISM_SERVER_PORT         1883

#define ISM_SEND_TIMEOUT_MS     5000
#define ISM_RECV_TIMEOUT_MS     200
#define ISM_RECV_POLL_DELAY_MS  100

#endif /* ISM43362_CONFIG_H */
