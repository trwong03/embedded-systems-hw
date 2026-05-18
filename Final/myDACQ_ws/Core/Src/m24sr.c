/* m24sr.c
 * Driver for the M24SR64-Y NFC/EEPROM chip on the B-L475E-IOT01A.
 *
 * Ported from HW08 main.c. Key changes for myDACQ integration:
 *   - osDelay() replaced with vTaskDelay() (native FreeRTOS)
 *   - uart_print() debug calls removed (use mydacq_post instead)
 *   - All functions moved to this standalone module
 *
 * HARDWARE:
 *   M24SR64-Y sits on I2C2 at address 0xAC (0x56 << 1).
 *   RF must be disabled (PE2 high) before I2C access works.
 *   The chip uses APDU framing with CRC-16/CCITT (init 0x6363).
 *
 * PROTOCOL OVERVIEW:
 *   Every operation requires an active I2C session (get_session).
 *   Commands are wrapped in a frame: [PCB=0x02][cmd bytes][CRC16 lo][CRC16 hi]
 *   Responses arrive as:            [PCB][data bytes][SW1][SW2][CRC16 lo][CRC16 hi]
 *   The NDEF file must be selected before read/write operations.
 */

#include "m24sr.h"
#include "main.h"           /* provides hi2c2                              */
#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Hardware configuration - matches HW08 defines
 * ----------------------------------------------------------------------- */
#define M24SR_I2C_ADDR        (0x56 << 1)   /* 0xAC - memory read/write   */
#define M24SR_I2C_ADDR_CMD    (0x57 << 1)   /* 0xAE - extended commands   */
#define M24SR_HI2C            hi2c2
#define M24SR_I2C_TIMEOUT_MS  100
#define M24SR_WRITE_POLL_MS   5
#define M24SR_WRITE_POLL_MAX  40
#define M24SR_MEM_SIZE_BYTES  8192

/* -------------------------------------------------------------------------
 * APDU instruction bytes
 * ----------------------------------------------------------------------- */
#define M24SR_CMD_CLASS         0x00
#define M24SR_INS_SELECT        0xA4
#define M24SR_INS_READ          0xB0
#define M24SR_INS_UPDATE        0xD6

/* File IDs */
#define M24SR_FID_NDEF          0x0001   /* NDEF data file                 */
#define M24SR_FID_SYS           0xE101   /* System file (read-only UID)    */

/* -------------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/* Poll chip after a write until it acknowledges (write cycle complete) */
static int m24sr_wait_ready(void)
{
    for (int i = 0; i < M24SR_WRITE_POLL_MAX; i++)
    {
        if (HAL_I2C_Master_Transmit(&M24SR_HI2C, M24SR_I2C_ADDR,
                                     NULL, 0,
                                     M24SR_I2C_TIMEOUT_MS) == HAL_OK)
            return M24SR_OK;
        vTaskDelay(pdMS_TO_TICKS(M24SR_WRITE_POLL_MS));
    }
    return M24SR_ERR_TIMEOUT;
}

/* Acquire I2C session token - required before any APDU exchange */
static int m24sr_get_session(void)
{
    uint8_t cmd = 0x26;
    HAL_StatusTypeDef s = HAL_I2C_Master_Transmit(
            &M24SR_HI2C, M24SR_I2C_ADDR,
            &cmd, 1, M24SR_I2C_TIMEOUT_MS);
    if (s != HAL_OK) return M24SR_ERR_I2C;
    vTaskDelay(pdMS_TO_TICKS(5));
    return M24SR_OK;
}

/* Release any existing session - call before get_session to ensure clean state */
static int m24sr_kill_session(void)
{
    uint8_t cmd = 0x52;
    HAL_I2C_Master_Transmit(
            &M24SR_HI2C, M24SR_I2C_ADDR,
            &cmd, 1, M24SR_I2C_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(5));
    return M24SR_OK;
}

/* CRC-16/CCITT with initial value 0x6363 - required for M24SR frame integrity */
static uint16_t m24sr_crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x6363;
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t b = data[i] ^ (crc & 0xFF);
        b ^= (b << 4);
        crc = (crc >> 8) ^ ((uint16_t)b << 8) ^ ((uint16_t)b << 3) ^ (b >> 4);
    }
    return crc;
}

/*
 * m24sr_apdu()
 * Core I2C framing function. Builds and transmits an APDU frame,
 * then receives and parses the response.
 *
 * Frame format (outgoing):
 *   [0x02][cmd_len bytes of cmd][CRC16_lo][CRC16_hi]
 *
 * Response format (incoming):
 *   [PCB][data bytes][SW1][SW2][CRC16_lo][CRC16_hi]
 *
 * Parameters:
 *   cmd      : APDU command bytes (without PCB or CRC)
 *   cmd_len  : number of command bytes
 *   rsp      : buffer for response data (NULL if no data expected)
 *   rsp_len  : in=bytes expected, out=bytes received (NULL if no data)
 */
static int m24sr_apdu(uint8_t *cmd, uint8_t cmd_len,
                       uint8_t *rsp, uint8_t *rsp_len)
{
    /* build outgoing frame */
    uint8_t frame[32];
    frame[0] = 0x02;                          /* PCB byte                  */
    memcpy(&frame[1], cmd, cmd_len);
    uint16_t crc = m24sr_crc16(frame, 1 + cmd_len);
    frame[1 + cmd_len]     = crc & 0xFF;
    frame[1 + cmd_len + 1] = (crc >> 8) & 0xFF;

    /* transmit frame over I2C */
    HAL_StatusTypeDef s = HAL_I2C_Master_Transmit(
            &M24SR_HI2C, M24SR_I2C_ADDR,
            frame, 1 + cmd_len + 2, M24SR_I2C_TIMEOUT_MS);
    if (s != HAL_OK) return M24SR_ERR_I2C;

    /* wait for chip to process command */
    vTaskDelay(pdMS_TO_TICKS(20));

    /* receive response */
    uint8_t rbuf[32] = {0};
    s = HAL_I2C_Master_Receive(
            &M24SR_HI2C, M24SR_I2C_ADDR,
            rbuf, sizeof(rbuf), M24SR_I2C_TIMEOUT_MS);
    if (s != HAL_OK) return M24SR_ERR_I2C;

    /* extract data bytes if caller wants them */
    if (rsp && rsp_len && *rsp_len > 0)
    {
        uint8_t n = *rsp_len;
        memcpy(rsp, &rbuf[1], n);   /* skip PCB byte, copy n data bytes   */
    }
    else if (rsp && rsp_len)
    {
        *rsp_len = 0;
    }

    return M24SR_OK;
}

/* Select a file by FID before reading or writing it */
static int m24sr_select_file(uint16_t fid)
{
    uint8_t cmd[7] = {
        M24SR_CMD_CLASS,
        M24SR_INS_SELECT,
        0x00, 0x0C,
        0x02,
        (fid >> 8) & 0xFF,
        fid & 0xFF
    };
    uint8_t rsp[32];
    uint8_t rlen = 0;
    return m24sr_apdu(cmd, sizeof(cmd), rsp, &rlen);
}

/* -------------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

int setupM24SR(void)
{
    m24sr_kill_session();

    int rc = m24sr_get_session();
    if (rc != M24SR_OK) return rc;

    rc = m24sr_select_file(M24SR_FID_NDEF);
    return rc;
}

int peekM24SR(uint16_t adr, uint8_t *data)
{
    if (!data) return M24SR_ERR_PARAM;
    if (adr >= M24SR_MEM_SIZE_BYTES) return M24SR_ERR_PARAM;

    /* READ BINARY APDU: read 1 byte at offset adr */
    uint8_t cmd[5] = {
        M24SR_CMD_CLASS,
        M24SR_INS_READ,
        (adr >> 8) & 0xFF,   /* P1: offset high byte                      */
        adr & 0xFF,          /* P2: offset low byte                        */
        0x01                 /* Le: number of bytes to read                */
    };
    uint8_t rsp[4];
    uint8_t rlen = 1;
    int rc = m24sr_apdu(cmd, sizeof(cmd), rsp, &rlen);
    if (rc != M24SR_OK) return rc;
    *data = rsp[0];
    return M24SR_OK;
}

int pokeM24SR(uint16_t adr, uint8_t *data)
{
    if (!data) return M24SR_ERR_PARAM;
    if (adr >= M24SR_MEM_SIZE_BYTES) return M24SR_ERR_PARAM;

    /* UPDATE BINARY APDU: write 1 byte at offset adr */
    uint8_t cmd[6] = {
        M24SR_CMD_CLASS,
        M24SR_INS_UPDATE,
        (adr >> 8) & 0xFF,   /* P1: offset high byte                      */
        adr & 0xFF,          /* P2: offset low byte                        */
        0x01,                /* Lc: number of bytes to write               */
        *data                /* the byte to write                          */
    };
    uint8_t rlen = 0;
    int rc = m24sr_apdu(cmd, sizeof(cmd), NULL, &rlen);
    if (rc != M24SR_OK) return rc;
    return m24sr_wait_ready();
}

int peekmultiM24SR(uint16_t adr, uint8_t *data, uint16_t count)
{
    if (!data || count == 0) return M24SR_ERR_PARAM;
    if (adr + count > M24SR_MEM_SIZE_BYTES) return M24SR_ERR_PARAM;

    /* READ BINARY APDU: read 'count' bytes at offset adr */
    uint8_t cmd[5] = {
        M24SR_CMD_CLASS,
        M24SR_INS_READ,
        (adr >> 8) & 0xFF,
        adr & 0xFF,
        (uint8_t)count
    };
    uint8_t rlen = (uint8_t)count;
    return m24sr_apdu(cmd, sizeof(cmd), data, &rlen);
}

int pokemultiM24SR(uint16_t adr, uint8_t *data, uint16_t count)
{
    if (!data || count == 0) return M24SR_ERR_PARAM;
    if (adr + count > M24SR_MEM_SIZE_BYTES) return M24SR_ERR_PARAM;

    /* UPDATE BINARY APDU: write 'count' bytes at offset adr */
    uint8_t cmd[5 + count];
    cmd[0] = M24SR_CMD_CLASS;
    cmd[1] = M24SR_INS_UPDATE;
    cmd[2] = (adr >> 8) & 0xFF;
    cmd[3] = adr & 0xFF;
    cmd[4] = (uint8_t)count;
    memcpy(&cmd[5], data, count);

    uint8_t rlen = 0;
    int rc = m24sr_apdu(cmd, 5 + count, NULL, &rlen);
    if (rc != M24SR_OK) return rc;
    return m24sr_wait_ready();
}
