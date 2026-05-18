#ifndef ISM43362_CONFIG_H
#define ISM43362_CONFIG_H

/* ------------------------------------------------------------------
 * Wi-Fi credentials – must match your PC hotspot settings
 * ------------------------------------------------------------------ */
#define ISM_WIFI_SSID       "rogflow"       /* hotspot SSID          */
#define ISM_WIFI_PASSWORD   "password"      /* WPA2 passphrase       */

#define ISM_SERVER_IP       "192.168.137.1"
#define ISM_SERVER_PORT     8182

#define ISM_CONNECT_TIMEOUT_MS   10000
#define ISM_SEND_TIMEOUT_MS       5000
#define ISM_RECV_TIMEOUT_MS       5000
#define ISM_RECV_POLL_DELAY_MS      50

#endif
