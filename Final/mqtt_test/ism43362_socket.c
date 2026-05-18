#include "ism43362_socket.h"
#include "ism43362_config.h"

#include "wifi.h"
#include "stm32l4xx_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t  s_slot_used   = 0x00;   /* bitmask: bits 0-3 = sockets 0-3 */
static char     s_last_error[128] = "none";
static int      s_initialized = 0;

/* The ISM43362 firmware accepts/returns at most 1460 bytes per transaction */
#define ISM_MAX_XFER_SIZE  1460

static void set_error(const char *msg)
{
    strncpy(s_last_error, msg, sizeof(s_last_error) - 1);
    s_last_error[sizeof(s_last_error) - 1] = '\0';
}

static int alloc_slot(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (!(s_slot_used & (1u << i))) {
            s_slot_used |= (1u << i);
            return i;
        }
    }
    return -1;
}

static void free_slot(int slot)
{
    if (slot >= 0 && slot < 4)
        s_slot_used &= ~(1u << slot);
}

uint32_t inet_addr(const char *cp)
{
    uint32_t result = 0;
    int      shift  = 0;
    uint8_t  octet  = 0;
    char     c;

    if (!cp) return 0xFFFFFFFFu;

    while ((c = *cp++) != '\0') {
        if (c >= '0' && c <= '9') {
            octet = (uint8_t)(octet * 10 + (c - '0'));
        } else if (c == '.') {
            result |= ((uint32_t)octet << shift);
            shift  += 8;
            octet   = 0;
        } else {
            return 0xFFFFFFFFu;
        }
    }
    result |= ((uint32_t)octet << shift);
    return result;
}

/* -------------------------------------------------------------------------
 * ism_wifi_init()
 *
 * WIFI_Init()    – resets the ISM43362, negotiates the SPI handshake,
 *                  reads the module firmware version.
 * WIFI_Connect() – sends the WPA2 join sequence; blocks until associated
 *                  or the module reports failure (~10 s worst case).
 * ---------------------------------------------------------------------- */
int ism_wifi_init(void)
{
    if (s_initialized) return 0;

    printf("Calling WIFI_Init...\r\n");
    if (WIFI_Init() != WIFI_STATUS_OK) {
        set_error("WIFI_Init failed");
        return -1;
    }
    printf("WIFI_Init OK\r\n");

    if (WIFI_Connect(ISM_WIFI_SSID, ISM_WIFI_PASSWORD, WIFI_ECN_WPA2_PSK) != WIFI_STATUS_OK) {
        set_error("WIFI_Connect failed");
        return -1;
    }
    printf("Connected\r\n");

    uint8_t ip[4];
    if (WIFI_GetIP_Address(ip, 4) == WIFI_STATUS_OK)
        printf("IP: %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);

    s_initialized = 1;
    return 0;
}

int ism_socket(int domain, int type, int protocol)
{
    (void)protocol;

    if (!s_initialized) {
        set_error("ism_wifi_init() not called");
        return -1;
    }
    if (domain != AF_INET || type != SOCK_STREAM) {
        set_error("Only AF_INET + SOCK_STREAM (TCP) supported");
        return -1;
    }

    int slot = alloc_slot();
    if (slot < 0) {
        set_error("No free socket slots (max 4 concurrent)");
        return -1;
    }
    return slot;
}

/* -------------------------------------------------------------------------
 * ism_connect()
 *
 * WIFI_OpenClientConnection() parameters:
 *   socket   – 0-3
 *   type     – WIFI_TCP_PROTOCOL
 *   name     – human label, unused by firmware, pass ""
 *   ip_addr  – dotted-decimal string
 *   port     – uint16_t
 *   local_port – 0 (auto-assign)
 * ---------------------------------------------------------------------- */
int ism_connect(int sockfd, const struct sockaddr *addr, uint32_t addrlen)
{
    (void)addrlen;

    if (!s_initialized) { set_error("not initialized"); return -1; }
    if (sockfd < 0 || sockfd > 3) { set_error("bad fd"); return -1; }
    if (!addr) { set_error("null addr"); return -1; }

    const struct sockaddr_in *sin = (const struct sockaddr_in *)addr;

    uint32_t ip_h = ntohl(sin->sin_addr.s_addr);
    uint16_t port = ntohs(sin->sin_port);

    uint8_t ip_bytes[4];
    ip_bytes[0] = (ip_h >> 24) & 0xFF;
    ip_bytes[1] = (ip_h >> 16) & 0xFF;
    ip_bytes[2] = (ip_h >>  8) & 0xFF;
    ip_bytes[3] =  ip_h        & 0xFF;

    WIFI_Status_t rc = WIFI_OpenClientConnection(
            (uint32_t)sockfd,
            WIFI_TCP_PROTOCOL,
            "",
            ip_bytes,
            port,
            0);

    if (rc != WIFI_STATUS_OK) {
        char buf[80];
        snprintf(buf, sizeof(buf),
                 "WIFI_OpenClientConnection failed (rc=%d) -> %u.%u.%u.%u:%u",
                 (int)rc,
                 ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3],
                 (unsigned)port);
        set_error(buf);
        free_slot(sockfd);
        return -1;
    }
    return 0;
}

int ism_write(int fd, const void *buf, size_t count)
{
    if (!s_initialized) { set_error("not initialized"); return -1; }
    if (fd < 0 || fd > 3) { set_error("bad fd"); return -1; }
    if (!buf || count == 0) return 0;

    const uint8_t *ptr        = (const uint8_t *)buf;
    size_t         remaining  = count;
    size_t         total_sent = 0;

    while (remaining > 0) {
        uint16_t chunk = (uint16_t)(remaining > ISM_MAX_XFER_SIZE
                                    ? ISM_MAX_XFER_SIZE : remaining);
        uint16_t sent_len = 0;

        WIFI_Status_t rc = WIFI_SendData(
                (uint8_t)fd,
                (uint8_t *)ptr,
                chunk,
                &sent_len,
                ISM_SEND_TIMEOUT_MS);

        if (rc != WIFI_STATUS_OK) {
            set_error("WIFI_SendData failed");
            return (total_sent > 0) ? (int)total_sent : -1;
        }

        if (sent_len == 0) {
            set_error("WIFI_SendData: zero bytes accepted by module");
            break;
        }

        ptr        += sent_len;
        remaining  -= sent_len;
        total_sent += sent_len;
    }
    return (int)total_sent;
}

int ism_read(int fd, void *buf, size_t count)
{
    if (!s_initialized) { set_error("not initialized"); return -1; }
    if (fd < 0 || fd > 3) { set_error("bad fd"); return -1; }
    if (!buf || count == 0) return 0;

    uint16_t req_len  = (uint16_t)(count > ISM_MAX_XFER_SIZE
                                   ? ISM_MAX_XFER_SIZE : count);
    uint16_t recv_len = 0;

    uint32_t deadline = HAL_GetTick() + ISM_RECV_TIMEOUT_MS;

    while (1) {
        WIFI_Status_t rc = WIFI_ReceiveData(
                (uint8_t)fd,
                (uint8_t *)buf,
                req_len,
                &recv_len,
                ISM_RECV_POLL_DELAY_MS);

        if (rc == WIFI_STATUS_OK && recv_len > 0)
            return (int)recv_len;

        if (rc != WIFI_STATUS_OK)
            return 0;   /* remote end closed connection → EOF */

        if (HAL_GetTick() >= deadline) {
            set_error("ism_read: receive timeout");
            return -1;
        }

        HAL_Delay(ISM_RECV_POLL_DELAY_MS);
    }
}

int ism_close(int fd)
{
    if (fd < 0 || fd > 3) { set_error("bad fd"); return -1; }
    if (!s_initialized) return -1;

    WIFI_Status_t rc = WIFI_CloseClientConnection((uint32_t)fd);
    free_slot(fd);

    if (rc != WIFI_STATUS_OK) {
        set_error("WIFI_CloseClientConnection failed");
        return -1;
    }
    return 0;
}

const char *ism_last_error(void)
{
    return s_last_error;
}
