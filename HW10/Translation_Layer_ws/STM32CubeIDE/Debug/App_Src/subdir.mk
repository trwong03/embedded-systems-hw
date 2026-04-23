################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/client_ism43362.c \
C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/ism43362_socket.c \
C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/main.c \
C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/stm32l4xx_it.c \
C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/system_stm32l4xx.c 

OBJS += \
./App_Src/client_ism43362.o \
./App_Src/ism43362_socket.o \
./App_Src/main.o \
./App_Src/stm32l4xx_it.o \
./App_Src/system_stm32l4xx.o 

C_DEPS += \
./App_Src/client_ism43362.d \
./App_Src/ism43362_socket.d \
./App_Src/main.d \
./App_Src/stm32l4xx_it.d \
./App_Src/system_stm32l4xx.d 


# Each subdirectory must supply rules for building sources it contributes
App_Src/client_ism43362.o: C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/client_ism43362.c App_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
App_Src/ism43362_socket.o: C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/ism43362_socket.c App_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
App_Src/main.o: C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/main.c App_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
App_Src/stm32l4xx_it.o: C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/stm32l4xx_it.c App_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
App_Src/system_stm32l4xx.o: C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Src/system_stm32l4xx.c App_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32L475E_IOT01 -DSTM32L475xx -c -IDocuments/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc -I"C:/Users/Tyler/Documents/School/ENEE452/Projects/HW10/Translation_Layer_ws/Inc" -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/CMSIS/Device/ST/STM32L4xx/Include -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/STM32L4xx_HAL_Driver/Inc -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Drivers/BSP/B-L475E-IOT01 -IC:/Users/Tyler/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2/Projects/B-L475E-IOT01A/Applications/WiFi/Common/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App_Src

clean-App_Src:
	-$(RM) ./App_Src/client_ism43362.cyclo ./App_Src/client_ism43362.d ./App_Src/client_ism43362.o ./App_Src/client_ism43362.su ./App_Src/ism43362_socket.cyclo ./App_Src/ism43362_socket.d ./App_Src/ism43362_socket.o ./App_Src/ism43362_socket.su ./App_Src/main.cyclo ./App_Src/main.d ./App_Src/main.o ./App_Src/main.su ./App_Src/stm32l4xx_it.cyclo ./App_Src/stm32l4xx_it.d ./App_Src/stm32l4xx_it.o ./App_Src/stm32l4xx_it.su ./App_Src/system_stm32l4xx.cyclo ./App_Src/system_stm32l4xx.d ./App_Src/system_stm32l4xx.o ./App_Src/system_stm32l4xx.su

.PHONY: clean-App_Src

