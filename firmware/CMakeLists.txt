cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(c64_pico_ram_interface C CXX ASM)
set(PICO_BOARD pico_w)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(c64_pico_ram_interface
    c64_pico_ram_interface.c
    exposed_ram.c
    wifi.c
    ../c64-rom/loader_rom.c
    ../c64-rom/raspi.c
)

include_directories(../c64-rom PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_set_program_name(c64_pico_ram_interface "c64 pico ram interface")
pico_set_program_description(c64_pico_ram_interface "expose pico ram as a c64 rom")
pico_set_program_version(c64_pico_ram_interface "3.0")

pico_generate_pio_header(c64_pico_ram_interface ${CMAKE_CURRENT_LIST_DIR}/address_decoder.pio)
pico_generate_pio_header(c64_pico_ram_interface ${CMAKE_CURRENT_LIST_DIR}/command.pio)
pico_generate_pio_header(c64_pico_ram_interface ${CMAKE_CURRENT_LIST_DIR}/read.pio)

pico_enable_stdio_usb(c64_pico_ram_interface 1)

pico_add_extra_outputs(c64_pico_ram_interface)

target_link_libraries(c64_pico_ram_interface
    hardware_dma
    hardware_pio
    pico_cyw43_arch_lwip_poll
    pico_stdlib
)
