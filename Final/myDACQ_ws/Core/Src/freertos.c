#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "main.h"
#include "mydacq_msg.h"
#include "m24sr.h"
#include "cwpack.h"
#include <string.h>
#include <stdio.h>

/* Phase 2 enabled */
#define MYDACQ_PHASE2

#ifdef MYDACQ_PHASE2
#include "mqtt_client.h"
#include "ism43362_socket.h"
#include "wifi.h"
#include "ism43362_config.h"
#endif

/* -------------------------------------------------------------------------
 * Stack sizes and priorities
 * ----------------------------------------------------------------------- */
#define CONSOLE_TASK_STACK    256
#define DEVICE_TASK_STACK     384
#define COMMAND_TASK_STACK    256
#define MQTT_TASK_STACK      2048

#define CONSOLE_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define DEVICE_TASK_PRIORITY  (tskIDLE_PRIORITY + 1)
#define COMMAND_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define MQTT_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

/* -------------------------------------------------------------------------
 * Buffer sizes
 * ----------------------------------------------------------------------- */
#define CONSOLE_BUF_SIZE  128
#define MSGPACK_BUF_SIZE   32
#define CMD_BUF_SIZE       64

/* -------------------------------------------------------------------------
 * Message link pools
 * Each pool has N links and N matching buffers.
 * A link is FREE when link.next == NULL.
 * ----------------------------------------------------------------------- */

/* deviceTask: 2 links for human-readable output */
#define DEVICE_POOL_SIZE  2
static msgLink_type device_link[DEVICE_POOL_SIZE];
static char         device_buf [DEVICE_POOL_SIZE][CONSOLE_BUF_SIZE];

/* commandTask: 3 links for command responses */
#define CMD_POOL_SIZE  3
static msgLink_type cmd_link[CMD_POOL_SIZE];
static char         cmd_buf [CMD_POOL_SIZE][CONSOLE_BUF_SIZE];

/* mqttTask: 6 links for status messages - needs more during WiFi debug */
#define MQTT_POOL_SIZE  6
static msgLink_type mqtt_link[MQTT_POOL_SIZE];
static char         mqtt_buf [MQTT_POOL_SIZE][CONSOLE_BUF_SIZE];

/* MessagePack binary buffers (not posted as strings - used internally) */
static uint8_t pack_buf[MSGPACK_BUF_SIZE];

/* -------------------------------------------------------------------------
 * Device configuration (shared between deviceTask and commandTask)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t sample_interval_ms;
    uint16_t read_address;
    uint8_t  enabled;
} DeviceConfig_type;

static volatile DeviceConfig_type dev_cfg = {
    .sample_interval_ms = 1000,
    .read_address       = 0x0000,
    .enabled            = 1
};
static SemaphoreHandle_t dev_cfg_mutex = NULL;

/* -------------------------------------------------------------------------
 * Command receive buffer (built up byte by byte from g_rxQueue)
 * ----------------------------------------------------------------------- */
static uint8_t cmd_rx_buf[CMD_BUF_SIZE];
static uint8_t cmd_rx_idx = 0;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ----------------------------------------------------------------------- */
static void consoleTask (void *pvParameters);
static void deviceTask  (void *pvParameters);
static void commandTask (void *pvParameters);
static void mqttTask    (void *pvParameters);

/* -------------------------------------------------------------------------
 * find_free_link() - scan a pool for a link with next==NULL (free)
 * ----------------------------------------------------------------------- */
static msgLink_type *find_free_link(msgLink_type *pool, int pool_size)
{
    for (int i = 0; i < pool_size; i++) {
        if (pool[i].next == NULL)
            return &pool[i];
    }
    return NULL;
}

/* -------------------------------------------------------------------------
 * pool_buf() - given a link pointer, return the matching char buffer.
 * Template: pool and its parallel buf array must be the same size.
 * ----------------------------------------------------------------------- */
static char *pool_buf_device(msgLink_type *lnk)
{
    for (int i = 0; i < DEVICE_POOL_SIZE; i++)
        if (&device_link[i] == lnk) return device_buf[i];
    return NULL;
}
static char *pool_buf_cmd(msgLink_type *lnk)
{
    for (int i = 0; i < CMD_POOL_SIZE; i++)
        if (&cmd_link[i] == lnk) return cmd_buf[i];
    return NULL;
}
static char *pool_buf_mqtt(msgLink_type *lnk)
{
    for (int i = 0; i < MQTT_POOL_SIZE; i++)
        if (&mqtt_link[i] == lnk) return mqtt_buf[i];
    return NULL;
}

/* -------------------------------------------------------------------------
 * Static memory for FreeRTOS idle and timer tasks
 * (required when configSUPPORT_STATIC_ALLOCATION == 1)
 * ----------------------------------------------------------------------- */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t  xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = xIdleStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t      *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = xTimerStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    __disable_irq();
    while (1) {}
}

/* =========================================================================
 * MX_FREERTOS_Init - called from main() before vTaskStartScheduler()
 * ======================================================================= */
void MX_FREERTOS_Init(void)
{
    /* Zero-init all link pools so next==NULL (FREE) */
    for (int i = 0; i < DEVICE_POOL_SIZE; i++) device_link[i].next = NULL;
    for (int i = 0; i < CMD_POOL_SIZE;    i++) cmd_link[i].next    = NULL;
    for (int i = 0; i < MQTT_POOL_SIZE;   i++) mqtt_link[i].next   = NULL;

    /* Initialize messaging layer (creates mutexes and queues) */
    configASSERT(mydacq_msg_init() == 1);

    /* Device config mutex */
    dev_cfg_mutex = xSemaphoreCreateMutex();
    configASSERT(dev_cfg_mutex != NULL);

    /* Create tasks */
    configASSERT(xTaskCreate(consoleTask, "console", CONSOLE_TASK_STACK,
                             NULL, CONSOLE_TASK_PRIORITY, NULL) == pdPASS);
    configASSERT(xTaskCreate(deviceTask,  "device",  DEVICE_TASK_STACK,
                             NULL, DEVICE_TASK_PRIORITY,  NULL) == pdPASS);
    configASSERT(xTaskCreate(commandTask, "command", COMMAND_TASK_STACK,
                             NULL, COMMAND_TASK_PRIORITY, NULL) == pdPASS);
    configASSERT(xTaskCreate(mqttTask,    "mqtt",    MQTT_TASK_STACK,
                             NULL, MQTT_TASK_PRIORITY,    NULL) == pdPASS);
}

/* =========================================================================
 * consoleTask - drains g_consoleList to UART1 one byte at a time
 * ======================================================================= */
static void consoleTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        int rc = mydacq_send(&g_consoleList);
        if (rc == UART_LIST_IS_EMPTY)
            vTaskDelay(pdMS_TO_TICKS(1));
        else if (rc == UART_TX_IS_BUSY)
            taskYIELD();
    }
}

/* =========================================================================
 * deviceTask - reads M24SR, packs with CWPack, posts human-readable output
 * ======================================================================= */
static void deviceTask(void *pvParameters)
{
    (void)pvParameters;

    /* Give other init a moment to settle */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Setup M24SR NFC EEPROM */
    int setup_rc = setupM24SR();

    {
        msgLink_type *lnk = find_free_link(device_link, DEVICE_POOL_SIZE);
        if (lnk) {
            char *buf = pool_buf_device(lnk);
            mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                        "[device] myDACQ Phase 1 started. M24SR setup=%d\r\n"
                        "[device] Commands: s=toggle r=report +=slower -=faster\r\n",
                        setup_rc);
        }
    }

    for (;;) {
        uint32_t interval;
        uint16_t addr;
        uint8_t  en;

        if (xSemaphoreTake(dev_cfg_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            interval = dev_cfg.sample_interval_ms;
            addr     = dev_cfg.read_address;
            en       = dev_cfg.enabled;
            xSemaphoreGive(dev_cfg_mutex);
        } else {
            interval = 1000; addr = 0x0000; en = 1;
        }

        if (en) {
            /* Read one byte from M24SR */
            uint8_t data = 0;
            int     read_rc = peekM24SR(addr, &data);

            /* CWPack encode: map {addr: N, val: N} */
            cw_pack_context pc;
            cw_pack_context_init(&pc, pack_buf, MSGPACK_BUF_SIZE, NULL);
            cw_pack_map_size(&pc, 2);
            cw_pack_str(&pc, "addr", 4);
            cw_pack_unsigned(&pc, addr);
            cw_pack_str(&pc, "val", 3);
            cw_pack_unsigned(&pc, data);
            uint32_t packed_len = (pc.return_code == CWP_RC_OK)
                                  ? (uint32_t)(pc.current - pc.start) : 0;

            /* Post human-readable line + hex dump of packed bytes */
            msgLink_type *lnk = NULL;
            for (int retry = 0; retry < 5; retry++) {
                lnk = find_free_link(device_link, DEVICE_POOL_SIZE);
                if (lnk) break;
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            if (lnk) {
                char *buf = pool_buf_device(lnk);
                /* Build hex string of packed bytes inline */
                char hexstr[MSGPACK_BUF_SIZE * 2 + 1] = {0};
                for (uint32_t i = 0; i < packed_len && i < MSGPACK_BUF_SIZE; i++)
                    snprintf(&hexstr[i*2], 3, "%02X", pack_buf[i]);

                mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                            "[dev] addr=0x%04X val=0x%02X rc=%d pack(%lu)=%s\r\n",
                            (unsigned)addr, (unsigned)data, read_rc,
                            packed_len, hexstr);
            }

#ifdef MYDACQ_PHASE2
            /* Phase 2: forward packed bytes to mqttTask */
            if (packed_len > 0 && g_mqttTxQueue != NULL) {
                MqttMsg_type tx_msg;
                tx_msg.len = (uint16_t)(packed_len > MQTT_PAYLOAD_MAX
                                        ? MQTT_PAYLOAD_MAX : packed_len);
                memcpy(tx_msg.data, pack_buf, tx_msg.len);
                xQueueSend(g_mqttTxQueue, &tx_msg, 0);
            }
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(interval));
    }
}

/* =========================================================================
 * commandTask - receives characters from g_rxQueue, parses simple commands
 *
 * Commands (send from PuTTY, press Enter after each):
 *   s  - toggle sampling on/off
 *   r  - print current config report
 *   +  - double the sample interval (slower)
 *   -  - halve the sample interval (faster, min 200ms)
 * ======================================================================= */
static void commandTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t rx_byte;

    vTaskDelay(pdMS_TO_TICKS(600));

    for (;;) {
        if (xQueueReceive(g_rxQueue, &rx_byte, pdMS_TO_TICKS(100)) != pdTRUE)
            continue;

        /* Accumulate until Enter */
        if (rx_byte == '\r' || rx_byte == '\n') {
            if (cmd_rx_idx == 0) continue;

            uint8_t cmd = cmd_rx_buf[0];
            cmd_rx_idx  = 0;

            if (xSemaphoreTake(dev_cfg_mutex, pdMS_TO_TICKS(50)) != pdTRUE)
                continue;

            msgLink_type *lnk;
            char         *buf;

            switch (cmd) {
            case 's':
                dev_cfg.enabled = !dev_cfg.enabled;
                lnk = find_free_link(cmd_link, CMD_POOL_SIZE);
                if (lnk) {
                    buf = pool_buf_cmd(lnk);
                    mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                                "\r\n[cmd] Sampling %s\r\n",
                                dev_cfg.enabled ? "ON" : "OFF");
                }
                break;

            case 'r':
                lnk = find_free_link(cmd_link, CMD_POOL_SIZE);
                if (lnk) {
                    buf = pool_buf_cmd(lnk);
                    mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                                "\r\n[cmd] addr=0x%04X interval=%lums en=%d\r\n",
                                (unsigned)dev_cfg.read_address,
                                dev_cfg.sample_interval_ms,
                                (int)dev_cfg.enabled);
                }
                break;

            case '+':
                dev_cfg.sample_interval_ms *= 2;
                lnk = find_free_link(cmd_link, CMD_POOL_SIZE);
                if (lnk) {
                    buf = pool_buf_cmd(lnk);
                    mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                                "\r\n[cmd] interval -> %lu ms\r\n",
                                dev_cfg.sample_interval_ms);
                }
                break;

            case '-':
                if (dev_cfg.sample_interval_ms > 200)
                    dev_cfg.sample_interval_ms /= 2;
                lnk = find_free_link(cmd_link, CMD_POOL_SIZE);
                if (lnk) {
                    buf = pool_buf_cmd(lnk);
                    mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                                "\r\n[cmd] interval -> %lu ms\r\n",
                                dev_cfg.sample_interval_ms);
                }
                break;

            default:
                lnk = find_free_link(cmd_link, CMD_POOL_SIZE);
                if (lnk) {
                    buf = pool_buf_cmd(lnk);
                    mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                                "\r\n[cmd] unknown: '%c'  (s r + -)\r\n", cmd);
                }
                break;
            }

            xSemaphoreGive(dev_cfg_mutex);

        } else if (rx_byte >= 0x20 && rx_byte < 0x7F) {
            if (cmd_rx_idx < CMD_BUF_SIZE - 1)
                cmd_rx_buf[cmd_rx_idx++] = rx_byte;
        }
    }
}

/* =========================================================================
 * mqttTask
 * ======================================================================= */
static void mqttTask(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(2000));  /* wait for device/console tasks to settle */

#ifndef MYDACQ_PHASE2
    /* Phase 1 stub */
    msgLink_type *lnk = find_free_link(mqtt_link, MQTT_POOL_SIZE);
    if (lnk) {
        char *buf = pool_buf_mqtt(lnk);
        mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                    "[mqtt] Phase 1 mode - WiFi disabled. Define MYDACQ_PHASE2 to enable.\r\n");
    }
    vTaskSuspend(NULL);   /* suspend self forever */

#else
    /* Phase 2 implementation */
    MqttMsg_type msg;
    msgLink_type *lnk;
    char         *buf;

    /* Announce start */
    lnk = find_free_link(mqtt_link, MQTT_POOL_SIZE);
    if (lnk) {
        buf = pool_buf_mqtt(lnk);
        mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                    "[mqtt] task started, connecting...\r\n");
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    int rc = mqtt_init();

    if (rc != MQTT_OK) {
        vTaskSuspend(NULL);
    }

    /* Also post to console */
    for (int i = 0; i < 20; i++) {
        lnk = find_free_link(mqtt_link, MQTT_POOL_SIZE);
        if (lnk) break;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (lnk) {
        buf = pool_buf_mqtt(lnk);
        mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                    "[mqtt] connected! publishing to %s\r\n",
                    MYDACQ_TOPIC_DATA);
    }

    uint32_t last_publish = 0;

    for (;;) {
        if ((HAL_GetTick() - last_publish) >= 3000) {
            MqttMsg_type latest_msg;
            int got_msg = 0;

            while (xQueueReceive(g_mqttTxQueue, &msg, 0) == pdTRUE) {
                latest_msg = msg;
                got_msg = 1;
            }

            if (got_msg) {
                int pub_rc = mqtt_publish(MYDACQ_TOPIC_DATA,
                                          latest_msg.data, latest_msg.len);
                (void)pub_rc;   /* ignore publish errors - will retry next interval */
                last_publish = HAL_GetTick();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
#endif /* MYDACQ_PHASE2 */
}
