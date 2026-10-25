cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
elseif(EXISTS "/home/jack/pico-sdk/pico_sdk_init.cmake")
    set(PICO_SDK_PATH "/home/jack/pico-sdk")
else()
    message(FATAL_ERROR "PICO_SDK_PATH is not set")
endif()

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico2_usbrelay C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

pico_sdk_init()

set(USBRELAY_CH1_GPIO 6 CACHE STRING "GPIO connected to relay channel 1 driver")
set(USBRELAY_ACTIVE_LOW ON CACHE BOOL "Logical relay ON drives GPIO low")
set(USBRELAY_RELAY_ID "PICO1" CACHE STRING "Five-character DCTTech-compatible relay ID")
set(USBRELAY_PERSIST_ID ON CACHE BOOL "Persist relay ID changes in the last flash sector")
set(USBRELAY_DESCRIPTOR_STRICT ON CACHE BOOL "Match the original device descriptors exactly")

add_library(relay_protocol STATIC src/relay_protocol.c)
target_include_directories(relay_protocol PUBLIC src)
target_compile_options(relay_protocol PRIVATE -Wall -Wextra -Werror)

add_executable(pico2_usbrelay
    src/main.c
    src/usb_descriptors.c
)
target_include_directories(pico2_usbrelay PRIVATE src)
target_compile_definitions(pico2_usbrelay PRIVATE
    PICO_FLASH_ASSUME_CORE0_SAFE=1
    USBRELAY_CH1_GPIO=${USBRELAY_CH1_GPIO}
    USBRELAY_ACTIVE_LOW=$<BOOL:${USBRELAY_ACTIVE_LOW}>
    USBRELAY_RELAY_ID="${USBRELAY_RELAY_ID}"
    USBRELAY_PERSIST_ID=$<BOOL:${USBRELAY_PERSIST_ID}>
)
target_compile_options(pico2_usbrelay PRIVATE -Wall -Wextra -Werror)
target_link_libraries(pico2_usbrelay PRIVATE
    relay_protocol
    pico_stdlib
    pico_flash
    hardware_flash
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_uart(pico2_usbrelay 0)
pico_enable_stdio_usb(pico2_usbrelay 0)
pico_add_extra_outputs(pico2_usbrelay)

