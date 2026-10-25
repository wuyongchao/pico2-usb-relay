#include "relay_protocol.h"

#include <string.h>

static bool valid_id_char(uint8_t ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
}

bool relay_id_is_valid(const uint8_t id[RELAY_ID_SIZE]) {
    if (id == NULL) {
        return false;
    }
    for (size_t i = 0; i < RELAY_ID_SIZE; ++i) {
        if (!valid_id_char(id[i])) {
            return false;
        }
    }
    return true;
}

void relay_device_init(relay_device_t *device, const char id[RELAY_ID_SIZE],
                       uint8_t channel_count, relay_output_fn output,
                       void *output_context) {
    memset(device, 0, sizeof(*device));
    if (id != NULL) {
        memcpy(device->id, id, RELAY_ID_SIZE);
    }
    if (!relay_id_is_valid(device->id)) {
        memcpy(device->id, "PICO1", RELAY_ID_SIZE);
    }
    device->channel_count = channel_count > 8 ? 8 : channel_count;
    device->output = output;
    device->output_context = output_context;
}

static bool apply_channel(relay_device_t *device, unsigned channel, bool on) {
    const uint8_t mask = (uint8_t)(1u << (channel - 1u));
    const uint8_t old_state = device->state;
    if (on) {
        device->state |= mask;
    } else {
        device->state &= (uint8_t)~mask;
    }
    if (device->output != NULL) {
        device->output(device->output_context, channel, on);
    }
    return device->state != old_state;
}

relay_result_t relay_process_output(relay_device_t *device,
                                    const uint8_t *payload, size_t length) {
    if (device == NULL || payload == NULL || length < 3) {
        return RELAY_RESULT_IGNORED;
    }

    const uint8_t command = payload[0];
    const uint8_t channel = payload[1];

    if (command == RELAY_CMD_SET_ID && length >= 7) {
        if (!relay_id_is_valid(&payload[2])) {
            return RELAY_RESULT_IGNORED;
        }
        memcpy(device->id, &payload[2], RELAY_ID_SIZE);
        return RELAY_RESULT_ID_CHANGED;
    }

    const bool all_on = command == RELAY_CMD_ALL_ON;
    const bool all_off = command == RELAY_CMD_ALL_OFF;
    if (command != RELAY_CMD_ON && command != RELAY_CMD_OFF &&
        !all_on && !all_off) {
        return RELAY_RESULT_IGNORED;
    }
    if (channel == 0 && !all_on && !all_off) {
        return RELAY_RESULT_IGNORED;
    }

    const bool on = command == RELAY_CMD_ON || all_on;
    bool changed = false;
    if (channel == RELAY_ALL_CHANNELS || all_on || all_off) {
        for (unsigned current = 1; current <= device->channel_count; ++current) {
            changed |= apply_channel(device, current, on);
        }
    } else if (channel <= device->channel_count) {
        changed = apply_channel(device, channel, on);
    } else {
        return RELAY_RESULT_IGNORED;
    }
    return changed ? RELAY_RESULT_OUTPUT_CHANGED : RELAY_RESULT_IGNORED;
}

void relay_build_feature(const relay_device_t *device,
                         uint8_t out[RELAY_REPORT_SIZE]) {
    memset(out, 0, RELAY_REPORT_SIZE);
    memcpy(out, device->id, RELAY_ID_SIZE);
    out[7] = device->state;
}

