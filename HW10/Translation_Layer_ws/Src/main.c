#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"
#include "wifi.h"
#include <string.h>

UART_HandleTypeDef huart;  /* global so ism_socket.c can use it */
static void SystemClock_Config(void);

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  g.Mode = GPIO_MODE_AF_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  g.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOB, &g);

  huart.Instance = USART1;
  huart.Init.BaudRate = 115200;
  huart.Init.WordLength = UART_WORDLENGTH_8B;
  huart.Init.StopBits = UART_STOPBITS_1;
  huart.Init.Parity = UART_PARITY_NONE;
  huart.Init.Mode = UART_MODE_TX_RX;
  huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart);

  BSP_LED_Init(LED2);

  //WIFI_Init();

  HAL_Delay(1000);

  extern void ism_client_main(void);
  ism_client_main();

  while(1) {}
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef c;
  RCC_OscInitTypeDef o;
  o.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  o.MSIState = RCC_MSI_ON;
  o.MSIClockRange = RCC_MSIRANGE_6;
  o.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  o.PLL.PLLState = RCC_PLL_ON;
  o.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  o.PLL.PLLM = 1;
  o.PLL.PLLN = 40;
  o.PLL.PLLR = 2;
  o.PLL.PLLP = 7;
  o.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&o);
  c.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  c.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  c.AHBCLKDivider = RCC_SYSCLK_DIV1;
  c.APB1CLKDivider = RCC_HCLK_DIV1;
  c.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&c, FLASH_LATENCY_4);
}

void SPI3_IRQHandler(void)
{
  extern SPI_HandleTypeDef hspi;
  HAL_SPI_IRQHandler(&hspi);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_1) { SPI_WIFI_ISR(); }
}
