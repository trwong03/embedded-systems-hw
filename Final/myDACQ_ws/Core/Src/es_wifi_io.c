/**
  ******************************************************************************
  * @file    es_wifi_io.c
  * @author  MCD Application Team
  * @brief   This file implements the IO operations to deal with the es-wifi
  *          module. It mainly Inits and Deinits the SPI interface. Send and
  *          receive data over it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "es_wifi.h"
#include "es_wifi_io.h"
#include <string.h>
#include "es_wifi_conf.h"
#include <core_cm4.h>
#include "FreeRTOS.h"
#include "task.h"

/* Private define ------------------------------------------------------------*/
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi;
static  int volatile spi_rx_event = 0;
static  int volatile spi_tx_event = 0;
static  int volatile cmddata_rdy_rising_event = 0;

#ifdef WIFI_USE_CMSIS_OS
osMutexId es_wifi_mutex;
osMutexDef(es_wifi_mutex);

SemaphoreHandle_t spi_mutex_handle = NULL;   /* exported for LOCK_SPI/UNLOCK_SPI */


static    osSemaphoreId spi_rx_sem;
osSemaphoreDef(spi_rx_sem);

static    osSemaphoreId spi_tx_sem;
osSemaphoreDef(spi_tx_sem);

static    osSemaphoreId cmddata_rdy_rising_sem;
osSemaphoreDef(cmddata_rdy_rising_sem);
#endif /* WIFI_USE_CMSIS_OS */

/* Private function prototypes -----------------------------------------------*/
static  int wait_cmddata_rdy_high(int timeout);
static  int wait_cmddata_rdy_rising_event(int timeout);
static  int wait_spi_tx_event(int timeout);
static  int wait_spi_rx_event(int timeout);
static  void SPI_WIFI_DelayUs(uint32_t);

/*******************************************************************************
                       COM Driver Interface (SPI)
*******************************************************************************/

/**
  * @brief  Initialize SPI MSP
  */
void SPI_WIFI_GpioInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_Init;

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* WAKEUP pin HIGH - module must be awake */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_Init.Pin   = GPIO_PIN_13;
    GPIO_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull  = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Data ready pin - input interrupt */
    GPIO_Init.Pin   = GPIO_PIN_1;
    GPIO_Init.Mode  = GPIO_MODE_IT_RISING;
    GPIO_Init.Pull  = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    /* Reset pin */
    GPIO_Init.Pin       = GPIO_PIN_8;
    GPIO_Init.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull      = GPIO_NOPULL;
    GPIO_Init.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    /* NSS pin - deasserted (high) */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);
    GPIO_Init.Pin   = GPIO_PIN_0;
    GPIO_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull  = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    /* SPI CLK */
    GPIO_Init.Pin       = GPIO_PIN_10;
    GPIO_Init.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init.Pull      = GPIO_NOPULL;
    GPIO_Init.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_Init.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    /* SPI MOSI */
    GPIO_Init.Pin       = GPIO_PIN_12;
    GPIO_Init.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init.Pull      = GPIO_NOPULL;
    GPIO_Init.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_Init.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    /* SPI MISO */
    GPIO_Init.Pin       = GPIO_PIN_11;
    GPIO_Init.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init.Pull      = GPIO_PULLUP;
    GPIO_Init.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_Init.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_Init);
}

/**
  * @brief  Initialize the SPI3
  */
int8_t SPI_WIFI_Init(uint16_t mode)
{
    int8_t rc = 0;

    if (mode == ES_WIFI_INIT)
    {
        hspi.Instance               = SPI3;
        SPI_WIFI_GpioInit(&hspi);

        hspi.Init.Mode              = SPI_MODE_MASTER;
        hspi.Init.Direction         = SPI_DIRECTION_2LINES;
        hspi.Init.DataSize          = SPI_DATASIZE_16BIT;
        hspi.Init.CLKPolarity       = SPI_POLARITY_LOW;
        hspi.Init.CLKPhase          = SPI_PHASE_1EDGE;
        hspi.Init.NSS               = SPI_NSS_SOFT;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
        hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
        hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
        hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
        hspi.Init.CRCPolynomial     = 0;

        if (HAL_SPI_Init(&hspi) != HAL_OK)
            return -1;

        /* Enable DRDY interrupt */
        HAL_NVIC_SetPriority((IRQn_Type)EXTI1_IRQn, SPI_INTERFACE_PRIO, 0x00);
        HAL_NVIC_EnableIRQ((IRQn_Type)EXTI1_IRQn);

        /* Enable SPI interrupt */
        HAL_NVIC_SetPriority((IRQn_Type)SPI3_IRQn, SPI_INTERFACE_PRIO, 0);
        HAL_NVIC_EnableIRQ((IRQn_Type)SPI3_IRQn);

#ifdef WIFI_USE_CMSIS_OS
        cmddata_rdy_rising_event = 0;
        es_wifi_mutex            = osMutexCreate(osMutex(es_wifi_mutex));
        spi_mutex_handle                = osMutexCreate(NULL);
        spi_rx_sem               = osSemaphoreCreate(osSemaphore(spi_rx_sem), 1);
        spi_tx_sem               = osSemaphoreCreate(osSemaphore(spi_tx_sem), 1);
        cmddata_rdy_rising_sem   = osSemaphoreCreate(osSemaphore(cmddata_rdy_rising_sem), 1);
        SEM_WAIT(cmddata_rdy_rising_sem, 1);
        SEM_WAIT(spi_rx_sem, 1);
        SEM_WAIT(spi_tx_sem, 1);
#endif
        /* calibrate delay */
        SPI_WIFI_DelayUs(10);
    }

    rc = SPI_WIFI_ResetModule();
    return rc;
}

/**
  * @brief  Reset the ISM43362 module and read the boot prompt.
  *
  * The ISM43362 sends 6 bytes after reset: {0x15,0x15,'\r','\n','>',''}
  * This function resets the module, waits for it to boot, then reads
  * those 6 bytes via SPI polling.
  *
  * Timeout on HAL_SPI_Receive reduced to 1000ms (from 0xFFFF=65s)
  * so failures are detected quickly.
  */
int8_t SPI_WIFI_ResetModule(void)
{
    uint8_t           Prompt[6] = {0};
    uint8_t           count     = 0;
    HAL_StatusTypeDef Status;
    uint32_t          tickstart;

    WIFI_DISABLE_NSS();
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(50));
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(1000));

    WIFI_ENABLE_NSS();
    SPI_WIFI_DelayUs(15);

    tickstart = HAL_GetTick();

    while (WIFI_IS_CMDDATA_READY())
    {
        if (count > 4) { WIFI_DISABLE_NSS(); return -1; }
        Status = HAL_SPI_Receive(&hspi, &Prompt[count], 1, 1000);
        count += 2;
        if (Status != HAL_OK) { WIFI_DISABLE_NSS(); return -1; }
        if ((HAL_GetTick() - tickstart) > 5000) { WIFI_DISABLE_NSS(); return -1; }
    }

    WIFI_DISABLE_NSS();

    if ((Prompt[0] != 0x15) || (Prompt[1] != 0x15) ||
        (Prompt[2] != '\r') || (Prompt[3] != '\n') ||
        (Prompt[4] != '>')  || (Prompt[5] != ' '))
    {
        return -1;
    }
    return 0;
}

/**
  * @brief  DeInitialize the SPI
  */
int8_t SPI_WIFI_DeInit(void)
{
    HAL_SPI_DeInit(&hspi);
#ifdef WIFI_USE_CMSIS_OS
    osMutexDelete(spi_mutex_handle);
    osMutexDelete(es_wifi_mutex);
    osSemaphoreDelete(spi_tx_sem);
    osSemaphoreDelete(spi_rx_sem);
    osSemaphoreDelete(cmddata_rdy_rising_sem);
#endif
    return 0;
}

static int wait_cmddata_rdy_high(int timeout)
{
    int tickstart = HAL_GetTick();
    while (WIFI_IS_CMDDATA_READY() == 0)
    {
        if ((HAL_GetTick() - tickstart) > timeout)
            return -1;
    }
    return 0;
}

static int wait_cmddata_rdy_rising_event(int timeout)
{
    /* Pure polling - wait for DRDY (PE1) to go high */
    int tickstart = HAL_GetTick();
    while (!WIFI_IS_CMDDATA_READY())
    {
        if ((HAL_GetTick() - tickstart) > timeout)
            return -1;
    }
    cmddata_rdy_rising_event = 0;
    return 0;
}

static int wait_spi_rx_event(int timeout)
{
    /* Pure polling - wait for SPI RX complete flag */
    int tickstart = HAL_GetTick();
    while (spi_rx_event == 1)
    {
        if ((HAL_GetTick() - tickstart) > timeout)
            return -1;
    }
    return 0;
}

static int wait_spi_tx_event(int timeout)
{
    /* Pure polling - wait for SPI TX complete flag */
    int tickstart = HAL_GetTick();
    while (spi_tx_event == 1)
    {
        if ((HAL_GetTick() - tickstart) > timeout)
            return -1;
    }
    return 0;
}

int16_t SPI_WIFI_ReceiveData(uint8_t *pData, uint16_t len, uint32_t timeout)
{
    int16_t length = 0;
    uint8_t tmp[2];

    WIFI_DISABLE_NSS();
    UNLOCK_SPI();
    SPI_WIFI_DelayUs(3);

    if (wait_cmddata_rdy_rising_event(timeout) < 0)
        return ES_WIFI_ERROR_WAITING_DRDY_FALLING;

    LOCK_SPI();
    WIFI_ENABLE_NSS();
    SPI_WIFI_DelayUs(15);

    while (WIFI_IS_CMDDATA_READY())
    {
        if ((length < len) || (!len))
        {
            if (HAL_SPI_Receive(&hspi, tmp, 1, timeout) != HAL_OK)
            {
                WIFI_DISABLE_NSS();
                UNLOCK_SPI();
                return ES_WIFI_ERROR_SPI_FAILED;
            }

            pData[0] = tmp[0];
            pData[1] = tmp[1];
            length  += 2;
            pData   += 2;

            if (length >= ES_WIFI_DATA_SIZE)
            {
                WIFI_DISABLE_NSS();
                SPI_WIFI_ResetModule();
                UNLOCK_SPI();
                return ES_WIFI_ERROR_STUFFING_FOREVER;
            }
        }
        else
        {
            break;
        }
    }

    WIFI_DISABLE_NSS();
    UNLOCK_SPI();
    return length;
}

int16_t SPI_WIFI_SendData(const uint8_t *pdata, uint16_t len, uint32_t timeout)
{
    uint8_t Padding[2];

    if (wait_cmddata_rdy_high(timeout) < 0)
        return ES_WIFI_ERROR_SPI_FAILED;

    cmddata_rdy_rising_event = 1;
    LOCK_SPI();
    WIFI_ENABLE_NSS();
    SPI_WIFI_DelayUs(15);

    if (len > 1)
    {
        if (HAL_SPI_Transmit(&hspi, (uint8_t *)pdata, len / 2, timeout) != HAL_OK)
        {
            WIFI_DISABLE_NSS();
            UNLOCK_SPI();
            return ES_WIFI_ERROR_SPI_FAILED;
        }
    }

    if (len & 1)
    {
        Padding[0]   = pdata[len - 1];
        Padding[1]   = '\n';
        if (HAL_SPI_Transmit(&hspi, Padding, 1, timeout) != HAL_OK)
        {
            WIFI_DISABLE_NSS();
            UNLOCK_SPI();
            return ES_WIFI_ERROR_SPI_FAILED;
        }
    }

    return len;
}

void SPI_WIFI_Delay(uint32_t Delay)
{
    vTaskDelay(pdMS_TO_TICKS(Delay));
}

void SPI_WIFI_DelayUs(uint32_t n)
{
    volatile        uint32_t ct           = 0;
    uint32_t                 loop_per_us  = 0;
    static uint32_t          cycle_per_loop = 0;

    if (cycle_per_loop == 0)
    {
        uint32_t cycle_per_ms = (SystemCoreClock / 1000UL);
        uint32_t t = 0;
        ct = cycle_per_ms;
        t  = HAL_GetTick();
        while (ct) ct--;
        cycle_per_loop = HAL_GetTick() - t;
        if (cycle_per_loop == 0) cycle_per_loop = 1;
    }

    loop_per_us = SystemCoreClock / 1000000UL / cycle_per_loop;
    ct = n * loop_per_us;
    while (ct) ct--;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (spi_rx_event)
    {
        SEM_SIGNAL(spi_rx_sem);
        spi_rx_event = 0;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (spi_tx_event)
    {
        SEM_SIGNAL(spi_tx_sem);
        spi_tx_event = 0;
    }
}

void SPI_WIFI_ISR(void)
{
    if (cmddata_rdy_rising_event == 1)
    {
        SEM_SIGNAL(cmddata_rdy_rising_sem);
        cmddata_rdy_rising_event = 0;
    }
}

/* wrapper to allow pre-calibration from main.c before FreeRTOS starts */
void SPI_WIFI_CalibDelay(void)
{
    SPI_WIFI_DelayUs(1);
}
