#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H
#define CFG_TUSB_MCU             OPT_MCU_RP2040
#define BOARD_TUD_RHPORT         0
#define CFG_TUD_ENABLED          1
#define CFG_TUH_ENABLED          0
#define CFG_TUSB_RHPORT0_MODE    OPT_MODE_DEVICE
#define CFG_TUSB_ENDPOINT0_SIZE  64
#define CFG_TUSB_RAM_SIZE        4096
#define CFG_TUD_HID              1
#define CFG_TUD_CDC              1
#define CFG_TUD_MSC              0
#define CFG_TUD_VENDOR           0
#define CFG_TUD_HID_BUFSIZE      16
#define CFG_TUD_CDC_RX_BUFSIZE   64
#define CFG_TUD_CDC_TX_BUFSIZE   64

#endif /* TUSB_CONFIG_H */