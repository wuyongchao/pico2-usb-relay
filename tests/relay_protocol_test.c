#include "relay_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

typedef struct {
    unsigned calls;
    unsigned channel;
    bool on;
} output_capture_t;

static void capture_output(void *context, unsigned channel, bool on) {
    output_capture_t *capture = context;
    ++capture->calls;
    capture->channel = channel;
    capture->on = on;
}

static void test_feature_layout(void) {
    relay_device_t device;
    relay_device_init(&device, "PICO1", 1, NULL, NULL);
    uint8_t report[RELAY_REPORT_SIZE];
    relay_build_feature(&device, report);
    const uint8_t expected[RELAY_REPORT_SIZE] =
        {'P', 'I', 'C', 'O', '1', 0, 0, 0};
    CHECK(memcmp(report, expected, sizeof(expected)) == 0);
    device.state = 1;
    relay_build_feature(&device, report);
    CHECK(report[7] == 1);
}

static void test_usbrelay_wire_layout(void) {
    relay_device_t device;
    relay_device_init(&device, "PICO1", 1, NULL, NULL);
    device.state = 1;

    uint8_t report[RELAY_REPORT_SIZE];
    relay_build_feature(&device, report);

    /*
     * usbrelay requests Report ID 1 and reads the first five returned bytes
     * as <ID[0..4]>. State is byte 7. A standard request without a Report ID
     * carries exactly the same eight data bytes.
     */
    const uint8_t expected[8] =
        {'P', 'I', 'C', 'O', '1', 0, 0, 1};
    uint8_t wire[8];
    memcpy(wire, report, sizeof(report));
    CHECK(memcmp(wire, expected, sizeof(expected)) == 0);
    CHECK(memcmp(wire, "PICO1", RELAY_ID_SIZE) == 0);
    CHECK(wire[7] == 1);
    CHECK(report[0] == 'P');
}

static void test_on_off_and_invalid(void) {
    relay_device_t device;
    output_capture_t capture = {0};
    relay_device_init(&device, "PICO1", 1, capture_output, &capture);

    const uint8_t on[8] = {RELAY_CMD_ON, 1, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, on, sizeof(on)) ==
          RELAY_RESULT_OUTPUT_CHANGED);
    CHECK(device.state == 1 && capture.calls == 1 && capture.on);
    CHECK(relay_process_output(&device, on, sizeof(on)) ==
          RELAY_RESULT_IGNORED);

    const uint8_t off[8] = {RELAY_CMD_OFF, 1, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, off, sizeof(off)) ==
          RELAY_RESULT_OUTPUT_CHANGED);
    CHECK(device.state == 0 && !capture.on);

    const uint8_t bad_channel[8] = {RELAY_CMD_ON, 2, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, bad_channel, sizeof(bad_channel)) ==
          RELAY_RESULT_IGNORED);
    const uint8_t bad_command[8] = {0x42, 1, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, bad_command, sizeof(bad_command)) ==
          RELAY_RESULT_IGNORED);
    const uint8_t short_frame[2] = {RELAY_CMD_ON, 1};
    CHECK(relay_process_output(&device, short_frame, sizeof(short_frame)) ==
          RELAY_RESULT_IGNORED);
    CHECK(device.state == 0);
}

static void test_channel_nine(void) {
    relay_device_t device;
    output_capture_t capture = {0};
    relay_device_init(&device, "PICO1", 1, capture_output, &capture);
    const uint8_t all_on[8] = {RELAY_CMD_ON, RELAY_ALL_CHANNELS,
                               0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, all_on, sizeof(all_on)) ==
          RELAY_RESULT_OUTPUT_CHANGED);
    CHECK(device.state == 1 && capture.channel == 1 && capture.on);
}

static void test_id_change(void) {
    relay_device_t device;
    relay_device_init(&device, "PICO1", 1, NULL, NULL);
    const uint8_t set_id[8] =
        {RELAY_CMD_SET_ID, 0, 'A', 'B', 'C', '1', '2', 0};
    CHECK(relay_process_output(&device, set_id, sizeof(set_id)) ==
          RELAY_RESULT_ID_CHANGED);
    CHECK(memcmp(device.id, "ABC12", RELAY_ID_SIZE) == 0);

    const uint8_t bad_id[8] =
        {RELAY_CMD_SET_ID, 0, 'A', 'b', 'C', '1', '2', 0};
    CHECK(relay_process_output(&device, bad_id, sizeof(bad_id)) ==
          RELAY_RESULT_IGNORED);
    CHECK(memcmp(device.id, "ABC12", RELAY_ID_SIZE) == 0);
}

static void test_windows_all_channels(void) {
    relay_device_t device;
    output_capture_t capture = {0};
    relay_device_init(&device, "PICO1", 1, capture_output, &capture);

    const uint8_t all_on[8] =
        {RELAY_CMD_ALL_ON, 0, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, all_on, sizeof(all_on)) ==
          RELAY_RESULT_OUTPUT_CHANGED);
    CHECK(device.state == 1 && capture.on && capture.channel == 1);

    const uint8_t all_off[8] =
        {RELAY_CMD_ALL_OFF, 0, 0, 0, 0, 0, 0, 0};
    CHECK(relay_process_output(&device, all_off, sizeof(all_off)) ==
          RELAY_RESULT_OUTPUT_CHANGED);
    CHECK(device.state == 0 && !capture.on);
}

int main(void) {
    test_feature_layout();
    test_usbrelay_wire_layout();
    test_on_off_and_invalid();
    test_channel_nine();
    test_id_change();
    test_windows_all_channels();
    puts("relay protocol tests: PASS");
    return 0;
}
