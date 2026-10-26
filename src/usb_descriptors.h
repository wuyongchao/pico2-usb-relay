// SPDX-License-Identifier: MIT

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

uint8_t const *usbrelay_hid_report_descriptor(void);
uint16_t usbrelay_hid_report_descriptor_length(void);

#endif
