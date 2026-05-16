/* freertos.c
 * FreeRTOS task definitions for myDACQ Phase 1.
 *
 * ARCHITECTURE OVERVIEW:
 *
 *   deviceTask  ----msgPost---->  g_consoleList  ----msgSend----> UART1 (Putty)
 *                                      ^
 *   commandTask --msgPost (status)-----+
 *        |
 *        +--- receives chars from UART1, unpacks commands, configures device
 *
 * TASK SUMMARY:
 *   consoleTask  : drains g_consoleList over UART1. Highest priority of the
 *                  three so messages don't pile up in the list.
 *   deviceTask   : reads M24SR at a configurable interval, packs data with
 *                  CWPack, posts packed+human-readable output to console list.
 *   commandTask  : receives bytes from UART1, assembles MessagePack commands,
 *                  unpacks them and adjusts deviceTask behavior.
 *
 * LINK/BUFFER POOL DESIGN:
 *   Each task owns a small pool of msgLink_type + char buffers.
 *   A link is FREE when link.next == NULL.
 *   mydacq_post() checks this before use and returns LINK_IS_BUSY if not free.
 *   Pool size = 2 per task: one being sent, one being prepared.
 *   This is the minimum safe pool - expand if tasks need to queue more.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "main.h"
#include "mydacq_msg.h"

/* -------------------------------------------------------------------------
 * Stack sizes (in words, not bytes)
 * 256 words = 1024 bytes per task - enough for CWPack + HAL calls
 * ----------------------------------------------------------------------- */
#define CONSOLE_TASK_STACK 256
#define DEVICE_TASK_STACK 256
#define COMMAND_TASK_STACK 256

/* -------------------------------------------------------------------------
 * Task priorities
 * consoleTask highest: keeps UART output flowing.
 * deviceTask normal: periodic sensor reads.
 * commandTask normal: responds to user input.
 * ----------------------------------------------------------------------- */
#define CONSOLE_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define DEVICE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define COMMAND_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* -------------------------------------------------------------------------
 * Message buffer sizes
 * CONSOLE_BUF_SIZE: large enough for a human-readable + msgpack line.
 * CMD_BUF_SIZE: large enough for one incoming MessagePack command.
 * ----------------------------------------------------------------------- */
#define CONSOLE_BUF_SIZE 128
#define CMD_BUF_SIZE 64

/* -------------------------------------------------------------------------
 * deviceTask link/buffer pool (2 links, 2 buffers)
 * Pool of 2 lets one message be in the transmit list while the next
 * is being prepared. next==NULL marks a link as free.
 * ----------------------------------------------------------------------- */
static msgLink_type device_link[2] = {{NULL, NULL, 0, 0}, {NULL, NULL, 0, 0}};
static char device_buf[2][CONSOLE_BUF_SIZE];

/* -------------------------------------------------------------------------
 * commandTask link/buffer pool (1 link for status replies)
 * ----------------------------------------------------------------------- */
static msgLink_type cmd_link[1] = {{NULL, NULL, 0, 0}};
static char cmd_buf[1][CONSOLE_BUF_SIZE];

/* -------------------------------------------------------------------------
 * Device configuration - adjusted by commandTask at runtime.
 * Volatile because deviceTask reads and commandTask writes.
 * Protected by dev_cfg_mutex.
 * ----------------------------------------------------------------------- */
typedef struct
{
  uint32_t sample_interval_ms; /* how often deviceTask reads M24SR       */
  uint16_t read_address;       /* which M24SR byte address to read       */
  uint8_t enabled;             /* 1 = sampling active, 0 = paused        */
} DeviceConfig_type;

static volatile DeviceConfig_type dev_cfg = {
    .sample_interval_ms = 1000, /* default: read once per second          */
    .read_address = 0x0000,     /* default: byte 0 of NDEF file           */
    .enabled = 1};

static SemaphoreHandle_t dev_cfg_mutex = NULL; /* protects dev_cfg         */

/* -------------------------------------------------------------------------
 * Command receive buffer - filled by commandTask character by character.
 * cmd_rx_buf accumulates incoming bytes until a complete MessagePack
 * message is detected (we use a fixed length protocol in Phase 1).
 * ----------------------------------------------------------------------- */
static uint8_t cmd_rx_buf[CMD_BUF_SIZE];
static uint8_t cmd_rx_idx = 0;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ----------------------------------------------------------------------- */
static void consoleTask(void *pvParameters);
static void deviceTask(void *pvParameters);
static void commandTask(void *pvParameters);

/* -------------------------------------------------------------------------
 * Helper: find a free link in a pool.
 * Returns pointer to a free link, or NULL if all are busy.
 * A link is free when link->next == NULL.
 * ----------------------------------------------------------------------- */
static msgLink_type *find_free_link(msgLink_type *pool, int pool_size)
{
  for (int i = 0; i < pool_size; i++)
  {
    if (pool[i].next == NULL)
      return &pool[i];
  }
  return NULL;
}

/* -------------------------------------------------------------------------
 * Helper: find the buffer belonging to a link in a pool.
 * Returns the char buffer at the same index as the link.
 * ----------------------------------------------------------------------- */
static char *link_buf(msgLink_type *pool, char buf[][CONSOLE_BUF_SIZE],
                      msgLink_type *link)
{
  for (int i = 0; i < 2; i++)
  {
    if (&pool[i] == link)
      return buf[i];
  }
  return NULL; /* should never happen */
}

/* =========================================================================
 * MX_FREERTOS_Init()
 * Called from main.c before osKernelStart().
 * Creates mutex, initializes messaging layer, creates all tasks.
 * ======================================================================= */
void MX_FREERTOS_Init(void)
{
  /* Step 1: initialize the thread-safe messaging layer.
     This creates g_consoleMutex. Must happen before any task runs. */
  configASSERT(mydacq_msg_init() == 1);

  /* Step 2: create the device config mutex */
  dev_cfg_mutex = xSemaphoreCreateMutex();
  configASSERT(dev_cfg_mutex != NULL);

  /* Step 3: create tasks */
  xTaskCreate(consoleTask, "console", CONSOLE_TASK_STACK,
              NULL, CONSOLE_TASK_PRIORITY, NULL);

  xTaskCreate(deviceTask, "device", DEVICE_TASK_STACK,
              NULL, DEVICE_TASK_PRIORITY, NULL);

  xTaskCreate(commandTask, "command", COMMAND_TASK_STACK,
              NULL, COMMAND_TASK_PRIORITY, NULL);
}

/* =========================================================================
 * consoleTask
 *
 * Drains g_consoleList one byte at a time over UART1.
 * Runs at the highest priority of the three tasks so the output queue
 * doesn't grow faster than it's drained.
 *
 * When the list is empty it yields for 1ms to avoid burning CPU.
 * When actively sending it yields for 0 ticks (taskYIELD) to stay
 * responsive but still let equal-priority tasks run.
 * ======================================================================= */
static void consoleTask(void *pvParameters)
{
  (void)pvParameters;
  int rc;

  for (;;)
  {
    rc = mydacq_send(&g_consoleList);

    if (rc == UART_LIST_IS_EMPTY)
    {
      /* nothing to send - sleep briefly to avoid spinning */
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    else if (rc == UART_TX_IS_BUSY)
    {
      /* UART hardware busy - yield and retry immediately */
      taskYIELD();
    }
    /* UART_TX_OK and UART_LIST_WAS_UPDATED: loop immediately */
  }
}

/* =========================================================================
 * deviceTask
 *
 * Reads one byte from the M24SR at dev_cfg.read_address every
 * dev_cfg.sample_interval_ms milliseconds.
 *
 * The read value is formatted as a human-readable string and posted
 * to g_consoleList. In Phase 1 this is plain text. In a later step
 * we will add CWPack serialization here so the data is also
 * MessagePack-encoded before posting.
 *
 * Link pool rotation: tries link[0] first, then link[1].
 * If both are busy (still being transmitted) it skips this sample
 * and logs a warning. This is the correct behavior - we never block
 * waiting for a link because that would stall the sampling interval.
 * ======================================================================= */
static void deviceTask(void *pvParameters)
{
  (void)pvParameters;

  /* Wait for the console to be ready before sending anything */
  vTaskDelay(pdMS_TO_TICKS(500));

  /* Announce startup */
  {
    msgLink_type *lnk = find_free_link(device_link, 2);
    if (lnk != NULL)
    {
      char *buf = link_buf(device_link, device_buf, lnk);
      mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                  "[device] myDACQ started. Sampling M24SR @ 0x%04X "
                  "every %lu ms\r\n",
                  (unsigned)dev_cfg.read_address,
                  dev_cfg.sample_interval_ms);
    }
  }

  for (;;)
  {
    uint32_t interval;
    uint16_t addr;
    uint8_t en;

    /* read config safely */
    if (xSemaphoreTake(dev_cfg_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      interval = dev_cfg.sample_interval_ms;
      addr = dev_cfg.read_address;
      en = dev_cfg.enabled;
      xSemaphoreGive(dev_cfg_mutex);
    }
    else
    {
      /* couldn't get config - use safe defaults and carry on */
      interval = 1000;
      addr = 0x0000;
      en = 1;
    }

    if (en)
    {
      /* TODO Step 4: call peekM24SR(addr, &data) here.
       * For now we post a placeholder so the pipeline can be tested
       * end-to-end before the M24SR driver is integrated. */
      uint8_t data = 0xAA; /* placeholder - replaced in Step 4 */

      msgLink_type *lnk = find_free_link(device_link, 2);
      if (lnk != NULL)
      {
        char *buf = link_buf(device_link, device_buf, lnk);
        /* Phase 1 format: human-readable.
         * Step 5 will add MessagePack encoding alongside this. */
        mydacq_post(&g_consoleList, lnk, buf, CONSOLE_BUF_SIZE,
                    "[device] M24SR[0x%04X] = 0x%02X\r\n",
                    (unsigned)addr, (unsigned)data);
      }
      else
      {
        /* both links busy - transmit list is backed up.
         * This is a sign the console baud rate or task priority
         * needs adjustment. We simply skip this sample. */
      }
    }

    vTaskDelay(pdMS_TO_TICKS(interval));
  }
}

/* =========================================================================
 * commandTask
 *
 * Receives bytes from UART1 (Putty keyboard input) one at a time,
 * assembles them into a command, then acts on the command.
 *
 * Phase 1 command protocol (simple text for now, MessagePack in Step 6):
 *   's' + Enter  : toggle sampling on/off
 *   'r' + Enter  : report current config to console
 *   '+' + Enter  : double the sample interval
 *   '-' + Enter  : halve the sample interval (minimum 100ms)
 *
 * The Enter key ('\r' or '\n') triggers command processing.
 * All other bytes are buffered.
 *
 * In Step 6 we will replace this text protocol with MessagePack framing.
 * ======================================================================= */
static void commandTask(void *pvParameters)
{
  (void)pvParameters;
  uint8_t rx_byte;

  /* wait for system to stabilize */
  vTaskDelay(pdMS_TO_TICKS(600));

  /* announce ready */
  {
    msgLink_type *lnk = find_free_link(cmd_link, 1);
    if (lnk != NULL)
    {
      mydacq_post(&g_consoleList, lnk, cmd_buf[0], CONSOLE_BUF_SIZE,
                  "[cmd] Ready. Commands: s=toggle r=report +=slower -=faster\r\n");
    }
  }

  for (;;)
  {
    /* blocking receive - wait up to 100ms for a byte from Putty.
       HAL_UART_Receive with timeout is safe inside a FreeRTOS task
       as long as HAL_GetTick() is overridden (done in main.c Step 1). */
    HAL_StatusTypeDef status = HAL_UART_Receive(
        &huart1, &rx_byte, 1, 100);

    if (status == HAL_OK)
    {
      /* echo the character back so the user can see what they typed */
      HAL_UART_Transmit(&huart1, &rx_byte, 1, 10);

      if (rx_byte == '\r' || rx_byte == '\n')
      {
        /* process whatever is in the buffer */
        if (cmd_rx_idx > 0)
        {
          uint8_t cmd = cmd_rx_buf[0];
          cmd_rx_idx = 0; /* reset buffer */

          /* acquire config mutex and apply command */
          if (xSemaphoreTake(dev_cfg_mutex,
                             pdMS_TO_TICKS(50)) == pdTRUE)
          {
            msgLink_type *lnk;

            if (cmd == 's')
            {
              dev_cfg.enabled = !dev_cfg.enabled;
              lnk = find_free_link(cmd_link, 1);
              if (lnk)
                mydacq_post(&g_consoleList, lnk,
                            cmd_buf[0], CONSOLE_BUF_SIZE,
                            "[cmd] Sampling %s\r\n",
                            dev_cfg.enabled ? "ON" : "OFF");
            }
            else if (cmd == 'r')
            {
              lnk = find_free_link(cmd_link, 1);
              if (lnk)
                mydacq_post(&g_consoleList, lnk,
                            cmd_buf[0], CONSOLE_BUF_SIZE,
                            "[cmd] addr=0x%04X interval=%lums "
                            "enabled=%d\r\n",
                            (unsigned)dev_cfg.read_address,
                            dev_cfg.sample_interval_ms,
                            (int)dev_cfg.enabled);
            }
            else if (cmd == '+')
            {
              dev_cfg.sample_interval_ms *= 2;
              lnk = find_free_link(cmd_link, 1);
              if (lnk)
                mydacq_post(&g_consoleList, lnk,
                            cmd_buf[0], CONSOLE_BUF_SIZE,
                            "[cmd] interval -> %lu ms\r\n",
                            dev_cfg.sample_interval_ms);
            }
            else if (cmd == '-')
            {
              if (dev_cfg.sample_interval_ms > 200)
                dev_cfg.sample_interval_ms /= 2;
              lnk = find_free_link(cmd_link, 1);
              if (lnk)
                mydacq_post(&g_consoleList, lnk,
                            cmd_buf[0], CONSOLE_BUF_SIZE,
                            "[cmd] interval -> %lu ms\r\n",
                            dev_cfg.sample_interval_ms);
            }
            else
            {
              lnk = find_free_link(cmd_link, 1);
              if (lnk)
                mydacq_post(&g_consoleList, lnk,
                            cmd_buf[0], CONSOLE_BUF_SIZE,
                            "[cmd] unknown: '%c'\r\n", cmd);
            }

            xSemaphoreGive(dev_cfg_mutex);
          }
        }
      }
      else
      {
        /* buffer the incoming byte */
        if (cmd_rx_idx < CMD_BUF_SIZE - 1)
        {
          cmd_rx_buf[cmd_rx_idx++] = rx_byte;
        }
      }
    }
    /* timeout (HAL_TIMEOUT) is normal - just loop and listen again */
  }
}