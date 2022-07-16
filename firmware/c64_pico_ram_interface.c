// vim: ts=4:sw=4:sts=4:et
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"
#ifdef LIB_PICO_CYW43_ARCH
    #include "pico/cyw43_arch.h"
#endif
#include "pico/stdlib.h"

#include "address_decoder.pio.h"
#include "command.pio.h"
#include "loader_rom.h"
#include "raspi.h"
#include "read.pio.h"

// Blink error codes
const uint ERR_ADD_DECODER_PROGRAM = 1;
const uint ERR_DECODER_PROGRAM_SM = 2;
const uint ERR_ADD_READ_PROGRAM = 3;
const uint ERR_READ_PROGRAM_SM = 4;
const uint ERR_ADD_COMMAND_PROGRAM = 5;
const uint ERR_COMMAND_PROGRAM_SM = 6;

// Pin assignments
// Note: PIO programs assume the pins are in this order!
const uint PIN_D0 = 0;
const uint PIN_D1 = 1;
const uint PIN_D2 = 2;
const uint PIN_D3 = 3;
const uint PIN_D4 = 4;
const uint PIN_D5 = 5;
const uint PIN_D6 = 6;
const uint PIN_D7 = 7;
const uint PIN_A0 = 8;
const uint PIN_A1 = 9;
const uint PIN_A2 = 10;
const uint PIN_A3 = 11;
const uint PIN_A4 = 12;
const uint PIN_A5 = 13;
const uint PIN_A6 = 14;
const uint PIN_A7 = 15;
const uint PIN_A8 = 16;
const uint PIN_A9 = 17;
const uint PIN_A10 = 18;
const uint PIN_A11 = 19;
const uint PIN_A12 = 20;
const uint PIN_A13 = 21;
const uint PIN_ROML = 22;
const uint PIN_ROMH = 26;
const uint PIN_IE = 27;
const uint PIN_OE = 28;

// PIO IRQ bits
const uint PIO_IRQ_ON_READ = 1;

// NUFLI offset in our ROM area
const uint NUFLI_OFFSET = 0x400;
const uint NUFLI_WINDOW_SIZE = 0x400;


typedef enum {
    CMD_NEXT_PAGE = 0x01,   // Advance the NUFLI window to the next 1K
    CMD_SLEEP = 0x02,       // Test a slow command that sleeps for 5 seconds
} command_t;

const uint BLINK_MS = 100;


void on_pio_irq();
void blink_and_set_clear_timer();
void read_dma_init(PIO pio, uint sm, char *base_address);
void errorblink(int code) __attribute__((noreturn));
static inline void init_output_pin(uint pin, bool value);
static inline bool led_init();
static inline void led_set(bool on);


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main() {
    bi_decl(bi_1pin_with_name(PIN_D0, "data output pin 0 (U3 pin 2)"))
    bi_decl(bi_1pin_with_name(PIN_D1, "data output pin 1 (U3 pin 4)"))
    bi_decl(bi_1pin_with_name(PIN_D2, "data output pin 2 (U3 pin 6)"))
    bi_decl(bi_1pin_with_name(PIN_D3, "data output pin 3 (U3 pin 8)"))
    bi_decl(bi_1pin_with_name(PIN_D4, "data output pin 4 (U3 pin 11)"))
    bi_decl(bi_1pin_with_name(PIN_D5, "data output pin 5 (U3 pin 13)"))
    bi_decl(bi_1pin_with_name(PIN_D6, "data output pin 6 (U3 pin 15)"))
    bi_decl(bi_1pin_with_name(PIN_D7, "data output pin 7 (U3 pin 17)"))
    bi_decl(bi_1pin_with_name(PIN_A0, "address input pin 0 (U2 pin 18)"))
    bi_decl(bi_1pin_with_name(PIN_A1, "address input pin 1 (U2 pin 16)"))
    bi_decl(bi_1pin_with_name(PIN_A2, "address input pin 2 (U2 pin 14)"))
    bi_decl(bi_1pin_with_name(PIN_A3, "address input pin 3 (U2 pin 12)"))
    bi_decl(bi_1pin_with_name(PIN_A4, "address input pin 4 (U2 pin 9)"))
    bi_decl(bi_1pin_with_name(PIN_A5, "address input pin 5 (U2 pin 7)"))
    bi_decl(bi_1pin_with_name(PIN_A6, "address input pin 6 (U2 pin 5)"))
    bi_decl(bi_1pin_with_name(PIN_A7, "address input pin 7 (U2 pin 3)"))
    bi_decl(bi_1pin_with_name(PIN_A8, "address input pin 8 (U1 pin 3)"))
    bi_decl(bi_1pin_with_name(PIN_A9, "address input pin 9 (U1 pin 5)"))
    bi_decl(bi_1pin_with_name(PIN_A10, "address input pin 10 (U1 pin 7)"))
    bi_decl(bi_1pin_with_name(PIN_A11, "address input pin 11 (U1 pin 9)"))
    bi_decl(bi_1pin_with_name(PIN_A12, "address input pin 12 (U1 pin 12)"))
    bi_decl(bi_1pin_with_name(PIN_A13, "address input pin 13 (U1 pin 14)"))
    bi_decl(bi_1pin_with_name(PIN_ROML, "ROML input pin (U1 pin 16)"))
    bi_decl(bi_1pin_with_name(PIN_ROMH, "ROMH input pin (U1 pin 18)"))
    bi_decl(bi_1pin_with_name(PIN_OE, "\"input enable\" output pin (U1 and U2, pins 1 and 19)"))
    bi_decl(bi_1pin_with_name(PIN_OE, "\"output enable\" output pin (U3 pins 1 and 19)"))

    // Start with I/O to the C64 disabled
    init_output_pin(PIN_OE, true);  // high = disabled
    init_output_pin(PIN_IE, true);  // high = disabled

    stdio_usb_init();
    printf("\n\n\n");
    printf("C64 pico ram interface %s\n", PICO_PROGRAM_VERSION_STRING);

    // Data exposed by the ROM window must be aligned by 16 kbytes so we can use the least
    // significant bits of its address for A0-A13
    const int rom_size = 16384;
    char *rom_data = memalign(rom_size, rom_size);

    // Initialize the ROM area with our loader ROM
    memcpy(rom_data, loader_rom, sizeof(loader_rom));

    // Initialize the NUFLI area of our ROM with the first 1KB
    int raspi_offset = 0;
    memcpy(rom_data + NUFLI_OFFSET, raspi, NUFLI_WINDOW_SIZE);

    PIO pio = pio0;

    // Address decoder: waits for ROMH/ROML line to be set low, and determines whether the address
    // to read is a command or a normal ROM read.  Each state machine monitors one ROM line.
    if(!pio_can_add_program(pio, &address_decoder_program)) {
        errorblink(ERR_ADD_DECODER_PROGRAM);
    }
    uint address_decoder_offset = pio_add_program(pio, &address_decoder_program);
    uint address_decoder_sm[2];
    const uint rom_pins[2] = {PIN_ROMH, PIN_ROML};
    for(int i = 0; i < 2; i++) {
        address_decoder_sm[i] = pio_claim_unused_sm(pio, true);
        if(address_decoder_sm[i] == -1) {
            errorblink(ERR_DECODER_PROGRAM_SM + i);
        }
        address_decoder_program_init(
                pio,
                address_decoder_sm[i],
                address_decoder_offset,
                PIN_A8,
                rom_pins[i],
                PIN_OE);
    }

    // Read handler: send rom_data value over D0..D7 when the C64 reads from ROML or ROMH
    if(!pio_can_add_program(pio, &read_program)) {
        errorblink(ERR_ADD_READ_PROGRAM);
    }
    uint read_offset = pio_add_program(pio, &read_program);
    uint read_sm = pio_claim_unused_sm(pio, true);
    if(read_sm == -1) {
        errorblink(ERR_READ_PROGRAM_SM);
    }
    read_program_init(
            pio,
            read_sm,
            read_offset,
            PIN_D0,
            PIN_A0,
            PIN_OE,
            rom_data);
    read_dma_init(pio, read_sm, rom_data);

    // Command handler: put command NN on the FIFO when the CPU reads from address $BFNN
    if(!pio_can_add_program(pio, &command_program)) {
        errorblink(ERR_ADD_COMMAND_PROGRAM);
    }
    uint command_offset = pio_add_program(pio, &command_program);
    uint command_sm = pio_claim_unused_sm(pio, true);
    if(command_sm == -1) {
        errorblink(ERR_COMMAND_PROGRAM_SM);
    }
    command_program_init(
            pio,
            command_sm,
            command_offset,
            PIN_A0,
            PIN_D0,
            PIN_OE);


    // Set up LED (and wi-fi if available)
    if(!led_init()) {
        printf("Failed to initialize CYW43\n");
        while(true);  // Can't blink an error code...
    }

    // Install IRQ handler for blinkenlights on read
    // Per the RP2040 datasheet (PIO: IRQ0_INTE Register, p. 399)
    // state machine enable flags start at bit 8
    pio->inte0 = 1 << (8 + address_decoder_sm[0]);
    irq_set_exclusive_handler(PIO0_IRQ_0, on_pio_irq);
    irq_set_enabled(PIO0_IRQ_0, true);

    // READY!  Open the floodgates!
    gpio_put(PIN_IE, false);  // low = enabled

    printf("Address decoder ROMH sm: %d\n", address_decoder_sm[0]);
    printf("Address decoder ROML sm: %d\n", address_decoder_sm[1]);
    printf("Read sm: %d\n", read_sm);
    printf("Command sm: %d\n", command_sm);
    printf("Pico RAM window start: 0x%08X\n", (uint)rom_data);
    printf("First 8 bytes of ROM: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           rom_data[0], rom_data[1], rom_data[2], rom_data[3], rom_data[4], rom_data[5],
           rom_data[6], rom_data[7]);
    printf("First 8 bytes of NUFLI: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           ((char *)(rom_data + NUFLI_OFFSET))[0], ((char *)(rom_data + NUFLI_OFFSET))[1],
           ((char *)(rom_data + NUFLI_OFFSET))[2], ((char *)(rom_data + NUFLI_OFFSET))[3],
           ((char *)(rom_data + NUFLI_OFFSET))[4], ((char *)(rom_data + NUFLI_OFFSET))[5],
           ((char *)(rom_data + NUFLI_OFFSET))[6], ((char *)(rom_data + NUFLI_OFFSET))[7]);
    printf("ROM address: $8000\n");
    printf("Command address prefix: $%04X\n", 0x8000 + (address_decoder_COMMAND_PREFIX << 8));

    const char spinner[] = {'|', '/', '-', '\\'};
    while(true) {
	    pio_sm_put(pio, command_sm, 1);  // tell the command program we're ready
        printf("\nReady for command %c", spinner[0]);

        uint64_t last_hb = time_us_64();
        uint spinner_pos = 0;
        do {
            uint64_t now = time_us_64();
            if(now - last_hb >= 1000000 / sizeof(spinner)) {
                printf("\b%c", spinner[spinner_pos]);
                spinner_pos++;
                if(spinner_pos == sizeof(spinner)) {
                    spinner_pos = 0;
                }
                last_hb = now;
            }
        } while(pio_sm_is_rx_fifo_empty(pio, command_sm));
        printf("\b \n");

        uint command = pio_sm_get_blocking(pio, command_sm);
        printf("Got command %08X\n", command);

        switch(command) {
            case CMD_NEXT_PAGE:
                raspi_offset += NUFLI_WINDOW_SIZE;
                if(raspi_offset >= sizeof(raspi)) {
                    raspi_offset = 0;
                }
                printf("NUFLI start is now %02X\n", (uint)raspi_offset);
                memcpy(rom_data + NUFLI_OFFSET, raspi + raspi_offset, NUFLI_WINDOW_SIZE);
                printf("First 8 bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n", ((char *)(rom_data + NUFLI_OFFSET))[0], ((char *)(rom_data + NUFLI_OFFSET))[1], ((char *)(rom_data + NUFLI_OFFSET))[2], ((char *)(rom_data + NUFLI_OFFSET))[3], ((char *)(rom_data + NUFLI_OFFSET))[4], ((char *)(rom_data + NUFLI_OFFSET))[5], ((char *)(rom_data + NUFLI_OFFSET))[6], ((char *)(rom_data + NUFLI_OFFSET))[7]);
                break;

        	case CMD_SLEEP:
        		printf("Sleeping\n");
		        sleep_ms(5000);
		        printf("Done\n");
		        break;
        }
    }
}

#pragma clang diagnostic pop


// Set up DMA channels for handling reads
void read_dma_init(PIO pio, uint sm, char *base_address) {
    // Read channel: copy requested byte to TX fifo (source address set by write channel below)
    uint read_channel = 0;
    dma_channel_claim(read_channel);

    dma_channel_config read_config = dma_channel_get_default_config(read_channel);
    channel_config_set_read_increment(&read_config, false);
    channel_config_set_write_increment(&read_config, false);
    channel_config_set_dreq(&read_config, pio_get_dreq(pio, sm, true));
    channel_config_set_transfer_data_size(&read_config, DMA_SIZE_8);

    dma_channel_configure(read_channel,
                          &read_config,
                          &pio->txf[sm], // write to TX fifo
                          base_address,  // read from base address (overwritten by write channel)
                          1,             // transfer count
                          false);        // start later

    // Write channel: copy address from RX fifo to the read channel's READ_ADDR_TRIGGER
    uint write_channel = 1;
    dma_channel_claim(write_channel);
    dma_channel_config write_config = dma_channel_get_default_config(write_channel);
    channel_config_set_read_increment(&write_config, false);
    channel_config_set_write_increment(&write_config, false);
    channel_config_set_dreq(&write_config, pio_get_dreq(pio, sm, false));
    channel_config_set_transfer_data_size(&write_config, DMA_SIZE_32);

    volatile void *read_channel_addr = &dma_channel_hw_addr(read_channel)->al3_read_addr_trig;
    dma_channel_configure(write_channel,
                          &write_config,
                          read_channel_addr, // write to read_channel READ_ADDR_TRIGGER
                          &pio->rxf[sm],     // read from RX fifo
                          0xffffffff,        // do many transfers
                          true);             // start now
}

// LED helpers
static inline bool led_init() {
    #ifdef PICO_DEFAULT_LED_PIN
    // Set up blinkenlight pin
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        return true;
    #elif defined LIB_PICO_CYW43_ARCH
        // Set up CYW43 (Pico W wi-fi and LED)
        int result = cyw43_arch_init();
        if(result != PICO_OK) {
            return false;
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        return true;
    #endif
}

static inline void led_set(bool on) {
    #ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, on);
    #elif defined LIB_PICO_CYW43_ARCH
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
    #endif
}

// Timer for on_clear
alarm_id_t clear_read_alarm = -1;

// Clear the LED after it's blunk
int64_t on_clear_led(alarm_id_t alarm_id, void *user_data) {
    led_set(false);
    clear_read_alarm = -1;
    return 0;
}

// On IRQ0 (command received), blink the LED
void on_pio_irq() {
    PIO pio = pio0;
    if(pio->irq & PIO_IRQ_ON_READ) {
        hw_clear_bits(&pio->irq, PIO_IRQ_ON_READ);
        blink_and_set_clear_timer();
    }
}

void blink_and_set_clear_timer() {
    // Turn on the LED
    led_set(true);

    // If we already had a timer to turn it off, cancel it
    if(clear_read_alarm > 0) {
        cancel_alarm(clear_read_alarm);
    }

    // Turn off the LED later
    clear_read_alarm = add_alarm_in_ms(BLINK_MS, on_clear_led, NULL, true);
}

// Repeat a series of blinks forever
void errorblink(int code) {
    led_init();
    while(true) {
        for(int i = 0; i < code; i++) {
            led_set(true);
            sleep_ms(333);
            led_set(false);
            sleep_ms(333);
        }
        sleep_ms(667);
    }
}

// Initialize an output GPIO pin and set its value
void init_output_pin(uint pin, bool value) {
    gpio_init(pin);
    gpio_set_dir(pin, true);  // output direction
    gpio_put(pin, value);
}
