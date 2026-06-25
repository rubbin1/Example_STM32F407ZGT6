cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Example_STM32F407ZGT6)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        Modules/Src/led.c
        Modules/Inc/led.h
        Modules/Inc/key_press.h
        Modules/Inc/soft_timer.h
        Modules/Src/key_press.c
        Modules/Src/soft_timer.c
        Modules/Src/buzzer.c
        Modules/Inc/buzzer.h
        Modules/Src/light_sensor.c
        Modules/Inc/light_sensor.h
        Modules/Src/touch_pad.c
        Modules/Inc/touch_pad.h
        Modules/Src/serial.c
        Modules/Inc/serial.h
        Modules/Src/adc_dev.c
        Modules/Inc/adc_dev.h
        Modules/Src/hw_timer.c
        Modules/Inc/hw_timer.h
        Modules/Src/oled.c
        Modules/Inc/oled.h
        Modules/Inc/oled_font.h
        Modules/Src/tft_lcd.c
        Modules/Inc/tft_lcd.h
        GUI/Scr/gui_button.c
        GUI/Inc/gui_button.h
        Modules/Src/servo_180.c
        Modules/Inc/servo_180.h
        Modules/Src/protocol.c
        Modules/Inc/protocol.h
        Modules/Src/w25qxx.c
        Modules/Inc/w25qxx.h
        Modules/Src/font_flash.c
        Modules/Inc/font_flash.h
        Modules/Inc/dht11.h
        Modules/Src/onewire.c
        Modules/Inc/i2c_bus.h
        Modules/Src/i2c_bus.c
        Modules/Src/ds18b20.c
        Modules/Inc/ds18b20.h
        Modules/Inc/onewire.h
        Modules/Src/dht11.c
        GUI/Scr/page_manager.c
        GUI/Inc/page_manager.h
        Functions/Scr/app_peripheral.c
        Functions/Inc/app_peripheral.h
        Functions/Scr/app_debug.c
        Functions/Inc/app_debug.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Modules/Inc
        GUI/Inc
    Functions/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
