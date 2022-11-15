/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Configuration for MediaTek MT7981 SoC
 *
 * Copyright (C) 2021 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef __MT7981_H
#define __MT7981_H

#include <linux/sizes.h>

#define CONFIG_SYS_MAXARGS                      32
#define CONFIG_SYS_BOOTM_LEN                    SZ_64M
#define CONFIG_SYS_CBSIZE                       SZ_1K
#define CONFIG_SYS_PBSIZE                       (CONFIG_SYS_CBSIZE + \
                                                    sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_NONCACHED_MEMORY             SZ_1M
#define CONFIG_SYS_MMC_ENV_DEV                  0

/* Uboot definition */
#define CONFIG_SYS_UBOOT_BASE                   CONFIG_SYS_TEXT_BASE

/* SPL -> Uboot */
#define CONFIG_SYS_UBOOT_START                  CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_INIT_SP_ADDR                 (CONFIG_SYS_TEXT_BASE + \
                                                    SZ_2M - GENERATED_GBL_DATA_SIZE)

/* Flash */
#define CONFIG_SYS_NAND_MAX_CHIPS               1

/* DRAM */
#define CONFIG_SYS_SDRAM_BASE                   0x40000000

/* Ethernet */
#define CONFIG_IPADDR                           192.168.1.20
#define CONFIG_SERVERIP                         192.168.1.2
#define CONFIG_NETMASK                          255.255.255.0

/* Auto boot */
#define CONFIG_BOOTCOMMAND                      "bootubnt"

#define CONFIG_UBNT_APP
#define CONFIG_UBNT_APP_ADDR                    0x44000000
#ifdef CONFIG_UBNT_APP
#include <configs/ubnt_app.h>

#define CONFIG_CMD_TFTPSRV

#undef CONFIG_NET_RETRY_COUNT
#define CONFIG_NET_RETRY_COUNT                  500
#define CONFIG_FIT_DISABLE_SHA1_RSA2048

#define CONFIG_UBNT_PERSISTENT_LOG
#ifdef CONFIG_UBNT_PERSISTENT_LOG
#define CONFIG_SYS_MEM_TOP_HIDE                 (32 << 10)
#define UBNT_LOG_BUFF_SIZE                      (128 << 10)
#define UBNT_LOG_RECORD_SIZE                    (32 << 10)
#define UBNT_RAM_OOPS_ADDRESS                   (0x42fe0000)
#endif

#ifdef CONFIG_MMC
#define KB_SIZE(s)                              (s << 10)
#define GET_PART_SIZE(n)                        CONFIG_UBNT_##n##_SIZE
#define GET_PART_OFFSET(n)                      CONFIG_UBNT_##n##_OFFSET

#define CONFIG_UBNT_HAS_SBL2
#define CONFIG_UBNT_HAS_GPT
#define CONFIG_UBNT_HAS_UBOOT


#define CONFIG_UBNT_GPT_SIZE                    KB_SIZE(17)
#define CONFIG_UBNT_GPT_OFFSET                  0
#define CONFIG_UBNT_SBL2_SIZE                   KB_SIZE(4079)
#define CONFIG_UBNT_SBL2_OFFSET                 (CONFIG_UBNT_GPT_OFFSET + CONFIG_UBNT_GPT_SIZE)
#define CONFIG_UBNT_UBOOT_ENV_SIZE              KB_SIZE(512)
#define CONFIG_UBNT_UBOOT_ENV_OFFSET            (CONFIG_UBNT_SBL2_OFFSET + CONFIG_UBNT_SBL2_SIZE)
#define CONFIG_UBNT_FACTORY_SIZE                KB_SIZE(2048)
#define CONFIG_UBNT_FACTORY_OFFSET              (CONFIG_UBNT_UBOOT_ENV_OFFSET + CONFIG_UBNT_UBOOT_ENV_SIZE)
#define CONFIG_UBNT_UBOOT_SIZE                  KB_SIZE(2048)
#define CONFIG_UBNT_UBOOT_OFFSET                (CONFIG_UBNT_FACTORY_OFFSET + CONFIG_UBNT_FACTORY_SIZE)
#define CONFIG_UBNT_EEPROM_SIZE                 KB_SIZE(64)
#define CONFIG_UBNT_EEPROM_OFFSET               (CONFIG_UBNT_UBOOT_OFFSET + CONFIG_UBNT_UBOOT_SIZE)
#define CONFIG_UBNT_KERNEL0_SIZE                KB_SIZE(131072)
#define CONFIG_UBNT_KERNEL0_OFFSET              (CONFIG_UBNT_EEPROM_OFFSET + CONFIG_UBNT_EEPROM_SIZE)
#define CONFIG_UBNT_KERNEL1_SIZE                KB_SIZE(131072)
#define CONFIG_UBNT_KERNEL1_OFFSET              (CONFIG_UBNT_KERNEL0_OFFSET + CONFIG_UBNT_KERNEL0_SIZE)
#define CONFIG_UBNT_BS_SIZE                     KB_SIZE(2)
#define CONFIG_UBNT_BS_OFFSET                   (CONFIG_UBNT_KERNEL1_OFFSET + CONFIG_UBNT_KERNEL1_SIZE)
#define CONFIG_UBNT_CFG_SIZE                    KB_SIZE(2048)
#define CONFIG_UBNT_CFG_OFFSET                  (CONFIG_UBNT_BS_OFFSET + CONFIG_UBNT_BS_SIZE)
#define CONFIG_UBNT_LOG_SIZE                    KB_SIZE(524288)
#define CONFIG_UBNT_LOG_OFFSET                  (CONFIG_UBNT_CFG_OFFSET + CONFIG_UBNT_CFG_SIZE)
#endif

#ifdef CONFIG_LZMA
#define CONFIG_UBNT_APP_LZMA
#endif

#define CONFIG_UBNT_FAKE_EEPROM

#define DCACHE_WRITEBACK_EXE                    (DCACHE_WRITEBACK & (~0x10))

#define MAX_FLASH_BLOCK_SIZE                    512
#define EMMC_BLOCK_SIZE                         512

#define PART_LOOKUP_GPT                         "gpt"
#define PART_LOOKUP_SBL2                        "bl2"
#define PART_LOOKUP_UBOOT_ENV                   "u-boot-env"
#define PART_LOOKUP_FACTORY                     "factory"
#define PART_LOOKUP_UBOOT                       "u-boot"
#define PART_LOOKUP_EEPROM                      "EEPROM"
#define PART_LOOKUP_BOOTSELECT                  "bs"
#define PART_LOOKUP_KERNEL0                     "kernel0"
#define PART_LOOKUP_KERNEL1                     "kernel1"
#define PART_LOOKUP_CFG                         "cfg"
#define PART_LOOKUP_LOG                         "log"

#define PART_LOOKUP_EEPROM_FLASH                "EEPROM-flash"
#define PART_LOOKUP_UBOOT_ENV_FLASH             "u-boot-env-flash"

#define RECOVERY_FWVERSION_ARCH                 "mt7981"
#define REQUIRED_RECOVERY_FW_VERSION
#define ALLOW_OVERRIDE_FW_VERSION_CHECK
#endif
#endif
