#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "ws2812.pio.h"
#include "bsp/board.h"
#include "tusb.h"

// This code allows to use the Rasperry Pi PICO as PicoPiDevice.

#define VERSION 1

//#### CONFIGURATION ####

// The default amount of LED of each channel. This can be configured afterwards through USB.
// Setting a channel to 0 LEDs will disable it.
#define DEFAULT_LED_COUNT_CHANNEL_1 255
#define DEFAULT_LED_COUNT_CHANNEL_2 255
#define DEFAULT_LED_COUNT_CHANNEL_3 255
#define DEFAULT_LED_COUNT_CHANNEL_4 255
#define DEFAULT_LED_COUNT_CHANNEL_5 255
#define DEFAULT_LED_COUNT_CHANNEL_6 255
#define DEFAULT_LED_COUNT_CHANNEL_7 255
#define DEFAULT_LED_COUNT_CHANNEL_8 255

#define PIN_CHANNEL_1 8
#define PIN_CHANNEL_2 9
#define PIN_CHANNEL_3 10
#define PIN_CHANNEL_4 11
#define PIN_CHANNEL_5 12
#define PIN_CHANNEL_6 13
#define PIN_CHANNEL_7 14
#define PIN_CHANNEL_8 15

//#######################

#define CHANNELS 8 // Change this only if you add or remove channels in the implementation-part. To disable channels set them to 0 LED.
#define MAX_CHANNEL_SIZE 255

#define BUFFER_SIZE (MAX_CHANNEL_SIZE * 3)
#define TRANSFER_BUFFER_SIZE ((BUFFER_SIZE + 3) * CHANNELS)

#define OFFSET_MULTIPLIER 60

#define FLASH_CONFIG_OFFSET (256 * 1024)
#define CONFIG_MAGIC_NUMBER_LENGTH 8

const PIO CHANNEL_PIO[CHANNELS] = {pio0, pio0, pio0, pio0, pio1, pio1, pio1, pio1};
const uint CHANNEL_SM[CHANNELS] = {0, 1, 2, 3, 0, 1, 2, 3};

uint8_t pins[CHANNELS] = {PIN_CHANNEL_1, PIN_CHANNEL_2, PIN_CHANNEL_3, PIN_CHANNEL_4, PIN_CHANNEL_5, PIN_CHANNEL_6, PIN_CHANNEL_7, PIN_CHANNEL_8};
uint8_t led_counts[CHANNELS] = {DEFAULT_LED_COUNT_CHANNEL_1, DEFAULT_LED_COUNT_CHANNEL_2, DEFAULT_LED_COUNT_CHANNEL_3, DEFAULT_LED_COUNT_CHANNEL_4, DEFAULT_LED_COUNT_CHANNEL_5, DEFAULT_LED_COUNT_CHANNEL_6, DEFAULT_LED_COUNT_CHANNEL_7, DEFAULT_LED_COUNT_CHANNEL_8};

uint8_t buffer_channel_1[BUFFER_SIZE];
uint8_t buffer_channel_2[BUFFER_SIZE];
uint8_t buffer_channel_3[BUFFER_SIZE];
uint8_t buffer_channel_4[BUFFER_SIZE];
uint8_t buffer_channel_5[BUFFER_SIZE];
uint8_t buffer_channel_6[BUFFER_SIZE];
uint8_t buffer_channel_7[BUFFER_SIZE];
uint8_t buffer_channel_8[BUFFER_SIZE];

uint8_t *buffers[] = {(uint8_t *)buffer_channel_1, (uint8_t *)buffer_channel_2, (uint8_t *)buffer_channel_3, (uint8_t *)buffer_channel_4, (uint8_t *)buffer_channel_5, (uint8_t *)buffer_channel_6, (uint8_t *)buffer_channel_7, (uint8_t *)buffer_channel_8};
uint8_t send_buffer[CFG_TUD_ENDPOINT0_SIZE];
uint8_t read_buffer[CFG_TUD_ENDPOINT0_SIZE];
uint8_t transfer_buffer[TRANSFER_BUFFER_SIZE];
uint32_t transfer_buffer_count = 0;
uint32_t transfer_length = 0;

const uint8_t config_magic_number[CONFIG_MAGIC_NUMBER_LENGTH] = {0x52, 0x47, 0x42, 0x2E, 0x4E, 0x45, 0x54, VERSION};
const uint8_t *config = (const uint8_t *)(XIP_BASE + FLASH_CONFIG_OFFSET);

static inline void put_pixel(PIO pio, int sm, uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | ((uint32_t)(b));
}

void init_channel(int channel)
{
    PIO pio = CHANNEL_PIO[channel];
    uint sm = CHANNEL_SM[channel];
    uint8_t pin = pins[channel];

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
}

void update_channel(int channel)
{
    PIO pio = CHANNEL_PIO[channel];
    uint sm = CHANNEL_SM[channel];
    uint8_t *data = buffers[channel];
    int count = led_counts[channel];

    for (int i = 0; i < count; i++)
    {
        int offset = i * 3;
        uint8_t r = data[offset];
        uint8_t g = data[offset + 1];
        uint8_t b = data[offset + 2];
        uint32_t pixel = urgb_u32(r, g, b);
        put_pixel(pio, sm, pixel);
    }
}

void stage_channel_update(uint8_t channel, uint8_t const *buffer, uint16_t length)
{
    uint8_t *data = buffers[channel];
    bool update = buffer[0] > 0;
    int offset = buffer[1] * OFFSET_MULTIPLIER;
    int dataLength = length - 2;
    int bufferSize = led_counts[channel] * 3;
    if ((offset + dataLength) > bufferSize)
    {
        dataLength = bufferSize - offset;
        if (dataLength <= 0)
        {
            return;
        }
    }
    multicore_fifo_pop_blocking();
    __builtin_memcpy(data + offset, buffer + 2, dataLength);
    if (update)
    {
        multicore_fifo_push_blocking(channel);
    }
    else
    {
        multicore_fifo_push_blocking(0xFF);
    }
}

void stage_channel_update_bulk(uint8_t channel, uint8_t const *buffer, uint16_t length)
{
    uint8_t *data = buffers[channel];

    multicore_fifo_pop_blocking();
    __builtin_memcpy(data, buffer, length);
    multicore_fifo_push_blocking(channel);
}

void send_info(uint8_t info)
{
    __builtin_memset(send_buffer, 0, sizeof(send_buffer));
    send_buffer[0] = info;
    tud_hid_report(0, send_buffer, 64);
}

void send_info_n(uint8_t const *data, uint16_t length)
{
    __builtin_memset(send_buffer, 0, sizeof(send_buffer));
    for (uint16_t i = 0; i < length; i++)
    {
        send_buffer[i] = data[i];
    }
    tud_hid_report(0, send_buffer, 64);
}

void write_configuration(uint8_t const *ledCounts, uint8_t const *pins, uint16_t length)
{
    if (length < CHANNELS)
    {
        return;
    }

    uint8_t config_buffer[FLASH_PAGE_SIZE];
    __builtin_memset(config_buffer, 0, FLASH_PAGE_SIZE);
    __builtin_memcpy(config_buffer, config_magic_number, CONFIG_MAGIC_NUMBER_LENGTH);

    for (int i = 0; i < CHANNELS; i++)
    {
        uint8_t ledCount = ledCounts[i];
        if (ledCount > MAX_CHANNEL_SIZE)
        {
            ledCount = MAX_CHANNEL_SIZE;
        }
        config_buffer[CONFIG_MAGIC_NUMBER_LENGTH + i] = ledCount;
        config_buffer[CONFIG_MAGIC_NUMBER_LENGTH + CHANNELS + i] = pins[i];
    }

    flash_range_erase(FLASH_CONFIG_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_CONFIG_OFFSET, config_buffer, FLASH_PAGE_SIZE);

    watchdog_reboot(0, SRAM_END, 100);
}

void load_configuration()
{
    for (int i = 0; i < CONFIG_MAGIC_NUMBER_LENGTH; i++)
    {
        if (config[i] != config_magic_number[i])
        {
            write_configuration(led_counts, pins, CHANNELS);
            break;
        }
    }

    for (int i = 0; i < CHANNELS; i++)
    {
        led_counts[i] = config[CONFIG_MAGIC_NUMBER_LENGTH + i];
        pins[i] = config[CONFIG_MAGIC_NUMBER_LENGTH + CHANNELS + i];
    }
}

void process_command(uint8_t command, uint8_t const *data, uint16_t length)
{
    int request = command & 0x0F;
    int channel = (command >> 4) & 0x0F;

    if (channel == 0) // Device-commands
    {
        if (request == 0x01)
        {
            send_info(CHANNELS);
        }
        else if (request == 0x0A)
        {
            for (int i = 0; i < CHANNELS; i++)
            {
                led_counts[i] = data[i];
            }
            
            write_configuration(data, pins, length);
        }
        else if (request == 0x0B)
        {
            for (int i = 0; i < CHANNELS; i++)
            {
                pins[i] = data[i];
            }

            write_configuration(led_counts, data, length);
        }
        else if (request == 0x0E)
        {
            uint8_t id[8];
            flash_get_unique_id(id);
            send_info_n(id, sizeof(id));
        }
        else if (request == 0x0F)
        {
            send_info(VERSION);
        }
    }
    else if (channel <= CHANNELS)
    {
        channel--;
        if (request == 0x01)
        {
            stage_channel_update(channel, data, length);
        }
        else if (request == 0x02)
        {
            stage_channel_update_bulk(channel, data, length);
        }
        else if (request == 0x0A)
        {
            send_info(led_counts[channel]);
        }
        else if (request == 0x0B)
        {
            send_info(pins[channel]);
        }
    }
}

void process_transfer_buffer()
{
    uint32_t offset = 0;
    while (offset < transfer_buffer_count)
    {
        uint8_t command = transfer_buffer[offset++];
        uint8_t payload_length = ((uint16_t)transfer_buffer[offset++] << 8) | transfer_buffer[offset++];
        process_command(command, transfer_buffer + offset, payload_length);
        offset += payload_length;
    }

    transfer_buffer_count = 0;
    transfer_length = 0;
}

void reset()
{
    for (int i = 0; i < CHANNELS; i++)
    {
        if (led_counts[i] > 0)
        {
            uint8_t *data = buffers[i];

            multicore_fifo_pop_blocking();
            __builtin_memset(data, 0, BUFFER_SIZE);
            multicore_fifo_push_blocking(i);
        }
    }
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)report_type;

    process_command(buffer[0], buffer + 1, bufsize - 1);
}

void tud_umount_cb(void)
{
    reset();
}

void tud_vendor_task(void)
{
#if CFG_TUD_VENDOR > 0
    if (tud_vendor_available())
    {
        uint8_t *readBuffer = read_buffer;
        uint32_t count = tud_vendor_read(read_buffer, sizeof(read_buffer));
        while (count > 0)
        {
            if (transfer_length == 0)
            {
                if (count < 2)
                {
                    count = 0;
                }
                else
                {
                    transfer_length = ((uint16_t)readBuffer[0] << 8) | readBuffer[1];
                    readBuffer += 2;
                    count -= 2;
                }
            }
            else
            {
                uint32_t missingData = transfer_length - transfer_buffer_count;
                uint32_t copyAmount = missingData > count ? count : missingData;

                __builtin_memcpy(transfer_buffer + transfer_buffer_count, readBuffer, copyAmount);

                transfer_buffer_count += copyAmount;
                missingData -= copyAmount;

                if (missingData == 0)
                {
                    process_transfer_buffer();
                }

                readBuffer += copyAmount;
                count -= copyAmount;
            }
        }
    }
#endif
}

void loop_core1()
{
    multicore_fifo_push_blocking(0);
    while (1)
    {
        uint32_t channel = multicore_fifo_pop_blocking();
        if (channel < CHANNELS)
        {
            update_channel(channel);
        }
        multicore_fifo_push_blocking(channel);
    }
}

void loop()
{
    watchdog_update();
    tud_task();
    tud_vendor_task();
}

void setup()
{
    load_configuration();

    watchdog_enable(500, 1);

    for (int i = 0; i < CHANNELS; i++)
    {
        if (led_counts[i] > 0)
        {
            init_channel(i);
        }
    }

    tusb_init();
}

int main(void)
{
    setup();

    multicore_launch_core1(loop_core1);

    while (1)
    {
        loop();
    }

    return 0;
}

// Configuration Validation
#if DEFAULT_LED_COUNT_CHANNEL_1 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 1
#endif

#if DEFAULT_LED_COUNT_CHANNEL_2 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 2
#endif

#if DEFAULT_LED_COUNT_CHANNEL_3 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 3
#endif

#if DEFAULT_LED_COUNT_CHANNEL_4 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 4
#endif

#if DEFAULT_LED_COUNT_CHANNEL_5 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 5
#endif

#if DEFAULT_LED_COUNT_CHANNEL_6 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 6
#endif

#if DEFAULT_LED_COUNT_CHANNEL_7 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 7
#endif

#if DEFAULT_LED_COUNT_CHANNEL_8 > MAX_CHANNEL_SIZE
#error There are more than MAX_CHANNEL_SIZE LEDs in channel 8
#endif

#if CHANNELS != 8
#error Channel-count can not be changed without adapting the code for it!
#endif

#if MAX_CHANNEL_SIZE != 255
#error Max channel size can not be changed without adapting the code for it!
#endif

#if CFG_TUD_HID < 1
#error At least one HID-endpoint is required!
#endif
