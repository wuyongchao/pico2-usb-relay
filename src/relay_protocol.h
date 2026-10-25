#ifndef RELAY_PROTOCOL_H
#define RELAY_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RELAY_REPORT_SIZE 8u
#define RELAY_ID_SIZE 5u

#define RELAY_CMD_ON 0xFFu
#define RELAY_CMD_OFF 0xFD
#define RELAY_CMD_ALL_ON 0xFEu
#define RELAY_CMD_ALL_OFF 0xFCu
#define RELAY_CMD_SET_ID 0xFAu
#define RELAY_ALL_CHANNELS 9u

typedef void (*relay_output_fn)(void *context, unsigned channel, bool on);

typedef struct {
    uint8_t id[RELAY_ID_SIZE];
    uint8_t state;
    uint8_t channel_count;
    relay_output_fn output;
    void *output_context;
} relay_device_t;

typedef enum {
    RELAY_RESULT_IGNORED = 0,
    RELAY_RESULT_OUTPUT_CHANGED,
    RELAY_RESULT_ID_CHANGED,
} relay_result_t;

void relay_device_init(relay_device_t *device, const char id[RELAY_ID_SIZE],
                       uint8_t channel_count, relay_output_fn output,
                       void *output_context);

/* Receives the report payload only: the host-side Report ID is not included. */
relay_result_t relay_process_output(relay_device_t *device,
                                    const uint8_t *payload, size_t length);

/* Produces the eight DCTTech data bytes without a Windows report ID. */
void relay_build_feature(const relay_device_t *device,
                         uint8_t out[RELAY_REPORT_SIZE]);


bool relay_id_is_valid(const uint8_t id[RELAY_ID_SIZE]);

/* USB glue obtains the shared relay state through this accessor. */
relay_device_t *usbrelay_relay_device(void);

#endif
