################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01.c 

OBJS += \
./BSP_Src/stm32l475e_iot01.o 

C_DEPS += \
./BSP_Src/stm32l475e_iot01.d 


# Each subdirectory must supply rules for building sources it contributes
BSP_Src/stm32l475e_iot01.o: C:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01/stm32l475e_iot01.c BSP_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP_Src

clean-BSP_Src:
	-$(RM) ./BSP_Src/stm32l475e_iot01.cyclo ./BSP_Src/stm32l475e_iot01.d ./BSP_Src/stm32l475e_iot01.o ./BSP_Src/stm32l475e_iot01.su

.PHONY: clean-BSP_Src

