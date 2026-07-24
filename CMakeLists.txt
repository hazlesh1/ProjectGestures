cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(ProjectGestures C CXX ASM)

pico_sdk_init()

if(NOT DEFINED HID_PROFILE)
    set(HID_PROFILE 1)
endif()

add_executable(ProjectGestures
        main.cpp
        drivers/vl53l0x_driver.c
        gesture/history.c
        gesture/gesture_logic.c
        hid/hid_output.c
        usb_descriptors.c
)

if(HID_PROFILE EQUAL 1)
    target_sources(ProjectGestures PRIVATE hid/hid_profile_media.c)
elseif(HID_PROFILE EQUAL 2)
    target_sources(ProjectGestures PRIVATE hid/hid_profile_paging.c)
endif()

target_compile_definitions(ProjectGestures PRIVATE
        HID_PROFILE=${HID_PROFILE}
)

pico_set_program_name(ProjectGestures "ProjectGestures")
pico_set_program_version(ProjectGestures "0.1")

pico_enable_stdio_uart(ProjectGestures 1)
pico_enable_stdio_usb(ProjectGestures 0)

target_compile_definitions(ProjectGestures PRIVATE
        CFG_TUSB_MCU=OPT_MCU_RP2040
)

target_link_libraries(ProjectGestures
        pico_stdlib
        hardware_i2c
        tinyusb_device
        tinyusb_board
)

target_include_directories(ProjectGestures PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/drivers
        ${CMAKE_CURRENT_LIST_DIR}/gesture
        ${CMAKE_CURRENT_LIST_DIR}/hid
        ${PICO_SDK_PATH}/lib/tinyusb/src
)

set_property(TARGET tinyusb_device APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_LIST_DIR}/hid
)

pico_add_extra_outputs(ProjectGestures)
