/* m24sr.h
 * Driver for the M24SR64-Y NFC/EEPROM chip on the B-L475E-IOT01A.
 * Communicates over I2C2 using APDU framing with CRC-16/CCITT.
 *
 * Ported from HW08 main.c for use as a standalone FreeRTOS-safe driver.
 * osDelay() calls replaced with vTaskDelay() for native FreeRTOS compatibility.
 */

#ifndef M24SR_H
#define M24SR_H

#include <stdint.h>

/* Return codes */
#define M24SR_OK           0
#define M24SR_ERR_PARAM   -1
#define M24SR_ERR_I2C     -2
#define M24SR_ERR_TIMEOUT -3

/*
 * setupM24SR()
 * Kills any existing session, acquires a new I2C session token,
 * and selects the NDEF data file ready for peek/poke operations.
 * Call once from deviceTask before any reads or writes.
 * Returns M24SR_OK on success.
 */
int setupM24SR(void);

/*
 * peekM24SR(adr, data)
 * Reads one byte from NDEF file offset 'adr' into *data.
 * Returns M24SR_OK on success.
 */
int peekM24SR(uint16_t adr, uint8_t *data);

/*
 * pokeM24SR(adr, data)
 * Writes one byte from *data to NDEF file offset 'adr'.
 * Returns M24SR_OK on success.
 */
int pokeM24SR(uint16_t adr, uint8_t *data);

/*
 * peekmultiM24SR(adr, data, count)
 * Reads 'count' bytes starting at NDEF file offset 'adr' into data[].
 * Returns M24SR_OK on success.
 */
int peekmultiM24SR(uint16_t adr, uint8_t *data, uint16_t count);

/*
 * pokemultiM24SR(adr, data, count)
 * Writes 'count' bytes from data[] starting at NDEF file offset 'adr'.
 * Returns M24SR_OK on success.
 */
int pokemultiM24SR(uint16_t adr, uint8_t *data, uint16_t count);

#endif /* M24SR_H */
