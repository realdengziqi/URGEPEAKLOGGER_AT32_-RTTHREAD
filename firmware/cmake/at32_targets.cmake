set(SURGE_FW_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
get_filename_component(SURGE_FW_DIR ${SURGE_FW_DIR} ABSOLUTE)
get_filename_component(SURGE_ROOT_DIR ${SURGE_FW_DIR}/.. ABSOLUTE)

set(SURGE_AT32_VENDOR_DIR ${SURGE_FW_DIR}/vendor/AT32F403A_407_Firmware_Library_V2.2.2)
set(SURGE_RTTHREAD_DIR ${SURGE_FW_DIR}/vendor/rt-thread-v5.2.2)
set(SURGE_FREEMODBUS_DIR ${SURGE_FW_DIR}/vendor/freemodbus)
set(SURGE_FLASHDB_DIR ${SURGE_FW_DIR}/vendor/flashdb)
set(SURGE_WIZNET_DIR ${SURGE_FW_DIR}/vendor/wiznet_iolibrary)

set(SURGE_COMMON_DEFINES
    AT32F403ARGT7
    USE_STDPERIPH_DRIVER
    __RTTHREAD__
    __RT_KERNEL_SOURCE__
    _WIZCHIP_=W5500
)

set(SURGE_COMMON_INCLUDE_DIRS
    ${SURGE_FW_DIR}/app/inc
    ${SURGE_FW_DIR}/bsp/at32f403/inc
    ${SURGE_FW_DIR}/drivers/screen/inc
    ${SURGE_FW_DIR}/drivers/net/inc
    ${SURGE_FW_DIR}/drivers/sensor/inc
    ${SURGE_FW_DIR}/drivers/storage/inc
    ${SURGE_FW_DIR}/external_port/flashdb_port
    ${SURGE_FLASHDB_DIR}/inc
    ${SURGE_FLASHDB_DIR}/port/fal/inc
    ${SURGE_AT32_VENDOR_DIR}/libraries/drivers/inc
    ${SURGE_AT32_VENDOR_DIR}/libraries/cmsis/cm4/core_support
    ${SURGE_AT32_VENDOR_DIR}/libraries/cmsis/cm4/device_support
    ${SURGE_RTTHREAD_DIR}/include
    ${SURGE_RTTHREAD_DIR}/include/klibc
    ${SURGE_RTTHREAD_DIR}/libcpu/arm/cortex-m4
    ${SURGE_RTTHREAD_DIR}/components/finsh
    ${SURGE_FW_DIR}/external_port/freemodbus_port
    ${SURGE_FREEMODBUS_DIR}/modbus/include
    ${SURGE_FREEMODBUS_DIR}/modbus/rtu
    ${SURGE_FREEMODBUS_DIR}/modbus/ascii
    ${SURGE_FREEMODBUS_DIR}/modbus/tcp
    ${SURGE_WIZNET_DIR}/Ethernet
    ${SURGE_WIZNET_DIR}/Ethernet/W5500
    ${SURGE_WIZNET_DIR}/Internet/DHCP
    ${SURGE_FW_DIR}/vendor/ugui
    ${SURGE_FW_DIR}/external_port/ugui_port
    ${SURGE_FW_DIR}/external_port/surge_ui_kit
)

set(SURGE_APP_SOURCES
    ${SURGE_FW_DIR}/app/src/main.c
    ${SURGE_FW_DIR}/app/src/app_main.c
    ${SURGE_FW_DIR}/app/src/app_backend_request.c
    ${SURGE_FW_DIR}/app/src/app_alarm_service.c
    ${SURGE_FW_DIR}/app/src/app_buzzer_service.c
    ${SURGE_FW_DIR}/app/src/app_data_service.c
    ${SURGE_FW_DIR}/app/src/app_factory_service.c
    ${SURGE_FW_DIR}/app/src/app_param_service.c
    ${SURGE_FW_DIR}/app/src/app_record_service.c
    ${SURGE_FW_DIR}/app/src/app_surge_service.c
    ${SURGE_FW_DIR}/app/src/app_modbus_map.c
    ${SURGE_FW_DIR}/app/src/app_modbus_rtu.c
    ${SURGE_FW_DIR}/app/src/app_modbus_tcp.c
    ${SURGE_FW_DIR}/app/src/app_shell.c
    ${SURGE_FW_DIR}/app/src/app_rtc.c
    ${SURGE_FW_DIR}/app/src/app_bringup.c
    ${SURGE_FW_DIR}/app/src/app_key_input.c
)

set(SURGE_BSP_SOURCES
    ${SURGE_FW_DIR}/bsp/at32f403/src/board.c
    ${SURGE_FW_DIR}/bsp/at32f403/src/at32f403a_407_clock.c
    ${SURGE_FW_DIR}/bsp/at32f403/src/at32f403_int.c
    ${SURGE_FW_DIR}/bsp/at32f403/src/gcc_newlib_stubs.c
)

set(SURGE_DRIVER_SOURCES
    ${SURGE_FW_DIR}/drivers/sensor/src/sensor_adc_raw.c
    ${SURGE_FW_DIR}/drivers/sensor/src/sensor_digital_input.c
    ${SURGE_FW_DIR}/drivers/sensor/src/sensor_hdc1080.c
    ${SURGE_FW_DIR}/drivers/net/src/net_w5500.c
    ${SURGE_FW_DIR}/drivers/storage/src/storage_at24c128.c
    ${SURGE_FW_DIR}/external_port/flashdb_port/fal_flash_at24c128.c
    ${SURGE_FW_DIR}/external_port/flashdb_port/fal_flash_onchip.c
    ${SURGE_FW_DIR}/external_port/flashdb_port/storage_flashdb.c
)

set(SURGE_SCREEN_SOURCES
    ${SURGE_FW_DIR}/drivers/screen/src/screen_st7789.c
)

file(GLOB SURGE_AT32_DRIVER_SOURCES
    ${SURGE_AT32_VENDOR_DIR}/libraries/drivers/src/*.c
)

set(SURGE_CMSIS_SOURCES
    ${SURGE_AT32_VENDOR_DIR}/libraries/cmsis/cm4/device_support/system_at32f403a_407.c
    ${SURGE_AT32_VENDOR_DIR}/libraries/cmsis/cm4/device_support/startup/gcc/startup_at32f403a_407.s
)

set(SURGE_RTTHREAD_SOURCES
    ${SURGE_RTTHREAD_DIR}/src/clock.c
    ${SURGE_RTTHREAD_DIR}/src/components.c
    ${SURGE_RTTHREAD_DIR}/src/cpu_up.c
    ${SURGE_RTTHREAD_DIR}/src/defunct.c
    ${SURGE_RTTHREAD_DIR}/src/idle.c
    ${SURGE_RTTHREAD_DIR}/src/ipc.c
    ${SURGE_RTTHREAD_DIR}/src/irq.c
    ${SURGE_RTTHREAD_DIR}/src/kservice.c
    ${SURGE_RTTHREAD_DIR}/src/mem.c
    ${SURGE_RTTHREAD_DIR}/src/object.c
    ${SURGE_RTTHREAD_DIR}/src/scheduler_comm.c
    ${SURGE_RTTHREAD_DIR}/src/scheduler_up.c
    ${SURGE_RTTHREAD_DIR}/src/thread.c
    ${SURGE_RTTHREAD_DIR}/src/timer.c
    ${SURGE_RTTHREAD_DIR}/src/klibc/kstring.c
    ${SURGE_RTTHREAD_DIR}/src/klibc/kstdio.c
    ${SURGE_RTTHREAD_DIR}/src/klibc/kerrno.c
    ${SURGE_RTTHREAD_DIR}/src/klibc/rt_vsnprintf_tiny.c
    ${SURGE_RTTHREAD_DIR}/src/klibc/rt_vsscanf.c
    ${SURGE_RTTHREAD_DIR}/libcpu/arm/cortex-m4/cpuport.c
    ${SURGE_RTTHREAD_DIR}/libcpu/arm/cortex-m4/context_gcc.S
    ${SURGE_RTTHREAD_DIR}/components/finsh/shell.c
    ${SURGE_RTTHREAD_DIR}/components/finsh/msh.c
)

set(SURGE_UI_SOURCES
    ${SURGE_FW_DIR}/app/src/app_ui.c
    ${SURGE_FW_DIR}/vendor/ugui/ugui.c
    ${SURGE_FW_DIR}/external_port/ugui_port/ugui_port.c
    ${SURGE_FW_DIR}/external_port/surge_ui_kit/surge_ui_widgets.c
    ${SURGE_FW_DIR}/external_port/surge_ui_kit/surge_ui_font_zh_18.c
    ${SURGE_FW_DIR}/external_port/surge_ui_kit/surge_ui_font_zh_20.c
    ${SURGE_FW_DIR}/external_port/surge_ui_kit/surge_ui_font_zh_24.c
)

set(SURGE_FREEMODBUS_SOURCES
    ${SURGE_FREEMODBUS_DIR}/modbus/mb.c
    ${SURGE_FREEMODBUS_DIR}/modbus/rtu/mbrtu.c
    ${SURGE_FREEMODBUS_DIR}/modbus/rtu/mbcrc.c
    ${SURGE_FREEMODBUS_DIR}/modbus/functions/mbfuncdiag.c
    ${SURGE_FREEMODBUS_DIR}/modbus/functions/mbutils.c
    ${SURGE_FREEMODBUS_DIR}/modbus/functions/mbfuncother.c
    ${SURGE_FREEMODBUS_DIR}/modbus/functions/mbfuncholding.c
    ${SURGE_FREEMODBUS_DIR}/modbus/functions/mbfuncinput.c
    ${SURGE_FW_DIR}/external_port/freemodbus_port/port.c
    ${SURGE_FW_DIR}/external_port/freemodbus_port/portevent.c
    ${SURGE_FW_DIR}/external_port/freemodbus_port/portserial.c
    ${SURGE_FW_DIR}/external_port/freemodbus_port/porttimer.c
)

set(SURGE_FLASHDB_SOURCES
    ${SURGE_FLASHDB_DIR}/src/fdb.c
    ${SURGE_FLASHDB_DIR}/src/fdb_kvdb.c
    ${SURGE_FLASHDB_DIR}/src/fdb_tsdb.c
    ${SURGE_FLASHDB_DIR}/src/fdb_utils.c
    ${SURGE_FLASHDB_DIR}/port/fal/src/fal.c
    ${SURGE_FLASHDB_DIR}/port/fal/src/fal_flash.c
    ${SURGE_FLASHDB_DIR}/port/fal/src/fal_partition.c
)

set(SURGE_WIZNET_SOURCES
    ${SURGE_WIZNET_DIR}/Ethernet/wizchip_conf.c
    ${SURGE_WIZNET_DIR}/Ethernet/socket.c
    ${SURGE_WIZNET_DIR}/Ethernet/W5500/w5500.c
    ${SURGE_WIZNET_DIR}/Internet/DHCP/dhcp.c
)

set(SURGE_MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=soft
)

function(surge_add_firmware_target target_name)
    cmake_parse_arguments(SURGE_TARGET "WITH_UI" "" "" ${ARGN})

    add_executable(${target_name}
        ${SURGE_APP_SOURCES}
        ${SURGE_BSP_SOURCES}
        ${SURGE_DRIVER_SOURCES}
        ${SURGE_AT32_DRIVER_SOURCES}
        ${SURGE_CMSIS_SOURCES}
        ${SURGE_RTTHREAD_SOURCES}
        ${SURGE_FREEMODBUS_SOURCES}
        ${SURGE_FLASHDB_SOURCES}
        ${SURGE_WIZNET_SOURCES}
    )

    target_compile_definitions(${target_name} PRIVATE ${SURGE_COMMON_DEFINES})
    target_include_directories(${target_name} PRIVATE ${SURGE_COMMON_INCLUDE_DIRS})

    if(SURGE_TARGET_WITH_UI)
        target_sources(${target_name} PRIVATE
            ${SURGE_SCREEN_SOURCES}
            ${SURGE_UI_SOURCES}
        )
        target_compile_definitions(${target_name} PRIVATE
            SURGE_ENABLE_UI=1
            SURGE_UI_USE_RTTHREAD
        )
    endif()

    target_compile_options(${target_name} PRIVATE
        ${SURGE_MCU_FLAGS}
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-Og -g3>
        $<$<CONFIG:Release>:-Os -g0>
    )

    target_link_options(${target_name} PRIVATE
        ${SURGE_MCU_FLAGS}
        -T${SURGE_FW_DIR}/linker/AT32F403ARGT7.ld
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target_name}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,-e,entry
        -nostartfiles
        --specs=nano.specs
        --specs=nosys.specs
    )

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target_name}> ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target_name}>
        BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.hex
            ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.bin
        VERBATIM
    )
endfunction()
