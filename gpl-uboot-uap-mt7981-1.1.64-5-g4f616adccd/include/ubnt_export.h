// vim: ts=4:sw=4:expandtab
/* Copyright Ubiquiti Networks Inc.
** All Right Reserved.
*/

#ifndef _UBNT_EXPORT_H_
#define _UBNT_EXPORT_H_
#include <exports.h>
#ifdef CONFIG_UBNT_LEDBAR
#include <ubnt_ledbar.h>
#endif

#define UBNT_ACCEPT_OLD_FW_STRING

#define UBNT_APP_MAGIC 0x87564523
#define UBNT_BOOTSEL_SIZE 8

#define VENDORID_UBNT           0x0777
#define SYSTEMID_U6PROv2        0xa650
#define SYSTEMID_U6MESHv2       0xa651
#define SYSTEMID_U6IWv2         0xa652
#define SYSTEMID_U6EXTENDERv2   0xa653
#define SYSTEMID_U6ENTERPRISE   0xa654
#define SYSTEMID_U6AUDIENCE     0xa655
#define SYSTEMID_U6ENTERPRISEIW 0xa656
#define SYSTEMID_UNIFIPOEAF     0xdcb4
#define SYSTEMID_UNIFIPOEAT     0xdcb5
#define SYSTEMID_U6PLUS         0xa642
#define SYSTEMID_U6LRPLUS       0xa643

#define UBOOT_PART_NAME                 "u-boot"
#define GPT_PART_NAME                   "gpt"
#define KERNEL0_PART_NAME               "kernel0"
#define KERNEL1_PART_NAME               "kernel1"
#define KERNELx_PART_NAME               "kernel?"
#define SBL1_PART_NAME                  "SBL1"
#define SBL2_PART_NAME                  "bl2"
#define SBL3_PART_NAME                  "SBL3"
#define TZ_PART_NAME                    "TZ"
#define RPM_PART_NAME                   "RPM"
#define QSEE_PART_NAME                  "QSEE"
#define DEVCFG_PART_NAME                "DEVCFG"

#define UBNT_FLAG_TRUE                  "TRUE"
#define UBNT_FLAG_FALSE                 "FALSE"

#define GPIO_NOT_AVAILABLE              (-1)
#define GPIO_RESET_DEFAULT              1
#define GPIO_WHITE_DEFAULT              34
#define GPIO_BLUE_1109_DEFAULT          35
#define GPIO_BLUE_1210_DEFAULT          9

#define ETH_ALEN 6

#define UBNT_PRINTF(d, ...)                                                         \
                                        do {                                        \
                                            if (!d && !ubnt_flag("ubnt_debug"))     \
                                                break;                              \
                                            printf(__VA_ARGS__);                    \
                                        } while (0)


/* this structure shared with uboot common code */
#define UBNT_INVALID_ADDR 0
typedef struct ubnt_app_partition {
    uintptr_t addr;
    unsigned int size;
} ubnt_app_partition_t;

typedef struct ubnt_app_update_table {
#ifdef CONFIG_UBNT_HAS_GPT
        ubnt_app_partition_t gpt;
#endif
#ifdef CONFIG_UBNT_HAS_SBL1
    ubnt_app_partition_t sbl1;
#endif
#ifdef CONFIG_UBNT_HAS_SBL2
    ubnt_app_partition_t bl2;
#endif
#ifdef CONFIG_UBNT_HAS_SBL3
    ubnt_app_partition_t sbl3;
#endif
    ubnt_app_partition_t uboot;
    ubnt_app_partition_t kernel0;
    ubnt_app_partition_t kernel1;
    ubnt_app_partition_t bootselect;
#ifdef CONFIG_UBNT_HAS_TZ
    ubnt_app_partition_t tz;
#endif
#ifdef CONFIG_UBNT_HAS_RPM
    ubnt_app_partition_t rpm;
#endif
#ifdef CONFIG_UBNT_HAS_QSEE
    ubnt_app_partition_t qsee;
#endif
#ifdef CONFIG_UBNT_HAS_DEVCFG
    ubnt_app_partition_t devcfg;
#endif
} ubnt_app_update_table_t;

typedef struct ubnt_bdinfo {
    uint8_t eth0[ETH_ALEN];
    uint8_t eth1[ETH_ALEN];
    const char *eth_primary;
    uint16_t boardid;
    uint16_t vendorid;
    uint32_t bomrev;
    uint8_t is_ubnt;
    int16_t reset_button_gpio;
    int16_t white_led_gpio;
    int16_t blue_led_gpio;
#ifdef CONFIG_UBNT_LEDBAR
    uint8_t led_count;
    ledbar_color_t white_color;
#endif
} ubnt_bdinfo_t;

typedef struct ubnt_app_share_data {
    /* These vars must be initialized before calling ubnt_app */
    ubnt_bdinfo_t board_info;
    uintptr_t bootselect_addr;
    /* These vars are set by ubnt_app */
    uint8_t kernel_index;
    ubnt_app_update_table_t update;
} ubnt_app_share_data_t;

enum ubnt_app_events {
    UBNT_EVENT_UNKNOWN          = 0,
    UBNT_EVENT_TFTP_TIMEOUT     = 1,
    UBNT_EVENT_TFTP_RECEIVING   = 2,
    UBNT_EVENT_TFTP_DONE        = 3,
    UBNT_EVENT_ERASING_FLASH    = 4,
    UBNT_EVENT_WRITING_FLASH    = 5,
    UBNT_EVENT_READING_FLASH    = 6,
    UBNT_EVENT_KERNEL_VALID     = 7,
    UBNT_EVENT_KERNEL_INVALID   = 8,
    UBNT_EVENT_USE_FACTORY_KERNEL = 9,
    UBNT_EVENT_END
};

typedef struct {
    uint32_t ubnt_magic;
    ubnt_app_share_data_t* data_addr;
    int* entry;
} ubnt_app_hdr_t;

void ubnt_app_continue(int event);

// For use in u-boot
const ubnt_bdinfo_t *ubnt_bdinfo_get(void);
// For use in ubnt_app
const ubnt_bdinfo_t *get_parent_board_info(void);

uint8_t ubnt_prepare_beacon(uint8_t *buf);

static inline int ubnt_flag(const char *name)
{
    char *val = env_get(name);
    return val && !strcmp(val, UBNT_FLAG_TRUE);
}
#if defined(CONFIG_IPQ_BT_SUPPORT) || defined(CONFIG_CSR8811_BT_SUPPORT)
#include <ubnt_ble.h>
#endif
#endif
