################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.c \
../Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.c \
../Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.c 

OBJS += \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.o \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.o \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.o 

C_DEPS += \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.d \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.d \
./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/%.o Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/%.su Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/%.cyclo: ../Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/%.c Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L475xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master

clean-Core-2f-Src-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master:
	-$(RM) ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.cyclo ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.d ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.o ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi.su ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.cyclo ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.d ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.o ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/es_wifi_io.su ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.cyclo ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.d ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.o ./Core/Src/DISCO-WIFI-ISM43362-master/DISCO-WIFI-ISM43362-master/wifi.su

.PHONY: clean-Core-2f-Src-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master-2f-DISCO-2d-WIFI-2d-ISM43362-2d-master

