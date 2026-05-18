#ifndef ES_WIFI_CONF_H
#define ES_WIFI_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#undef DEBUG
#endif

/* Pure polling mode - no semaphores, simpler and more reliable */
/* #define WIFI_USE_CMSIS_OS */

#ifdef WIFI_USE_CMSIS_OS
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Remap CMSIS-OS calls to native FreeRTOS */
typedef SemaphoreHandle_t osMutexId;
typedef SemaphoreHandle_t osSemaphoreId;

#define osMutexCreate(x)          xSemaphoreCreateMutex()
#define osMutexWait(m, t)         xSemaphoreTake(m, pdMS_TO_TICKS(t == 0 ? portMAX_DELAY : t))
#define osMutexRelease(m)         xSemaphoreGive(m)
#define osMutexDelete(m)          vSemaphoreDelete(m)

#define osSemaphoreCreate(x, c)   xSemaphoreCreateCounting(c, 0)
#define osSemaphoreWait(s, t)     (xSemaphoreTake(s, pdMS_TO_TICKS(t)) == pdTRUE ? 0 : -1)
#define osSemaphoreRelease(s)     xSemaphoreGive(s)
#define osSemaphoreDelete(s)      vSemaphoreDelete(s)

#define osMutexDef(x)
#define osSemaphoreDef(x)
#define osMutex(x)                NULL
#define osSemaphore(x)            NULL

extern SemaphoreHandle_t es_wifi_mutex;
extern SemaphoreHandle_t spi_mutex_handle;  /* defined in es_wifi_io.c as spi_mutex */

#define LOCK_SPI()      xSemaphoreTake(spi_mutex_handle, portMAX_DELAY)
#define UNLOCK_SPI()    xSemaphoreGive(spi_mutex_handle)
#define LOCK_WIFI()     xSemaphoreTake(es_wifi_mutex, portMAX_DELAY)
#define UNLOCK_WIFI()   xSemaphoreGive(es_wifi_mutex)

/* ISR-safe semaphore signal */
#define SEM_SIGNAL(a)   do { \
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; \
    xSemaphoreGiveFromISR(a, &xHigherPriorityTaskWoken); \
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); \
} while(0)

#define SEM_WAIT(a,t)   (xSemaphoreTake(a, pdMS_TO_TICKS(t)) == pdTRUE ? 0 : -1)

/* SPI_INTERFACE_PRIO must be > configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)
 * so numerically higher (lower urgency) = 6 or above.
 * This ensures xSemaphoreGiveFromISR() is safe to call from SPI/EXTI ISRs. */
#define SPI_INTERFACE_PRIO   6

#else /* no CMSIS_OS */
#define LOCK_WIFI()
#define UNLOCK_WIFI()
#define LOCK_SPI()
#define UNLOCK_SPI()
#define SEM_SIGNAL(a)
#define SPI_INTERFACE_PRIO   0
#endif /* WIFI_USE_CMSIS_OS */

#define ES_WIFI_MAX_SSID_NAME_SIZE      32
#define ES_WIFI_MAX_PSWD_NAME_SIZE      32
#define ES_WIFI_PRODUCT_ID_SIZE         32
#define ES_WIFI_PRODUCT_NAME_SIZE       32
#define ES_WIFI_FW_REV_SIZE             24
#define ES_WIFI_API_REV_SIZE            16
#define ES_WIFI_STACK_REV_SIZE          16
#define ES_WIFI_RTOS_REV_SIZE           16

#define ES_WIFI_DATA_SIZE               2000
#define ES_WIFI_MAX_DETECTED_AP         10
#define ES_WIFI_TIMEOUT                 30000
#define ES_WIFI_USE_PING                1
#define ES_WIFI_USE_AWS                 0
#define ES_WIFI_USE_FIRMWAREUPDATE      0
#define ES_WIFI_USE_WPS                 0
#define ES_WIFI_USE_SPI                 1
#define ES_WIFI_USE_UART                (!ES_WIFI_USE_SPI)

#ifdef __cplusplus
}
#endif
#endif /* ES_WIFI_CONF_H */
