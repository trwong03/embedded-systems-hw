################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/cwpack.c \
../Core/Src/es_wifi.c \
../Core/Src/es_wifi_io.c \
../Core/Src/freertos.c \
../Core/Src/ism43362_socket.c \
../Core/Src/m24sr.c \
../Core/Src/main.c \
../Core/Src/mpaland_printf.c \
../Core/Src/mpaland_printf_services.c \
../Core/Src/mqtt_client.c \
../Core/Src/mydacq_msg.c \
../Core/Src/stm32l475e_iot01.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_hal_timebase_tim.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/wifi.c 

OBJS += \
./Core/Src/cwpack.o \
./Core/Src/es_wifi.o \
./Core/Src/es_wifi_io.o \
./Core/Src/freertos.o \
./Core/Src/ism43362_socket.o \
./Core/Src/m24sr.o \
./Core/Src/main.o \
./Core/Src/mpaland_printf.o \
./Core/Src/mpaland_printf_services.o \
./Core/Src/mqtt_client.o \
./Core/Src/mydacq_msg.o \
./Core/Src/stm32l475e_iot01.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_hal_timebase_tim.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/wifi.o 

C_DEPS += \
./Core/Src/cwpack.d \
./Core/Src/es_wifi.d \
./Core/Src/es_wifi_io.d \
./Core/Src/freertos.d \
./Core/Src/ism43362_socket.d \
./Core/Src/m24sr.d \
./Core/Src/main.d \
./Core/Src/mpaland_printf.d \
./Core/Src/mpaland_printf_services.d \
./Core/Src/mqtt_client.d \
./Core/Src/mydacq_msg.d \
./Core/Src/stm32l475e_iot01.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_hal_timebase_tim.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/wifi.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L475xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/cwpack.cyclo ./Core/Src/cwpack.d ./Core/Src/cwpack.o ./Core/Src/cwpack.su ./Core/Src/es_wifi.cyclo ./Core/Src/es_wifi.d ./Core/Src/es_wifi.o ./Core/Src/es_wifi.su ./Core/Src/es_wifi_io.cyclo ./Core/Src/es_wifi_io.d ./Core/Src/es_wifi_io.o ./Core/Src/es_wifi_io.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/ism43362_socket.cyclo ./Core/Src/ism43362_socket.d ./Core/Src/ism43362_socket.o ./Core/Src/ism43362_socket.su ./Core/Src/m24sr.cyclo ./Core/Src/m24sr.d ./Core/Src/m24sr.o ./Core/Src/m24sr.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mpaland_printf.cyclo ./Core/Src/mpaland_printf.d ./Core/Src/mpaland_printf.o ./Core/Src/mpaland_printf.su ./Core/Src/mpaland_printf_services.cyclo ./Core/Src/mpaland_printf_services.d ./Core/Src/mpaland_printf_services.o ./Core/Src/mpaland_printf_services.su ./Core/Src/mqtt_client.cyclo ./Core/Src/mqtt_client.d ./Core/Src/mqtt_client.o ./Core/Src/mqtt_client.su ./Core/Src/mydacq_msg.cyclo ./Core/Src/mydacq_msg.d ./Core/Src/mydacq_msg.o ./Core/Src/mydacq_msg.su ./Core/Src/stm32l475e_iot01.cyclo ./Core/Src/stm32l475e_iot01.d ./Core/Src/stm32l475e_iot01.o ./Core/Src/stm32l475e_iot01.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_hal_timebase_tim.cyclo ./Core/Src/stm32l4xx_hal_timebase_tim.d ./Core/Src/stm32l4xx_hal_timebase_tim.o ./Core/Src/stm32l4xx_hal_timebase_tim.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/wifi.cyclo ./Core/Src/wifi.d ./Core/Src/wifi.o ./Core/Src/wifi.su

.PHONY: clean-Core-2f-Src

