#ifndef MYDACQ_MSG_H
#define MYDACQ_MSG_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stddef.h>
#include <stdint.h>
#include "queue.h"

#define MQTT_PAYLOAD_MAX  32

typedef struct {
    uint8_t  data[MQTT_PAYLOAD_MAX];
    uint16_t len;
} MqttMsg_type;

extern QueueHandle_t g_mqttTxQueue;  /* deviceTask  -> mqttTask */
extern QueueHandle_t g_mqttRxQueue;  /* mqttTask -> commandTask */

/* -------------------------------------------------------------------------
 * Re-export the core types from consolemsg05.c so the rest of the project
 * only needs to include this one header.
 * ----------------------------------------------------------------------- */

typedef struct listLink {
    struct listLink *next;  /* next link in list; points to itself if tail  */
    char            *addr;  /* pointer to message buffer                    */
    uint32_t         count; /* length of message in bytes                   */
    uint32_t         info;  /* reserved, kept for MoT compatibility         */
} msgLink_type;

typedef struct listAnchor {
    msgLink_type *listheadp;  /* head of linked list                        */
    msgLink_type *listtailp;  /* tail of linked list                        */
    char         *byteptr;    /* current send position within head buffer   */
    uint32_t      bytecount;  /* bytes remaining to send from head buffer   */
} msgList_type;

/* Return codes from msgPost() - matches consolemsg05.c */
typedef enum {
    LINK_IS_BUSY        = -3,
    LINK_SNPRINTF_FAILED = -2,
    LINK_MSG_TOO_LONG   = -1,
    LINK_IS_INSTALLED   =  0
} CONSOLEMSG_returncodes;

/* Return codes from msgSend() - matches consolemsg05.c */
typedef enum {
    UART_TX_FAILED         = -1,
    UART_TX_OK             =  0,
    UART_TX_IS_BUSY        =  1,
    UART_LIST_WAS_UPDATED  =  2,
    UART_LIST_IS_EMPTY     =  3,
    UART_LIST_DEFAULT_RETURN = 4
} MSGSEND_RETURNCODES;

/* -------------------------------------------------------------------------
 * The single shared console message list and its mutex.
 * Declared here, defined in mydacq_msg.c.
 * ----------------------------------------------------------------------- */
extern msgList_type  g_consoleList;   /* the shared UART transmit list      */
extern SemaphoreHandle_t g_consoleMutex; /* mutex protecting g_consoleList  */
extern SemaphoreHandle_t g_uartMutex;  /* shared UART access mutex */

extern QueueHandle_t g_rxQueue;  /* single-byte UART RX queue */

/* -------------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/*
 * mydacq_msg_init()
 * Call once from MX_FREERTOS_Init() before any task is created.
 * Creates the mutex that protects the console list.
 * Returns 1 on success, 0 on failure (mutex creation failed).
 */
int mydacq_msg_init(void);

/*
 * mydacq_post()
 * Thread-safe wrapper around msgPost().
 * Acquires the console mutex, calls msgPost(), releases the mutex.
 *
 * Returns CONSOLEMSG_returncodes value.
 * Returns LINK_IS_BUSY if the mutex cannot be acquired within 100ms.
 */
int mydacq_post(msgList_type *pList, msgLink_type *pLink,
                char *bufp, size_t bufsize,
                const char *fmt, ...);

/*
 * mydacq_send()
 * Thread-safe wrapper around msgSend().
 * Acquires the mutex, sends one byte from the list head, releases mutex.
 *
 * Call this repeatedly from the consoleTask loop.
 * Returns MSGSEND_RETURNCODES value.
 */
int mydacq_send(msgList_type *pList);

/* Called from UART RX ISR - feeds g_rxQueue */
void mydacq_uart_rx_isr(uint8_t byte);

#endif /* MYDACQ_MSG_H */
