#include "mydacq_msg.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Global shared objects - defined here, declared extern in mydacq_msg.h
 * ----------------------------------------------------------------------- */
msgList_type      g_consoleList   = { NULL, NULL, NULL, 0 };
SemaphoreHandle_t g_consoleMutex  = NULL;
SemaphoreHandle_t g_uartMutex     = NULL;
QueueHandle_t     g_rxQueue       = NULL;
QueueHandle_t     g_mqttTxQueue   = NULL;
QueueHandle_t     g_mqttRxQueue   = NULL;

/* -------------------------------------------------------------------------
 * Internal raw msgSend - caller must hold g_consoleMutex
 * ----------------------------------------------------------------------- */
static int _msgSend(msgList_type *pList)
{
    msgLink_type *ptemp;

    if (pList->listtailp == NULL)
        return UART_LIST_IS_EMPTY;

    /* Safety: bytecount hit zero but list not empty */
    if (pList->bytecount == 0) {
        if (pList->listheadp == pList->listtailp) {
            pList->listtailp->next = NULL;
            pList->listheadp = pList->listtailp = NULL;
            return UART_LIST_IS_EMPTY;
        } else {
            ptemp = pList->listheadp->next;
            pList->listheadp->next = NULL;
            pList->listheadp = ptemp;
            pList->byteptr   = pList->listheadp->addr;
            pList->bytecount = pList->listheadp->count;
            return UART_LIST_WAS_UPDATED;
        }
    }

    /* Send one byte */
    if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX)
        return UART_TX_IS_BUSY;

    if (HAL_UART_Transmit(&huart1, (uint8_t *)pList->byteptr, 1, 1) != HAL_OK)
        return UART_TX_FAILED;

    if (--(pList->bytecount) > 0) {
        pList->byteptr++;
        return UART_TX_OK;
    }

    /* Current link exhausted - retire it */
    if (pList->listheadp == pList->listtailp) {
        pList->listtailp->next = NULL;
        pList->listheadp = pList->listtailp = NULL;
        return UART_LIST_IS_EMPTY;
    } else {
        ptemp = pList->listheadp->next;
        pList->listheadp->next = NULL;
        pList->listheadp = ptemp;
        pList->byteptr   = pList->listheadp->addr;
        pList->bytecount = pList->listheadp->count;
        return UART_LIST_WAS_UPDATED;
    }
}

/* -------------------------------------------------------------------------
 * Internal raw msgPost - caller must hold g_consoleMutex
 * ----------------------------------------------------------------------- */
static int _msgPost(msgList_type *pList, msgLink_type *pLink,
                    char *bufp, size_t bufsize,
                    const char *fmt, va_list args)
{
    int len;

    if (pLink->next != NULL)
        return LINK_IS_BUSY;

    len = vsnprintf(bufp, bufsize, fmt, args);

    if (len <= 0)
        return LINK_SNPRINTF_FAILED;
    if ((size_t)len >= bufsize)
        len = (int)bufsize - 1;   /* truncated but usable */

    pLink->addr  = bufp;
    pLink->count = (uint32_t)len;

    if (pList->listtailp == NULL) {
        pList->listheadp = pList->listtailp = pLink;
        pList->byteptr   = pLink->addr;
        pList->bytecount = pLink->count;
    } else {
        pList->listtailp->next = pLink;
        pList->listtailp       = pLink;
    }

    pLink->next = pLink;   /* sentinel: tail points to itself */

    return LINK_IS_INSTALLED;
}

/* -------------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

int mydacq_msg_init(void)
{
    g_consoleMutex = xSemaphoreCreateMutex();
    if (g_consoleMutex == NULL) return 0;

    g_uartMutex = xSemaphoreCreateMutex();
    if (g_uartMutex == NULL) return 0;

    g_rxQueue = xQueueCreate(64, sizeof(uint8_t));
    if (g_rxQueue == NULL) return 0;

    g_mqttTxQueue = xQueueCreate(4, sizeof(MqttMsg_type));
    if (g_mqttTxQueue == NULL) return 0;

    g_mqttRxQueue = xQueueCreate(4, sizeof(MqttMsg_type));
    if (g_mqttRxQueue == NULL) return 0;

    g_consoleList.listheadp = NULL;
    g_consoleList.listtailp = NULL;
    g_consoleList.byteptr   = NULL;
    g_consoleList.bytecount = 0;

    return 1;
}

int mydacq_post(msgList_type *pList, msgLink_type *pLink,
                char *bufp, size_t bufsize,
                const char *fmt, ...)
{
    int     rc;
    va_list args;

    if (xSemaphoreTake(g_consoleMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return LINK_IS_BUSY;

    va_start(args, fmt);
    rc = _msgPost(pList, pLink, bufp, bufsize, fmt, args);
    va_end(args);

    xSemaphoreGive(g_consoleMutex);
    return rc;
}

int mydacq_send(msgList_type *pList)
{
    int rc;

    if (xSemaphoreTake(g_consoleMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return UART_TX_IS_BUSY;

    rc = _msgSend(pList);

    xSemaphoreGive(g_consoleMutex);
    return rc;
}

/* -------------------------------------------------------------------------
 * UART RX callback - feeds g_rxQueue from ISR context.
 * Wire this up in stm32l4xx_it.c's HAL_UART_RxCpltCallback.
 * ----------------------------------------------------------------------- */
void mydacq_uart_rx_isr(uint8_t byte)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_rxQueue, &byte, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
