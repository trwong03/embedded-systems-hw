################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/console_IO/consolemsg05.c \
../Core/Src/console_IO/mpaland_printf.c \
../Core/Src/console_IO/mpaland_printf_services.c 

OBJS += \
./Core/Src/console_IO/consolemsg05.o \
./Core/Src/console_IO/mpaland_printf.o \
./Core/Src/console_IO/mpaland_printf_services.o 

C_DEPS += \
./Core/Src/console_IO/consolemsg05.d \
./Core/Src/console_IO/mpaland_printf.d \
./Core/Src/console_IO/mpaland_printf_services.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/console_IO/%.o Core/Src/console_IO/%.su Core/Src/console_IO/%.cyclo: ../Core/Src/console_IO/%.c Core/Src/console_IO/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L475xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-console_IO

clean-Core-2f-Src-2f-console_IO:
	-$(RM) ./Core/Src/console_IO/consolemsg05.cyclo ./Core/Src/console_IO/consolemsg05.d ./Core/Src/console_IO/consolemsg05.o ./Core/Src/console_IO/consolemsg05.su ./Core/Src/console_IO/mpaland_printf.cyclo ./Core/Src/console_IO/mpaland_printf.d ./Core/Src/console_IO/mpaland_printf.o ./Core/Src/console_IO/mpaland_printf.su ./Core/Src/console_IO/mpaland_printf_services.cyclo ./Core/Src/console_IO/mpaland_printf_services.d ./Core/Src/console_IO/mpaland_printf_services.o ./Core/Src/console_IO/mpaland_printf_services.su

.PHONY: clean-Core-2f-Src-2f-console_IO

