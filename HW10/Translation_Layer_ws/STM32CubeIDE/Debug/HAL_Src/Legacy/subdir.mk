################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Src/Legacy/stm32l4xx_hal_can.c 

OBJS += \
./HAL_Src/Legacy/stm32l4xx_hal_can.o 

C_DEPS += \
./HAL_Src/Legacy/stm32l4xx_hal_can.d 


# Each subdirectory must supply rules for building sources it contributes
HAL_Src/Legacy/stm32l4xx_hal_can.o: C:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Src/Legacy/stm32l4xx_hal_can.c HAL_Src/Legacy/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-HAL_Src-2f-Legacy

clean-HAL_Src-2f-Legacy:
	-$(RM) ./HAL_Src/Legacy/stm32l4xx_hal_can.cyclo ./HAL_Src/Legacy/stm32l4xx_hal_can.d ./HAL_Src/Legacy/stm32l4xx_hal_can.o ./HAL_Src/Legacy/stm32l4xx_hal_can.su

.PHONY: clean-HAL_Src-2f-Legacy

