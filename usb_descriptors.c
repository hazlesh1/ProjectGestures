#include "tusb.h"
#include "hid/hid_output.h"

#if HID_PROFILE == HID_PROFILE_MEDIA
#include "hid/hid_profile_media.h"
#elif HID_PROFILE == HID_PROFILE_PAGING
#include "hid/hid_profile_paging.h"
#endif

enum {
    ITF_NUM_HID = 0,
    ITF_COUNT
};

// Total length of everything returned by tud_descriptor_configuration_cb():
// config descriptor header (9 bytes) + HID interface/descriptor/endpoint block.
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID 0x81

uint8_t const * tud_descriptor_device_cb(void) {
    static uint8_t const desc[] = {
        0x12,
        TUSB_DESC_DEVICE,
        0x00, 0x02,
        0x00, 0x00, 0x00,
        0x40,
        0xC0, 0x16,
        0xDB, 0x05,
        0x00, 0x01,
        1, 2, 3,
        1
    };
    return desc;
}

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_descriptor;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;

    // Previously this only returned the TUD_HID_DESCRIPTOR block with no
    // configuration descriptor header in front of it. The host expects
    // byte 0 of this response to be a bDescriptorType == 0x02 (CONFIGURATION)
    // descriptor whose wTotalLength covers the whole returned blob. Without
    // it, the first thing the host parses is actually the HID interface
    // descriptor (type 0x04), which is why Windows reported an
    // "invalid configuration descriptor."
    static uint8_t const config[] = {
        TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_TOTAL_LEN,
                               TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

        TUD_HID_DESCRIPTOR(
            ITF_NUM_HID,
            0,
            HID_ITF_PROTOCOL_NONE,
            hid_report_descriptor_length,
            EPNUM_HID,
            CFG_TUD_HID_EP_BUFSIZE,
            5
        )
    };
    return config;
}

uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    // bLength = 2 (header: length byte + descriptor type byte) + 2 bytes per UTF-16 char.
    // These MUST match the actual number of char pairs below or the host reads
    // past the array (garbage/corrupt strings, intermittent enumeration failure).

    if (index == 0) {
        static uint8_t const lang_desc[] = { 0x04, 0x03, 0x09, 0x04 };
        return (uint16_t const *)lang_desc;
    }
    if (index == 1) {
        // "LeoGi" -> 5 chars -> 2 + 5*2 = 12 = 0x0C
        static uint8_t const mfg[] = {
            0x0C, 0x03,
            'L', 0, 'e', 0, 'o', 0, 'G', 0, 'i', 0
        };
        return (uint16_t const *)mfg;
    }
    if (index == 2) {
        // "ProjectGest" -> 11 chars -> 2 + 11*2 = 24 = 0x18
        static uint8_t const prod[] = {
            0x18, 0x03,
            'P', 0, 'r', 0, 'o', 0, 'j', 0, 'e', 0, 'c', 0, 't', 0, 'G', 0, 'e', 0, 's', 0, 't', 0
        };
        return (uint16_t const *)prod;
    }
    if (index == 3) {
        // "0001" -> 4 chars -> 2 + 4*2 = 10 = 0x0A
        static uint8_t const serial[] = {
            0x0A, 0x03,
            '0', 0, '0', 0, '0', 0, '1', 0
        };
        return (uint16_t const *)serial;
    }
    return NULL;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type, uint8_t *buffer,
                                uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}