/* mydacq_msg.c
 * Thread-safe messaging layer for myDACQ project.
 *
 * DESIGN NOTES:
 *
 * The critical region problem (HW07):
 *   msgPost() manipulates the list tail pointer and link->next.
 *   msgSend() reads the list head pointer and advances it.
 *   If both run concurrently (different tasks), a task switch mid-operation
 *   can leave the list in a corrupt state - a link half-installed, or the
 *   head advanced past a link that hasn't finished being appended.
 *
 * The fix:
 *   A single FreeRTOS mutex (g_consoleMutex) serializes all access to
 *   g_consoleList. Any task that wants to post must acquire the mutex first.
 *   The consoleTask acquires the mutex for each byte it sends, then releases
 *   it immediately - this keeps the mutex held for the shortest possible time
 *   so posting tasks are not blocked for long.
 *
 * Link ownership rules (unchanged from consolemsg05.c):
 *   link->next == NULL       : link is FREE, safe to pass to mydacq_post()
 *   link->next == link       : link is the LIST TAIL
 *   link->next == other link : link is MID-LIST
 *   Never pass a non-NULL next link to mydacq_post() - it will return
 *   LINK_IS_BUSY without modifying anything.
 */

#include "mydacq_msg.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"          /* provides huart1                              */
#include "stm32l4xx_hal.h"
#include <stdarg.h>
#include <string.h>

/* mpaland's printf - same as consolemsg05.c uses */
#include "mpaland_printf.h"

/* -------------------------------------------------------------------------
 * Global shared objects - defined here, declared extern in mydacq_msg.h
 * ----------------------------------------------------------------------- */
msgList_type     g_consoleList  = { NULL, NULL, NULL, 0 };
SemaphoreHandle_t g_consoleMutex = NULL;

/* -------------------------------------------------------------------------
 * Internal: the raw msgSend() and msgPost() logic from consolemsg05.c,
 * reproduced here so we control them. They are NOT called directly by
 * application code - only through the mutex-protected wrappers below.
 * ----------------------------------------------------------------------- */

static int _msgSend(msgList_type *pList)
{
    msgLink_type *ptemp;

    if (pList->listtailp == NULL)
        return UART_LIST_IS_EMPTY;

    if (pList->bytecount > 0) {
        /* send one byte from the current head link */
        if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX)
            return UART_TX_IS_BUSY;

        if (HAL_UART_Transmit(&huart1,
                              (uint8_t *)pList->byteptr,
                              1, 1) != HAL_OK)
            return UART_TX_FAILED;

        /* advance or retire the current link */
        if (--(pList->bytecount) > 0) {
            pList->byteptr++;
            return UART_TX_OK;
        } else {
            /* current link exhausted */
            if (pList->listheadp == pList->listtailp) {
                /* it was the last link - empty the list */
                pList->listtailp->next = NULL;
                pList->listheadp       = NULL;
                pList->listtailp       = NULL;
                return UART_LIST_IS_EMPTY;
            } else {
                /* advance to the next link */
                ptemp                  = pList->listheadp->next;
                pList->listheadp->next = NULL;       /* free retired link   */
                pList->listheadp       = ptemp;
                pList->byteptr         = pList->listheadp->addr;
                pList->bytecount       = pList->listheadp->count;
                return UART_LIST_WAS_UPDATED;
            }
        }
    }
    return UART_LIST_DEFAULT_RETURN;
}

static int _msgPost(msgList_type *pList, msgLink_type *pLink,
                    char *bufp, size_t bufsize,
                    const char *fmt, va_list args)
{
    int len;

    /* reject if link is already in a list */
    if (pLink->next != NULL)
        return LINK_IS_BUSY;

    /* format the message into the caller's buffer */
    len = vsnprintf_(bufp, bufsize, fmt, args);

    if (len <= 0)
        return LINK_SNPRINTF_FAILED;
    if (len >= (int)bufsize)
        return LINK_MSG_TOO_LONG;

    /* populate the link */
    pLink->addr  = bufp;
    pLink->count = (uint32_t)len;

    /* append link to list */
    if (pList->listtailp == NULL) {
        /* list was empty - this link is both head and tail */
        pList->listheadp  = pList->listtailp = pLink;
        pList->byteptr    = pLink->addr;
        pList->bytecount  = pLink->count;
    } else {
        /* append to existing list */
        pList->listtailp->next = pLink;
        pList->listtailp       = pLink;
    }

    /* mark link as tail (points to itself) */
    pLink->next = pLink;

    return LINK_IS_INSTALLED;
}

/* -------------------------------------------------------------------------
 * Public API implementation
 * ----------------------------------------------------------------------- */

int mydacq_msg_init(void)
{
    g_consoleMutex = xSemaphoreCreateMutex();
    if (g_consoleMutex == NULL)
        return 0;   /* failure - not enough heap */

    /* ensure list starts clean */
    g_consoleList.listheadp = NULL;
    g_consoleList.listtailp = NULL;
    g_consoleList.byteptr   = NULL;
    g_consoleList.bytecount = 0;

    return 1;   /* success */
}

int mydacq_post(msgList_type *pList, msgLink_type *pLink,
                char *bufp, size_t bufsize,
                const char *fmt, ...)
{
    int      rc;
    va_list  args;

    /* 100ms timeout - if the mutex is held longer than this something
       has gone wrong (a task died holding it, etc.)                    */
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

    /* acquire mutex for the duration of one byte send.
       Use a short timeout - if UART is busy we'll just retry next loop. */
    if (xSemaphoreTake(g_consoleMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        return UART_TX_IS_BUSY;

    rc = _msgSend(pList);

    xSemaphoreGive(g_consoleMutex);
    return rc;
}