// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "pico/flash.h"
#include "pico/stdlib.h"
#include "relay_protocol.h"
#include "tusb.h"

#ifndef USBRELAY_CH1_GPIO
#define USBRELAY_CH1_GPIO 6
#endif
#ifndef USBRELAY_ACTIVE_LOW
#define USBRELAY_ACTIVE_LOW 1
#endif
#ifndef USBRELAY_RELAY_ID
#define USBRELAY_RELAY_ID "PICO1"
#endif
#ifndef USBRELAY_PERSIST_ID
#define USBRELAY_PERSIST_ID 1
#endif

#define RELAY_CHANNEL_COUNT 1u
static const char default_relay_id[] = USBRELAY_RELAY_ID;
_Static_assert(sizeof(default_relay_id) == RELAY_ID_SIZE + 1u,
               "USBRELAY_RELAY_ID must contain exactly five characters");
_Static_assert(USBRELAY_CH1_GPIO >= 0 && USBRELAY_CH1_GPIO < NUM_BANK0_GPIOS,
               "USBRELAY_CH1_GPIO must be a valid RP2350 GPIO");

#if USBRELAY_PERSIST_ID
#define RELAY_STORAGE_MAGIC 0x31524450u
#define RELAY_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct {
    uint32_t magic;
    uint8_t id[RELAY_ID_SIZE];
    uint8_t reserved[3];
    uint32_t checksum;
} relay_storage_t;

_Static_assert(sizeof(relay_storage_t) <= FLASH_PAGE_SIZE,
               "storage record must fit in one flash page");
#endif

static relay_device_t relay;
static volatile bool persist_requested;

relay_device_t *usbrelay_relay_device(void) {
    return &relay;
}

#if USBRELAY_PERSIST_ID
static uint32_t storage_checksum(uint32_t magic,
                                 const uint8_t id[RELAY_ID_SIZE]) {
    uint32_t sum = magic;
    for (size_t i = 0; i < RELAY_ID_SIZE; ++i) {
        sum = (sum * 33u) ^ id[i];
    }
    return sum;
}
#endif

static void load_relay_id(uint8_t id[RELAY_ID_SIZE]) {
    memcpy(id, default_relay_id, RELAY_ID_SIZE);
#if USBRELAY_PERSIST_ID
    relay_storage_t stored;
    memcpy(&stored, (const void *)(XIP_BASE + RELAY_STORAGE_OFFSET),
           sizeof(stored));
    if (stored.magic == RELAY_STORAGE_MAGIC &&
        stored.checksum == storage_checksum(stored.magic, stored.id) &&
        relay_id_is_valid(stored.id)) {
        memcpy(id, stored.id, RELAY_ID_SIZE);
    }
#endif
}

static void gpio_write_logical(void *context, unsigned channel, bool on) {
    (void)context;
    if (channel != 1) {
        return;
    }
#if USBRELAY_ACTIVE_LOW
    gpio_put(USBRELAY_CH1_GPIO, on ? 0 : 1);
#else
    gpio_put(USBRELAY_CH1_GPIO, on ? 1 : 0);
#endif
}

static void relay_gpio_init(void) {
    gpio_init(USBRELAY_CH1_GPIO);
#if USBRELAY_ACTIVE_LOW
    gpio_put(USBRELAY_CH1_GPIO, 1);
#else
    gpio_put(USBRELAY_CH1_GPIO, 0);
#endif
    gpio_write_logical(NULL, 1, false);
    gpio_set_dir(USBRELAY_CH1_GPIO, GPIO_OUT);
}

#if USBRELAY_PERSIST_ID
static void flash_write_storage(void *param) {
    const relay_storage_t *record = (const relay_storage_t *)param;
    uint8_t page[FLASH_PAGE_SIZE] = {0};
    memcpy(page, record, sizeof(*record));
    flash_range_erase(RELAY_STORAGE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(RELAY_STORAGE_OFFSET, page, sizeof(page));
}
#endif

static void persist_id(const uint8_t id[RELAY_ID_SIZE]) {
#if USBRELAY_PERSIST_ID
    relay_storage_t record = {0};
    record.magic = RELAY_STORAGE_MAGIC;
    memcpy(record.id, id, RELAY_ID_SIZE);
    record.checksum = storage_checksum(record.magic, record.id);

    (void)flash_safe_execute(flash_write_storage, &record, 1000);
#else
    (void)id;
#endif
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    if ((report_type != HID_REPORT_TYPE_OUTPUT &&
         report_type != HID_REPORT_TYPE_FEATURE) ||
        buffer == NULL) {
        return;
    }
    if (relay_process_output(&relay, buffer, bufsize) ==
        RELAY_RESULT_ID_CHANGED) {
        persist_requested = true;
    }
}

bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate) {
    (void)instance;
    (void)idle_rate;
    return true;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    (void)instance;
    if (report_type != HID_REPORT_TYPE_FEATURE || buffer == NULL ||
        reqlen == 0) {
        return 0;
    }

    uint8_t report[RELAY_REPORT_SIZE];
    relay_build_feature(&relay, report);

    (void)report_id;
    const uint16_t response_length =
        reqlen < sizeof(report) ? reqlen : sizeof(report);
    memcpy(buffer, report, response_length);
    return response_length;
}

int main(void) {
    relay_gpio_init();

    uint8_t id[RELAY_ID_SIZE];
    load_relay_id(id);
    relay_device_init(&relay, (const char *)id, RELAY_CHANNEL_COUNT,
                      gpio_write_logical, NULL);

    tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_init(0, &rh_init);
    while (true) {
        tud_task();
        if (persist_requested) {
            persist_requested = false;
            persist_id(relay.id);
        }
    }
}
