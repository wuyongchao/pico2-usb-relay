#include "usb_descriptors.h"

#include "tusb.h"
#include "relay_protocol.h"

#define USBRELAY_VID 0x16C0
#define USBRELAY_PID 0x05DF
#define USBRELAY_BCD_USB 0x0110
#define USBRELAY_BCD_DEVICE 0x0100

tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USBRELAY_BCD_USB,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBRELAY_VID,
    .idProduct = USBRELAY_PID,
    .bcdDevice = USBRELAY_BCD_DEVICE,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

/*
 * DCTTech USBRelay1 report descriptor:
 * 06 00 ff 09 01 a1 01 15 00 26 ff 00 75 08 95 08
 * 09 00 b2 02 01 c0
 */
uint8_t const desc_hid_report[] = {
    0x06, 0x00, 0xFF,       /* USAGE_PAGE (Vendor 0xFF00) */
    0x09, 0x01,             /* USAGE (1) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x15, 0x00,             /* LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,       /* LOGICAL_MAXIMUM (255) */
    0x75, 0x08,             /* REPORT_SIZE (8) */
    0x95, 0x08,             /* REPORT_COUNT (8) */
    0x09, 0x00,             /* USAGE (Undefined) */
    0xB2, 0x02, 0x01,       /* FEATURE (Data,Var,Abs,Buffered) */
    0xC0,                   /* END_COLLECTION */
};

_Static_assert(sizeof(desc_hid_report) == 22,
               "DCTTech report descriptor must be exactly 22 bytes");

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL,
};

#define EPNUM_HID_IN 0x81

#define CONFIG_TOTAL_LEN (9u + 9u + 9u + 7u)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 20),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report), EPNUM_HID_IN, 16, 20),
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

uint8_t const *usbrelay_hid_report_descriptor(void) {
    return desc_hid_report;
}

uint16_t usbrelay_hid_report_descriptor_length(void) {
    return sizeof(desc_hid_report);
}

char const *const string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "www.dcttech.com",
    "USBRelay1",
    NULL,
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_str[32];
    size_t count = 0;
    if (index == 0) {
        desc_str[1] = 0x0409;
        count = 1;
    } else {
        char const *str = index < (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
                              ? string_desc_arr[index]
                              : NULL;
        if (str == NULL) {
            return NULL;
        }
        while (str[count] != '\0' && count < 31) {
            desc_str[count + 1] = (uint8_t)str[count];
            ++count;
        }
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return desc_str;
}
