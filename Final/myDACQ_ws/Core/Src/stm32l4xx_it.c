/* stm32l4xx_it.c - Interrupt Service Routines for myDACQ */

#include "main.h"
#include "stm32l4xx_it.h"
#include "es_wifi_io.h"
#include "mydacq_msg.h"

/* htim6 owned by stm32l4xx_hal_timebase_tim.c */
extern TIM_HandleTypeDef  htim6;
extern UART_HandleTypeDef huart1;

/* -------------------------------------------------------------------------
 * Cortex-M4 core handlers
 * ----------------------------------------------------------------------- */
void NMI_Handler(void)      { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void)  { while (1) {} }
void UsageFault_Handler(void){ while (1) {} }
void DebugMon_Handler(void)  {}

void HardFault_Handler(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIOB->MODER &= ~(3U << (14 * 2));
    GPIOB->MODER |=  (1U << (14 * 2));
    while (1) {
        GPIOB->ODR ^= (1U << 14);
        for (volatile int i = 0; i < 200000; i++);
    }
}

/* -------------------------------------------------------------------------
 * Peripheral IRQ handlers
 * ----------------------------------------------------------------------- */

/* TIM6 - HAL timebase (owned by stm32l4xx_hal_timebase_tim.c, but the
   IRQ vector must be here or there - only one place, check which file
   CubeMX put TIM6_DAC_IRQHandler in. If it's already in timebase_tim.c,
   comment this out to avoid a duplicate. */
void TIM6_DAC_IRQHandler(void)  { HAL_TIM_IRQHandler(&htim6); }

void USART1_IRQHandler(void)    { HAL_UART_IRQHandler(&huart1); }

void SPI3_IRQHandler(void)
{
    extern SPI_HandleTypeDef hspi;   /* es_wifi_io.c's handle */
    HAL_SPI_IRQHandler(&hspi);
}

void EXTI1_IRQHandler(void)     { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(SPSGRF_915_GPIO3_EXTI5_Pin);
    HAL_GPIO_EXTI_IRQHandler(SPBTLE_RF_IRQ_EXTI6_Pin);
    HAL_GPIO_EXTI_IRQHandler(VL53L0X_GPIO1_EXTI7_Pin);
    HAL_GPIO_EXTI_IRQHandler(LSM3MDL_DRDY_EXTI8_Pin);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(LPS22HB_INT_DRDY_EXTI0_Pin);
    HAL_GPIO_EXTI_IRQHandler(LSM6DSL_INT1_EXTI11_Pin);
    HAL_GPIO_EXTI_IRQHandler(BUTTON_EXTI13_Pin);
    HAL_GPIO_EXTI_IRQHandler(ARD_D2_Pin);
    HAL_GPIO_EXTI_IRQHandler(HTS221_DRDY_EXTI15_Pin);
}

/* -------------------------------------------------------------------------
 * HAL callbacks
 * ----------------------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_1)
        SPI_WIFI_ISR();
}

/* HAL_UART_RxCpltCallback is defined in main.c - do not redefine here */
/* HAL_SPI_TxCpltCallback is defined in es_wifi_io.c - do not redefine here */
