/**
 * ism43362_config.h
 *
 * Board / network configuration for the ISM43362 socket shim.
 * Edit the values below before building.
 *
 * Changes from the original version:
 *   - Removed ISM_WIFI_SECURITY / ES_WIFI_SEC_xxx.
 *     The Wifi_Client_Server project's wifi.h layer uses the
 *     WIFI_ECN_WPA2_PSK constant directly; see ism43362_socket.c.
 *   - Removed ISM_DEBUG_UART.
 *     The project's syscalls.c already retargets printf to UART;
 *     the shim no longer needs to do it manually.
 */

#ifndef ISM43362_CONFIG_H
#define ISM43362_CONFIG_H

/* ------------------------------------------------------------------
 * Wi-Fi credentials – must match your PC hotspot settings
 * ------------------------------------------------------------------ */
#define ISM_WIFI_SSID       "MyHotspot"       /* hotspot SSID          */
#define ISM_WIFI_PASSWORD   "mypassword"      /* WPA2 passphrase       */

/* ------------------------------------------------------------------
 * nweb server address
 * The default Windows hotspot gateway is 192.168.137.1
 * Confirm with: ipconfig  (look for "Mobile Hotspot adapter")
 * ------------------------------------------------------------------ */
#define ISM_SERVER_IP       "192.168.137.1"
#define ISM_SERVER_PORT     8182

/* ------------------------------------------------------------------
 * Timing (milliseconds) – tune if you see spurious timeouts
 * ------------------------------------------------------------------ */
#define ISM_CONNECT_TIMEOUT_MS   10000
#define ISM_SEND_TIMEOUT_MS       5000
#define ISM_RECV_TIMEOUT_MS       5000
#define ISM_RECV_POLL_DELAY_MS      50

#endif /* ISM43362_CONFIG_H */
