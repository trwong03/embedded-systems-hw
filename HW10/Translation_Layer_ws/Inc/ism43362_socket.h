/**
 * ism43362_socket.h
 *
 * POSIX-style socket shim for the Inventek ISM43362-M3G-L44 Wi-Fi module
 * on the B-L475E-IOT01A, targeting the STM32CubeIDE
 * Wifi_Client_Server example project.
 *
 * Changes from the original version:
 *   - es_wifi.h / es_wifi_io.h includes removed; wifi.h is included instead.
 *   - ism_wifi_init() prototype comment updated to reflect that it now
 *     calls WIFI_Init() / WIFI_Connect() rather than the raw ES_WIFI layer.
 *   - The ISM_DEBUG_UART note is removed; printf retargeting is handled
 *     by the project's syscalls.c, not by this shim.
 *
 * Usage: in client_ism43362.c replace the Linux socket headers with:
 *   #include "ism43362_socket.h"
 */

#ifndef ISM43362_SOCKET_H
#define ISM43362_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------
 * Minimal POSIX compatibility layer
 *
 * Provides just enough types and constants so that client.c compiles
 * without any changes to its socket-related code.
 *
 * If you later add lwIP to the project, define ISM_NO_POSIX_COMPAT before
 * including this header to suppress these declarations and call the
 * ism_xxx() functions directly.
 * ---------------------------------------------------------------------- */
#ifndef ISM_NO_POSIX_COMPAT

#define AF_INET     2
#define SOCK_STREAM 1

struct in_addr {
    uint32_t s_addr;    /* network byte order */
};

struct sockaddr_in {
    uint16_t        sin_family;
    uint16_t        sin_port;   /* network byte order – use htons() */
    struct in_addr  sin_addr;
    uint8_t         sin_zero[8];
};

struct sockaddr {
    uint16_t sa_family;
    uint8_t  sa_data[14];
};

/* Byte-order helpers (Cortex-M4 is little-endian) */
static inline uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static inline uint16_t ntohs(uint16_t v) { return htons(v); }
static inline uint32_t htonl(uint32_t v) {
    return ((v & 0xFF000000u) >> 24) |
           ((v & 0x00FF0000u) >>  8) |
           ((v & 0x0000FF00u) <<  8) |
           ((v & 0x000000FFu) << 24);
}
static inline uint32_t ntohl(uint32_t v) { return htonl(v); }

/* inet_addr() – converts "a.b.c.d" string to network-order uint32_t */
uint32_t inet_addr(const char *cp);

/* Map standard POSIX names onto our implementation functions */
#define socket(af, type, proto)      ism_socket(af, type, proto)
#define connect(fd, addr, addrlen)   ism_connect(fd, addr, addrlen)
#define write(fd, buf, n)            ism_write(fd, buf, n)
#define read(fd, buf, n)             ism_read(fd, buf, n)
#define close(fd)                    ism_close(fd)

#endif /* ISM_NO_POSIX_COMPAT */

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/**
 * ism_wifi_init()
 *
 * Calls the Wifi_Client_Server project's WIFI_Init() and WIFI_Connect()
 * to bring up the ISM43362 and join the hotspot defined in
 * ism43362_config.h.  Must be called once before any socket function.
 *
 * Returns 0 on success, -1 on failure (see ism_last_error()).
 */
int ism_wifi_init(void);

/**
 * ism_socket()
 *
 * Allocates one of the ISM43362's 4 virtual TCP socket slots (0–3).
 * Only AF_INET + SOCK_STREAM is supported; protocol is ignored.
 *
 * Returns a non-negative fd on success, -1 if all slots are busy.
 */
int ism_socket(int domain, int type, int protocol);

/**
 * ism_connect()
 *
 * Opens a TCP connection via WIFI_OpenClientConnection().
 * Returns 0 on success, -1 on failure.
 */
int ism_connect(int sockfd, const struct sockaddr *addr, uint32_t addrlen);

/**
 * ism_write()
 *
 * Sends data over an open TCP socket via WIFI_SendData().
 * Automatically fragments payloads larger than 1460 bytes.
 *
 * fd == 1 or fd == 2 (stdout/stderr): passed straight through to the
 * C library's _write() / newlib retarget in syscalls.c – no special
 * handling needed here.
 *
 * Returns bytes sent on success, -1 on error.
 */
int ism_write(int fd, const void *buf, size_t count);

/**
 * ism_read()
 *
 * Receives data from an open TCP socket via WIFI_ReceiveData().
 * Polls until data arrives or ISM_RECV_TIMEOUT_MS elapses.
 *
 * Returns bytes received (>=1), 0 on connection close, -1 on error.
 */
int ism_read(int fd, void *buf, size_t count);

/**
 * ism_close()
 *
 * Closes the TCP socket via WIFI_CloseClientConnection() and frees
 * the slot for reuse.
 */
int ism_close(int fd);

/**
 * ism_last_error()
 *
 * Returns a human-readable string describing the most recent error.
 */
const char *ism_last_error(void);

#ifdef __cplusplus
}
#endif
#endif /* ISM43362_SOCKET_H */
