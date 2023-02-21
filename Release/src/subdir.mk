################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main.cpp \
../src/malsa.cpp \
../src/tools.cpp 

CPP_DEPS += \
./src/main.d \
./src/malsa.d \
./src/tools.d 

OBJS += \
./src/main.o \
./src/malsa.o \
./src/tools.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/main.d ./src/main.o ./src/malsa.d ./src/malsa.o ./src/tools.d ./src/tools.o

.PHONY: clean-src

