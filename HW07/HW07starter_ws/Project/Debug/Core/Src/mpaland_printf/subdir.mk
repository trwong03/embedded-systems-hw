################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/mpaland_printf/console01.c \
../Core/Src/mpaland_printf/mpaland_printf.c \
../Core/Src/mpaland_printf/mpaland_printf_services.c 

S_UPPER_SRCS += \
../Core/Src/mpaland_printf/snprintf_from_asm.S 

OBJS += \
./Core/Src/mpaland_printf/console01.o \
./Core/Src/mpaland_printf/mpaland_printf.o \
./Core/Src/mpaland_printf/mpaland_printf_services.o \
./Core/Src/mpaland_printf/snprintf_from_asm.o 

S_UPPER_DEPS += \
./Core/Src/mpaland_printf/snprintf_from_asm.d 

C_DEPS += \
./Core/Src/mpaland_printf/console01.d \
./Core/Src/mpaland_printf/mpaland_printf.d \
./Core/Src/mpaland_printf/mpaland_printf_services.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/mpaland_printf/%.o Core/Src/mpaland_printf/%.su Core/Src/mpaland_printf/%.cyclo: ../Core/Src/mpaland_printf/%.c Core/Src/mpaland_printf/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L475xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/mpaland_printf/%.o: ../Core/Src/mpaland_printf/%.S Core/Src/mpaland_printf/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-Src-2f-mpaland_printf

clean-Core-2f-Src-2f-mpaland_printf:
	-$(RM) ./Core/Src/mpaland_printf/console01.cyclo ./Core/Src/mpaland_printf/console01.d ./Core/Src/mpaland_printf/console01.o ./Core/Src/mpaland_printf/console01.su ./Core/Src/mpaland_printf/mpaland_printf.cyclo ./Core/Src/mpaland_printf/mpaland_printf.d ./Core/Src/mpaland_printf/mpaland_printf.o ./Core/Src/mpaland_printf/mpaland_printf.su ./Core/Src/mpaland_printf/mpaland_printf_services.cyclo ./Core/Src/mpaland_printf/mpaland_printf_services.d ./Core/Src/mpaland_printf/mpaland_printf_services.o ./Core/Src/mpaland_printf/mpaland_printf_services.su ./Core/Src/mpaland_printf/snprintf_from_asm.d ./Core/Src/mpaland_printf/snprintf_from_asm.o

.PHONY: clean-Core-2f-Src-2f-mpaland_printf

