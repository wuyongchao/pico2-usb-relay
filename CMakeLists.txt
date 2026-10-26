# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED USBRELAY_VERSION)
    set(USBRELAY_VERSION 0.1.0 CACHE STRING "Firmware version without a leading v")
endif()

if(NOT USBRELAY_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "USBRELAY_VERSION must use MAJOR.MINOR.PATCH format")
endif()

if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK")
endif()

if(PICO_SDK_PATH AND EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
elseif(PICO_SDK_PATH AND EXISTS "${PICO_SDK_PATH}/pico_sdk_init.cmake")
    include("${PICO_SDK_PATH}/pico_sdk_init.cmake")
else()
    message(FATAL_ERROR
        "Set PICO_SDK_PATH to a Raspberry Pi Pico SDK checkout, or pass "
        "-DPICO_SDK_PATH=/path/to/pico-sdk."
    )
endif()

if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Raspberry Pi Pico board target")
endif()

project(pico2_usb_relay VERSION ${USBRELAY_VERSION} LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

pico_sdk_init()

set(USBRELAY_CH1_GPIO 6 CACHE STRING "GPIO connected to relay channel 1 driver")
set(USBRELAY_ACTIVE_LOW ON CACHE BOOL "Logical relay ON drives GPIO low")
set(USBRELAY_RELAY_ID "PICO1" CACHE STRING "Five-character DCTTech-compatible relay ID")
set(USBRELAY_PERSIST_ID ON CACHE BOOL "Persist relay ID changes in the last flash sector")
set(USBRELAY_VID 16C0 CACHE STRING "USB vendor ID in hexadecimal, without 0x")
set(USBRELAY_PID 05DF CACHE STRING "USB product ID in hexadecimal, without 0x")
set(USBRELAY_MANUFACTURER "www.dcttech.com" CACHE STRING "USB manufacturer string")
set(USBRELAY_PRODUCT "USBRelay1" CACHE STRING "USB product string")

if(NOT USBRELAY_CH1_GPIO MATCHES "^([0-9]|[1-3][0-9]|4[0-7])$")
    message(FATAL_ERROR "USBRELAY_CH1_GPIO must be an RP2350 GPIO number from 0 to 47")
endif()

if(NOT USBRELAY_RELAY_ID MATCHES "^[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]$")
    message(FATAL_ERROR "USBRELAY_RELAY_ID must contain exactly five uppercase letters or digits")
endif()

if(NOT USBRELAY_VID MATCHES "^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$")
    message(FATAL_ERROR "USBRELAY_VID must be exactly four hexadecimal digits")
endif()

if(NOT USBRELAY_PID MATCHES "^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$")
    message(FATAL_ERROR "USBRELAY_PID must be exactly four hexadecimal digits")
endif()

if(USBRELAY_MANUFACTURER STREQUAL "" OR USBRELAY_PRODUCT STREQUAL "")
    message(FATAL_ERROR "USB manufacturer and product strings must not be empty")
endif()

add_library(relay_protocol STATIC src/relay_protocol.c)
target_include_directories(relay_protocol PUBLIC src)
target_compile_features(relay_protocol PUBLIC c_std_11)
target_compile_options(relay_protocol PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(pico2_usb_relay
    src/main.c
    src/usb_descriptors.c
)
target_include_directories(pico2_usb_relay PRIVATE src)
target_compile_features(pico2_usb_relay PRIVATE c_std_11)
target_compile_definitions(pico2_usb_relay PRIVATE
    PICO_FLASH_ASSUME_CORE0_SAFE=1
    USBRELAY_CH1_GPIO=${USBRELAY_CH1_GPIO}
    USBRELAY_ACTIVE_LOW=$<BOOL:${USBRELAY_ACTIVE_LOW}>
    USBRELAY_RELAY_ID="${USBRELAY_RELAY_ID}"
    USBRELAY_PERSIST_ID=$<BOOL:${USBRELAY_PERSIST_ID}>
    USBRELAY_VID=0x${USBRELAY_VID}
    USBRELAY_PID=0x${USBRELAY_PID}
    USBRELAY_MANUFACTURER="${USBRELAY_MANUFACTURER}"
    USBRELAY_PRODUCT="${USBRELAY_PRODUCT}"
    USBRELAY_VERSION="${PROJECT_VERSION}"
)
target_compile_options(pico2_usb_relay PRIVATE -Wall -Wextra -Werror)
target_link_libraries(pico2_usb_relay PRIVATE
    relay_protocol
    pico_stdlib
    pico_flash
    hardware_flash
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_uart(pico2_usb_relay 0)
pico_enable_stdio_usb(pico2_usb_relay 0)
pico_add_extra_outputs(pico2_usb_relay)
