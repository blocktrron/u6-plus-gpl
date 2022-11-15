// vim: ts=4:sw=4:expandtab
/*          Copyright Ubiquiti Networks Inc. 2014
**          All Rights Reserved.
*/

#include <common.h>
#include <command.h>
#include <version.h>
#include <ubnt_export.h>
#include <ubnt_part.h>
#include <ubnt_util.h>
#ifdef CONFIG_UBNT_APP_LZMA
#include <lzma/LzmaTools.h>
#endif
#if defined(CONFIG_FIT) || defined(CONFIG_OF_LIBFDT)
    #include <fdt.h>
    #include <linux/libfdt.h>
#endif
#include <asm/system.h>
#include <image.h>
#include <env_internal.h>

#define SPI_FLASH_PAGE_SIZE             0x1000

/* UBNT_NUM_FLASH_SECTORS determines the LED timing during boot */
#define UBNT_NUM_FLASH_SECTORS          250
#define UBNT_FLASH_SECTOR_SIZE          (UBNT_NUM_FLASH_SECTORS * SPI_FLASH_PAGE_SIZE)
#define UBNT_FWUPDATE_SECTOR_SIZE       (8 * SPI_FLASH_PAGE_SIZE)

#define UBOOT_VER_ARG_STR "ubootver="
#define UBNTBOOTID_ARG_STR "ubntbootid="

#define KERNEL_VALIDATION_UNKNOW    0
#define KERNEL_VALIDATION_VALID     1
#define KERNEL_VALIDATION_INVALID   2

#define DTB_CFG_LEN     64

DECLARE_GLOBAL_DATA_PTR;

uint8_t const __attribute__((weak)) _binary_ubnt_app_blob_start[] = {0, 0, 0, 0, 0};
uint8_t const __attribute__((weak)) _binary_ubnt_app_blob_end[] = {0};

static ubnt_app_share_data_t *p_share_data;

static inline int is_valid_ubnt_app_addr(uintptr_t addr, const char* name)
{
    int res = (addr >= CONFIG_UBNT_APP_ADDR) &&
            (addr < (CONFIG_UBNT_APP_ADDR + CONFIG_UBNT_APP_SIZE));
    if (!res) {
        UBNT_PRINTF(false, " *WARNING*: UBNT APP %s out of range, addr=%lx\n",
               name, addr);
    }
    return res;
}

/* Loads the application into memory */
int ubnt_app_load(void)
{
    size_t ubnt_app_blob_size = _binary_ubnt_app_blob_end - _binary_ubnt_app_blob_start;
    ubnt_app_hdr_t *hdr;
    uint32_t magic_value;

#ifdef CONFIG_UBNT_APP_LZMA
    SizeT ubnt_app_size = CONFIG_UBNT_APP_SIZE;
    int res = lzmaBuffToBuffDecompress(UBNT_APP_ADDR, &ubnt_app_size,
            (void *)_binary_ubnt_app_blob_start, ubnt_app_blob_size);
    if (res == SZ_OK) {
        UBNT_PRINTF(false, "ubnt_app: LZMA decompressed: size %zu\n", ubnt_app_size);
        hdr = UBNT_APP_ADDR;
        magic_value = ntohl(hdr->ubnt_magic);
    } else {
        UBNT_PRINTF(true, " *WARNING*: UBNT APP LZMA decompress failure: %d\n", res);
        return -1;
    }
#else
    hdr = (ubnt_app_hdr_t *)_binary_ubnt_app_blob_start;
    magic_value = ntohl(hdr->ubnt_magic);
    if (UBNT_APP_MAGIC == magic_value) {
        memcpy(UBNT_APP_ADDR, _binary_ubnt_app_blob_start, ubnt_app_blob_size);
    }
#endif
    if (UBNT_APP_MAGIC != magic_value) {
        UBNT_PRINTF(true, " *WARNING*: UBNT APP Magic mismatch, addr=%p, magic=%x\n",
               hdr, magic_value);
        return -1;
    }

    if (!is_valid_ubnt_app_addr((uintptr_t)hdr->data_addr, "data_addr")) {
        return -1;
    }

    if (!is_valid_ubnt_app_addr((uintptr_t)hdr->entry, "entry")) {
        return -1;
    }

#ifndef CONFIG_SYS_DCACHE_OFF
    mmu_set_region_dcache_behaviour((phys_addr_t)UBNT_APP_ADDR, CONFIG_UBNT_APP_SIZE, DCACHE_WRITEBACK_EXE);
#endif
    p_share_data = (void *)hdr->data_addr;

    env_set_hex("ubntaddr",(ulong) hdr->entry);

    return 0;
}

typedef struct ubnt_eeprom_info {
    uint8_t eth0[ETH_ALEN];
    uint8_t eth1[ETH_ALEN];
    uint16_t boardid;
    uint16_t vendorid;
    uint32_t bomrev;
} __attribute__((packed)) ubnt_eeprom_info_t;

typedef struct {
    uint16_t *sysids;
    const char *eth_primary;
    uint8_t eth_mac_swap;
    uint8_t reset_button_gpio;
    uint8_t white_led_gpio;
    uint8_t blue_led_gpio;
#ifdef CONFIG_UBNT_LEDBAR
    uint8_t led_count;
    ledbar_color_t white_color;
#endif
} ubnt_bdinfo_per_ssid_t;


#define SYSID_LIST(...) .sysids = (uint16_t[]){ __VA_ARGS__, 0 }

static const ubnt_bdinfo_per_ssid_t board_table[] = {
#if defined(CONFIG_ARCH_MEDIATEK)
    {
        SYSID_LIST(SYSTEMID_U6PLUS, SYSTEMID_U6LRPLUS),
        .reset_button_gpio = 1,
    },
// END CONFIG_ARCH_MEDIATEK
#else
#error "Must provide a list in board_table[]"
#endif
};

const ubnt_bdinfo_t *ubnt_bdinfo_get(void)
{
    static int loaded = 0;
    static ubnt_bdinfo_t bdinfo;
    ubnt_eeprom_info_t ee;

    if (!loaded) {
        uint32_t size = sizeof(ee);

        if (0 != ubnt_part_cache_in_ram(PART_LOOKUP_EEPROM_FLASH,
                    (uintptr_t)&ee, &size)) {

            memset(&bdinfo, 0, sizeof(bdinfo));
            memcpy(bdinfo.eth0, "\x00\x11\x22\x33\x44\x55", sizeof(bdinfo.eth0));
            memcpy(bdinfo.eth1, "\x00\x11\x22\x33\x44\x56", sizeof(bdinfo.eth1));
            bdinfo.vendorid = VENDORID_UBNT;
            bdinfo.boardid = 0xa642;
            bdinfo.bomrev = 0x1;
            loaded = 1;

            // TODO: copy SF_EEPROM to EEPROM in emmc
            //| (!buffer) || (0 != ubnt_part_cache_in_emmc(PART_LOOKUP_EEPROM, PART_LOOKUP_SF_EEPROM,(uintptr_t) buffer, 0x10000)))
        } else {
            UBNT_PRINTF(false, "[%s][eth0]%x:%x:%x:%x:%x:%x\n",
                __FUNCTION__, ee.eth0[0], ee.eth0[1], ee.eth0[2], ee.eth0[3], ee.eth0[4], ee.eth0[5]);
            UBNT_PRINTF(false, "[%s][eth1]%x:%x:%x:%x:%x:%x\n",
                __FUNCTION__, ee.eth1[0], ee.eth1[1], ee.eth1[2], ee.eth1[3], ee.eth1[4], ee.eth1[5]);
            UBNT_PRINTF(false, "[%s]vendorid: %x, boardid: %x, bomrev: %x\n",
                __FUNCTION__, ntohs(ee.vendorid), ntohs(ee.boardid), ntohl(ee.bomrev));

            memcpy(bdinfo.eth0, ee.eth0, sizeof(bdinfo.eth0));
            memcpy(bdinfo.eth1, ee.eth1, sizeof(bdinfo.eth1));
            bdinfo.boardid = ntohs(ee.boardid);
            bdinfo.vendorid = ntohs(ee.vendorid);
            bdinfo.bomrev = ntohl(ee.bomrev);
            loaded = 1;
        }



        bdinfo.is_ubnt = (bdinfo.vendorid == VENDORID_UBNT);
        bdinfo.reset_button_gpio = GPIO_NOT_AVAILABLE;
#ifdef CONFIG_UBNT_LEDBAR
        bdinfo.led_count = 1;
        memset(&bdinfo.white_color, 0xff, sizeof(bdinfo.white_color));
#endif
        if (bdinfo.is_ubnt) {
            int i;
            for (i = 0; i < sizeof(board_table) / sizeof(board_table[0]); i++) {
                const ubnt_bdinfo_per_ssid_t *dev = &board_table[i];
                const uint16_t *sysid;
                for (sysid = dev->sysids; *sysid; sysid++) {
                    if (bdinfo.boardid == *sysid) {
                        bdinfo.eth_primary = dev->eth_primary;
                        if (dev->eth_mac_swap) {
                            uint8_t old_eth0[ETH_ALEN];
                            memcpy(old_eth0, bdinfo.eth0, ETH_ALEN);
                            memcpy(bdinfo.eth0, bdinfo.eth1, ETH_ALEN);
                            memcpy(bdinfo.eth1, old_eth0, ETH_ALEN);
                        }
                        bdinfo.reset_button_gpio = dev->reset_button_gpio;
                        bdinfo.white_led_gpio = dev->white_led_gpio ?: GPIO_NOT_AVAILABLE;
                        bdinfo.blue_led_gpio = dev->blue_led_gpio ?: GPIO_NOT_AVAILABLE;

#ifdef CONFIG_UBNT_LEDBAR
                        bdinfo.led_count = dev->led_count;
                        // Update custom white color
                        if (dev->white_color.red ||
                                dev->white_color.green ||
                                dev->white_color.blue) {
                            memcpy(&bdinfo.white_color, &dev->white_color,
                                    sizeof(bdinfo.white_color));
                        }
#endif /* CONFIG_UBNT_LEDBAR */

                        break;
                    }
                }
            }
        }

    }
    return &bdinfo;
}

static int ubnt_is_wireless_only_device(void)
{
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    if (bdinfo->is_ubnt) {
        switch (bdinfo->boardid) {
            case SYSTEMID_U6EXTENDERv2:
                return 1;
        }
    }
    return 0;
}
#if 0

static void disable_poe_output(void)
{
#ifdef CONFIG_UBNT_POE_CONTROL
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();

    if (bdinfo->is_ubnt) {
        switch (bdinfo->boardid) {
            case SYSTEMID_UNIFIPOEAF:
            case SYSTEMID_UNIFIPOEAT:
                ubnt_pca953x_gpios_set(CONFIG_UBNT_POE_CONTROL_GPIO, 0);
                break;
            default:
                break;
        }
    }
#endif
}
#endif 

#if 1
static int ubnt_read_bootselect(void) {
    static uint8_t bootselect_buffer[UBNT_BOOTSEL_SIZE];
    uint32_t size = sizeof(bootselect_buffer);

    p_share_data->bootselect_addr = 0;

    if (0 != ubnt_part_cache_in_ram(PART_LOOKUP_BOOTSELECT,
                CONFIG_SYS_LOAD_ADDR, &size)) {
        return -1;
    }
    memcpy(bootselect_buffer, CONFIG_SYS_LOAD_ADDR, size);

    p_share_data->bootselect_addr = (uintptr_t)bootselect_buffer;


    UBNT_PRINTF(false, "UBNT, read bootselect done.\n");

    return 0;
}
#endif

static int ubnt_validate_kernel_image(const void * img_addr) {
    char dtb_config_name[DTB_CFG_LEN];
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    if (bdinfo->boardid) {
        if (snprintf(dtb_config_name, sizeof(dtb_config_name),
                    "#config-%x", bdinfo->boardid)
                >= sizeof(dtb_config_name)) {
            UBNT_PRINTF(true, "WARNING: config buffer not large enough for '#config@%x'\n",
                    bdinfo->boardid);
            dtb_config_name[0] = '\0';
        }
    } else {
        dtb_config_name[0] = '\0';
    }
    if (dtb_config_name[0] == '#') {
		if (fit_conf_get_node(img_addr, dtb_config_name + 1) < 0) {
            UBNT_PRINTF(true, "Config %s not found, using default\n",
                    dtb_config_name);
            dtb_config_name[0] = '\0';
        }
    }

    // use bootm sub-commands to verify the image for us.
    if (CMD_RET_SUCCESS != run_commandf("validate", "bootm start 0x%p%s",
                img_addr, dtb_config_name)) {
        return KERNEL_VALIDATION_INVALID;
    }
    if (CMD_RET_SUCCESS != run_commandf("validate", "bootm loados")) {
        return KERNEL_VALIDATION_INVALID;
    }
    return KERNEL_VALIDATION_VALID;
}

static int get_image_size(unsigned int img_addr, uint32_t *size) {
    int image_type;
    int ret = -1;

    image_type = genimg_get_format((const void *)img_addr);
    switch (image_type) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
    case IMAGE_FORMAT_LEGACY:
        {
            image_header_t *hdr = (image_header_t *)img_addr;
            *size = uimage_to_cpu(hdr->ih_size) + sizeof(image_header_t);
            ret = 0;
        }
        break;
#endif
#if defined(CONFIG_FIT)
    case IMAGE_FORMAT_FIT:
        {
            struct fdt_header *fdt = (struct fdt_header *)img_addr;
            *size = fdt_totalsize(fdt);
            ret = 0;
        }
        break;
#endif
    default:
        break;
    }

    return ret;
}

static int ubnt_read_kernel_image(void) {
    char *kernel_part_name;
    ubnt_part_t part;
    uint32_t header_size = 0;
    uint32_t img_size;

    ubnt_app_continue(UBNT_EVENT_READING_FLASH);
#if !defined(CONFIG_FIT) && !defined(CONFIG_IMAGE_FORMAT_LEGACY)
#error "At least one of FIT or uImage must be enabled"
#endif
#if defined(CONFIG_FIT)
    header_size = sizeof(struct fdt_header);
#endif
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
    if (header_size < sizeof(image_header_t))
        header_size = sizeof(image_header_t);
#endif

    kernel_part_name = p_share_data->kernel_index ? PART_LOOKUP_KERNEL1 : PART_LOOKUP_KERNEL0;
    UBNT_PRINTF(false, "loading part %d: '%s'\n", p_share_data->kernel_index, kernel_part_name);
    if (0 != ubnt_part_get(kernel_part_name, &part)) {
        UBNT_PRINTF(true, "Failed to load part: %s\n", kernel_part_name);
        return KERNEL_VALIDATION_INVALID;
    }
    if (0 != ubnt_part_read(&part, CONFIG_SYS_LOAD_ADDR, 0, header_size))
        return KERNEL_VALIDATION_INVALID;
    if (0 != get_image_size(CONFIG_SYS_LOAD_ADDR, &img_size))
        img_size = part.part_size;

    if (0 != ubnt_part_read(&part, CONFIG_SYS_LOAD_ADDR, 0, img_size))
        return KERNEL_VALIDATION_INVALID;
    ubnt_app_continue(UBNT_EVENT_READING_FLASH);
    return ubnt_validate_kernel_image((void *)CONFIG_SYS_LOAD_ADDR);
#if 0
    unsigned int load_addr = CONFIG_SYS_LOAD_ADDR;
    unsigned int current_offset;
    int current_size, size_left, img_size = 0;
    loff_t offset;
    loff_t size;
    char runcmd[256];
    int ret;
    int image_valid = KERNEL_VALIDATION_INVALID;

    if (p_share_data->kernel_index != 0) {
        offset = flash_info->hloss[1].offset;
        size = flash_info->hloss[1].size;
    } else {
        offset = flash_info->hloss[0].offset;
        size = flash_info->hloss[0].size;
    }

    p_share_data->kernel_image_addr = load_addr;
    current_offset = (unsigned int)offset;
    size_left = (int)size;
    current_size = (size_left < SPI_FLASH_PAGE_SIZE) ? size_left : SPI_FLASH_PAGE_SIZE;

    /* read first page to find f/w size */
    snprintf(runcmd, sizeof(runcmd), "sf read 0x%x 0x%x 0x%x", load_addr, current_offset, current_size);
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash read error (Kernel), command: %s, ret: %d \n", runcmd, ret);
        return -1;
    }
    ret = get_image_size(load_addr, &img_size);
    if (ret == 0) {
        size_left = img_size < size ? img_size : size;
        /* align to page size */
        size_left = (size_left + SPI_FLASH_PAGE_SIZE) & (~(SPI_FLASH_PAGE_SIZE - 1));
    }
    printf("reading kernel %d from: 0x%llx, size: 0x%08x\n", p_share_data->kernel_index, offset, size_left);

    size_left -= current_size;
    load_addr += current_size;
    current_offset += current_size;

    while ( size_left > 0 ) {
        if ( size_left > UBNT_FLASH_SECTOR_SIZE) {
            current_size = UBNT_FLASH_SECTOR_SIZE;
        } else {
            current_size = size_left;
        }
        snprintf(runcmd, sizeof(runcmd), "sf read 0x%x 0x%x 0x%x", load_addr, current_offset, current_size);
        ret = run_command(runcmd, 0);
        if (ret != CMD_RET_SUCCESS) {
            printf(" *WARNING*: flash read error (Kernel), command: %s, ret: %d \n", runcmd, ret);
            return image_valid;
        }

        ubnt_app_continue(UBNT_EVENT_READING_FLASH);
        size_left -= current_size;
        load_addr += current_size;
        current_offset += current_size;
    }

    image_valid = ubnt_validate_kernel_image((void *)CONFIG_SYS_LOAD_ADDR);
    if (ubnt_debug) {
        printf("UBNT, read kernel image done, kernel valid: %d.\n", image_valid);
    }

    return image_valid;
#endif

}

#ifdef CONFIG_UBNT_HAS_GPT
static int ensure_valid_gpt(ubnt_app_update_table_t *update)
{
    int write_gpt = 0;
    ubnt_part_t part;
    if (update->gpt.addr == UBNT_INVALID_ADDR) {
        return 0;
    }
    if (update->kernel0.addr != UBNT_INVALID_ADDR) {
        if ((0 == ubnt_part_get(PART_LOOKUP_KERNEL0, &part)) ||
                (part.part_size < update->kernel0.size)) {
            write_gpt = 1;
        }
    }
#ifdef CONFIG_UBNT_HAS_KERNEL1
    if (update->kernel1.addr != UBNT_INVALID_ADDR) {
        if ((0 != ubnt_part_get(PART_LOOKUP_KERNEL1, &part)) ||
                (part.part_size < update->kernel1.size)) {
            write_gpt = 1;
        }
    }
#endif
    if (write_gpt) {
        return ubnt_part_update_image(PART_LOOKUP_GPT, &update->gpt);
    }
    return 0;
}
#endif

static int ubnt_app_start(int argc, char *const argv[]) {
    char tmp_str[64];
    int ret, i;
    int ubnt_app_event;
    int do_urescue = 0;
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();

    ret = ubnt_app_load();
    if (ret != 0) {
        return -1;
    }

    memcpy(&p_share_data->board_info, bdinfo, sizeof(p_share_data->board_info));
    ret = ubnt_read_bootselect();
    if (ret != 0) {
        /* do not return here to give urescue a chance to run */
        UBNT_PRINTF(true, " *WARNING*: read bootselect failed, ret: %d.\n", ret);
    }

    snprintf(tmp_str, sizeof(tmp_str), "go ${ubntaddr} uappinit");
    for (i = 0; i < argc; i++) {
        snprintf(tmp_str, sizeof(tmp_str), "%s %s", tmp_str, argv[i]);
    }

    UBNT_PRINTF(false, "UBNT, init command: %s\n", tmp_str);
    ret = run_command(tmp_str, 0);
    if (ret != 0) {
        /* do not return here to give urescue a chance to run */
        UBNT_PRINTF(true, " *WARNING*: uappinit failed, ret: %d.\n", ret);
        return -1;
    }

    ubnt_app_event = UBNT_EVENT_UNKNOWN;
    while(1) {
        ubnt_app_continue(ubnt_app_event);
        ubnt_app_event = UBNT_EVENT_UNKNOWN;

        if (ubnt_flag("ubnt_do_boot")) {
            env_set("ubnt_do_boot", NULL);
            break;
        }

        if (ubnt_flag("do_urescue")) {
            do_urescue = 1;
            break;
        } else {
            ret = ubnt_read_kernel_image();

            if (ret == KERNEL_VALIDATION_INVALID) {
                ubnt_app_event = UBNT_EVENT_KERNEL_INVALID;
            } else if (ret == KERNEL_VALIDATION_VALID) {
                ubnt_app_event = UBNT_EVENT_KERNEL_VALID;
            }
        }
    }

    if (ubnt_flag("ubnt_clearcfg")) {
        ubnt_part_erase_all(PART_LOOKUP_CFG);
    }

    if (ubnt_flag("ubnt_clearenv")) {
        ubnt_part_erase_all(PART_LOOKUP_UBOOT_ENV_FLASH);
#ifdef PART_LOOKUP_LOG
        ubnt_part_erase_all(PART_LOOKUP_LOG);
#endif
    }

    if (do_urescue) {
        ubnt_app_update_table_t *update = &p_share_data->update;
        if (ubnt_is_wireless_only_device()) {
            ubnt_part_erase_all(PART_LOOKUP_BOOTSELECT);
            ubnt_app_continue(UBNT_EVENT_USE_FACTORY_KERNEL);
            run_command("reset", 0);
        }
        // jeffrey
        //disable_poe_output();
        sprintf(tmp_str, "0x%x", CONFIG_SYS_LOAD_ADDR);
        env_set("loadaddr", tmp_str);
        while(1) {
            if (CMD_RET_FAILURE == run_command("tftpsrv", 0)) {
                /* Link could be down, just pretend we timeout */
                ubnt_app_continue(UBNT_EVENT_TFTP_TIMEOUT);
                mdelay(1000);
                continue;
            }
            ubnt_app_continue(UBNT_EVENT_TFTP_DONE);
            if (ubnt_flag("ubnt_fw_verified")) {
                break;
            }
#ifdef CONFIG_UBNT_LEDBAR
            ledbar_color_simple(0xff, 0, 0, LEDBAR_INTENSITY_FROM_ENV);
            mdelay(1000);
            ledbar_color_simple(0, 0, 0, LEDBAR_INTENSITY_FROM_ENV);
            mdelay(1000);
            ledbar_color_simple(0xff, 0, 0, LEDBAR_INTENSITY_FROM_ENV);
            mdelay(1000);
#endif
        }
#ifdef CONFIG_UBNT_LEDBAR
        ledbar_color_simple(0, 0xff, 0, LEDBAR_INTENSITY_FROM_ENV);
#endif
        /* has done urescue, need to do following
         * 1) update kernel 0
         * 2) erase vendor data partition
         * 3) erase cfg partition
         * 4) erase uboot environment variables
         */

        UBNT_PRINTF(false, "addresses: kernel0: 0x%08x, %d"
#ifdef CONFIG_UBNT_HAS_KERNEL1
                ", kernel1: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_BS
                ", bootselect: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_UBOOT
                ", uboot: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_SBL1
                ", sbl1: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_SBL2
                ", bl2: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_SBL3
                ", sbl3: 0x%08, %dx"
#endif
#ifdef CONFIG_UBNT_HAS_TZ
                ", tz: 0x%08x, %d"
#endif
#ifdef CONFIG_UBNT_HAS_RPM
                ", rpm: 0x%08, %d"
#endif
#ifdef CONFIG_UBNT_HAS_GPT
                ", gpt: 0x%08x, %d"
#endif
                "\n"
                , p_share_data->update.kernel0.addr
                , p_share_data->update.kernel0.size
#ifdef CONFIG_UBNT_HAS_KERNEL1
                , p_share_data->update.kernel1.addr
                , p_share_data->update.kernel1.size
#endif
#ifdef CONFIG_UBNT_HAS_BS
                , p_share_data->update.bootselect.addr
                , p_share_data->update.bootselect.size
#endif
#ifdef CONFIG_UBNT_HAS_UBOOT
                , p_share_data->update.uboot.addr
                , p_share_data->update.uboot.size
#endif
#ifdef CONFIG_UBNT_HAS_SBL1
                , p_share_data->update.sbl1.addr
                , p_share_data->update.sbl1.size
#endif
#ifdef CONFIG_UBNT_HAS_SBL2
                , p_share_data->update.bl2.addr
                , p_share_data->update.bl2.size
#endif
#ifdef CONFIG_UBNT_HAS_SBL3
                , p_share_data->update.sbl3.addr
                , p_share_data->update.sbl3s.size
#endif
#ifdef CONFIG_UBNT_HAS_TZ
                , p_share_data->update.tz.addr
#endif
#ifdef CONFIG_UBNT_HAS_RPM
                , p_share_data->update.rpm.addr
                , p_share_data->update.rpm.size
#endif
#ifdef CONFIG_UBNT_HAS_GPT
                , p_share_data->update.gpt.addr
                , p_share_data->update.gpt.size
#endif
                );
        ret = 0;
#ifdef CONFIG_UBNT_HAS_GPT
        if (ret == 0)
            ret = ensure_valid_gpt(update);
#endif
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_KERNEL0, &update->kernel0);
#ifdef CONFIG_UBNT_HAS_KERNEL1
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_KERNEL1, &update->kernel1);
#endif
#ifdef CONFIG_UBNT_HAS_BS
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_BOOTSELECT, &update->bootselect);
#endif
#ifdef CONFIG_UBNT_HAS_UBOOT
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_UBOOT, &update->uboot);
#endif
#ifdef CONFIG_UBNT_HAS_SBL1
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_SBL2, &update->sbl1);
#endif
#ifdef CONFIG_UBNT_HAS_SBL2
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_SBL2, &update->bl2);
#endif
#ifdef CONFIG_UBNT_HAS_SBL3
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_SBL3, &update->sbl3);
#endif
#ifdef CONFIG_UBNT_HAS_TZ
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_TZ, &update->tz);
#endif
#ifdef CONFIG_UBNT_HAS_RPM
        if (ret == 0)
            ret = ubnt_part_update_image(PART_LOOKUP_RPM, &update->rpm);
#endif

        /* reset system */
        run_command("reset", 0);
        return 0;
    }

    return 0;
}

void ubnt_app_continue(int event) {

    if (env_get("ubntaddr")) {
        run_commandf("uappcontinue", "go ${ubntaddr} uappcontinue %d", event);
    } else {
        UBNT_PRINTF(false, "ubnt_app_continue, ubntaddr not set. \n");
    }
}

#ifdef CONFIG_UBNT_PERSISTENT_LOG
static void set_bootargs_ramoops(int * pmem_size) {
    if (append_bootargs("ramoops.mem_address=0x%x ramoops.mem_size=%d ramoops.ecc=1 ramoops.record_size=%d",
            UBNT_RAM_OOPS_ADDRESS,
            UBNT_LOG_BUFF_SIZE, UBNT_LOG_RECORD_SIZE) != CMD_RET_SUCCESS)
        UBNT_PRINTF(true, "**WARNING**, buffer too small for ramoops bootargs.\n");

}
#endif

static int load_kernel(void)
{
    int bootid;
    int linux_mem = gd->ram_size;
    env_t env;
    uint32_t crc1, crc2, size = sizeof(crc2);

    if (ubnt_is_wireless_only_device()) {
        bootid = 0;
    } else {
        bootid = p_share_data->kernel_index;
    }

#ifdef CONFIG_UBNT_PERSISTENT_LOG
    set_bootargs_ramoops(&linux_mem);
#endif
    append_bootargs("console=ttyS0,115200n1 mem=%luK", linux_mem >> 10);
    append_bootargs("%s%d", UBNTBOOTID_ARG_STR, bootid);
    env_set("appinitdone", NULL);

    env_export(&env);
    crc1 = env.crc;
    ubnt_part_cache_in_ram(PART_LOOKUP_UBOOT_ENV_FLASH, &crc2, &size);

    UBNT_PRINTF(false, "[%s]Enviorment check, %x, %x\n", __FUNCTION__, crc1, crc2);

    if (ubnt_flag("ubnt_debug")) {
        run_command("printenv", 0);
        UBNT_PRINTF(false, "Booting from emmc\n");
    }

    if (crc1 != crc2)
        run_command("saveenv", 0);

    if (CMD_RET_SUCCESS != run_commandf("bootm kernel", "bootm prep")) {
        return CMD_RET_FAILURE;
    }

    return run_commandf("bootm kernel", "bootm go");
}

/**
 * set boot argument: uboot version
 */
static void set_bootargs_uboot_version(void) {
    const char uboot_version[] = U_BOOT_VERSION;
    char *ver_start, *ver_end_delimiter;
    int ver_str_len;

    /* uboot version example:
     * U-Boot 2012.07-00069-g7963d5b-dirty [UniFi,ubnt_feature_ubntapp.69]
     */
    ver_start = strchr(uboot_version, ',') + 1;
    ver_end_delimiter = strchr(uboot_version, ']');
    ver_str_len = ver_end_delimiter - ver_start;

    if (append_bootargs("%s%.*s", UBOOT_VER_ARG_STR, ver_str_len, ver_start)
            != CMD_RET_SUCCESS) {
        UBNT_PRINTF(true, "**WARNING**, buffer too small for uboot version bootargs.\n");
    }

    UBNT_PRINTF(false, "uboot version: %s\nbootargs: %s\n", uboot_version, env_get(ENV_BOOTARG));
}

static int do_bootubnt(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int ret;

    UBNT_PRINTF(true, "ubnt boot ...\n");
    env_set("bootargs", NULL);
    set_bootargs_uboot_version();

    ret = ubnt_app_start(--argc, &argv[1]);

    if (ret != 0) {
        UBNT_PRINTF(true, "UBNT app error.\n");
        return CMD_RET_FAILURE;
    }

    return load_kernel();
}

U_BOOT_CMD(bootubnt, 5, 0, do_bootubnt,
       "ubnt boot from flash device",
       "bootubnt - Load image(s) and boots the kernel\n");

static int do_urescue(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    env_set("do_urescue", UBNT_FLAG_TRUE);
    env_set("ubnt_do_boot", NULL);
    return do_bootubnt(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(urescue, 5, 0, do_urescue,
       "load ubiquiti FW using tftp server",
       "urescue - Start tftp server to load ubiquiti FW\n");

#define VERSION_LEN_MAX 128

uint8_t ubnt_prepare_beacon(uint8_t *buf) {
    uint8_t *pkt = buf;
    uint8_t *len_p;
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    uint16_t ubnt_ssid = htons(bdinfo->boardid);

    // Lengths
    uint8_t total_len;
    uint8_t header_len;
    uint8_t body_len;

    // U-Boot Version
    char *version = U_BOOT_VERSION;
    uint8_t version_len = strlen(version);
    if (version_len > VERSION_LEN_MAX)
        version_len = VERSION_LEN_MAX;

    /* header */
    *pkt++ = 0x03; // version
    *pkt++ = 0x07; // type (net rescue beacon)
    len_p = pkt++; // beacon size
    *pkt++ = 0x03; // bootloader version

    header_len = pkt - buf;

    /* uboot */
    *pkt++ = version_len;
    memcpy(pkt, version, version_len);
    pkt += version_len;

    /* board */
    memcpy(pkt, &ubnt_ssid, sizeof(ubnt_ssid));
    pkt += sizeof(ubnt_ssid);

    total_len = pkt - buf;
    body_len = total_len - header_len;

    // Include size in beacon.
    *len_p = body_len;

    return total_len;
}
