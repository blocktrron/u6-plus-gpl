#include <common.h>
#include <command.h>
#include <mmc.h>
#include <ubnt_part.h>
#include <ubnt_util.h>
#include <ubnt_export.h>

enum ubnt_part_action {
    PART_ACTION_READ = 0,
    PART_ACTION_ERASE = 1 << 1,
    PART_ACTION_WRITE = 1 << 2,
    PART_ACTION_UPDATE = PART_ACTION_ERASE | PART_ACTION_WRITE,
};

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_QCA_MMC
static qca_smem_flash_info_t *sfi = &qca_smem_flash_info;
extern qca_mmc mmc_host;
static qca_mmc *host = &mmc_host;
#else
#define EMMC_MAX_STOR_DEV   1
static struct blk_desc *emmc_dev_desc;
#endif

int ubnt_part_get_nor_nand(char *name, ubnt_part_t *part)
{
    int ret;
    uint32_t start, size;
#ifdef CONFIG_QCA_MMC
    qca_part_entry_t qca_part;
#endif

    part->type = UBNT_PART_TYPE_UNKNOWN;

#ifdef CONFIG_QCA_MMC
    ret = smem_getpart(name, &start, &size);
    if (ret < 0) {
        return -1;
    }
    qca_set_part_entry(name, sfi, &qca_part, start, size);
#endif

#ifdef CONFIG_USE_NAND_UBI
    if (get_which_flash_param(name) == 1) {
        part->type = UBNT_PART_TYPE_NAND_UBI;
        part->cmd_name = "ubi";
        part->cmd_blksize = get_nand_block_size(1);
#else
    if (0) {
#endif
#ifdef CONFIG_QCA_MMC
    } else {
        part->part_offset = qca_part.offset;
        part->part_size = qca_part.size;
#else
    } else if (0 == strcmp(name, PART_LOOKUP_UBOOT_ENV_FLASH)) {
        part->type = UBNT_PART_TYPE_NOR;
        part->cmd_name = "sf";
        part->cmd_blksize = 1;
        part->part_offset = 0x10000;
        part->part_size = 0x80000;
    } else if (0 == strcmp(name, PART_LOOKUP_EEPROM_FLASH)) {
        part->type = UBNT_PART_TYPE_NOR;
        part->cmd_name = "sf";
        part->cmd_blksize = 1;
        part->part_offset = 0x0;
        part->part_size = 0x10000;
#endif
    }

    return 0;
}

int ubnt_part_get_emmc(char *name, ubnt_part_t *part)
{
#ifdef CONFIG_QCA_MMC
    block_dev_desc_t *blk_dev;
    disk_partition_t disk_info;

    part->type = UBNT_PART_TYPE_UNKNOWN;

    mmc_initialize(gd->bd);
    blk_dev = mmc_get_dev(host->dev_num);

    if (!blk_dev) {
        return -1;
    }
#else
    //struct blk_desc *blk_dev_desc = NULL;

    part->type = UBNT_PART_TYPE_UNKNOWN;
    mmc_initialize(gd->bd);

    //mmc_get_dev(0, &blk_dev_desc);
    //if (!blk_dev_desc) {
    //    return -1;
    //}
#endif

#ifdef CONFIG_QCA_MMC
    if (get_partition_info_efi_by_name(blk_dev,
                name, &disk_info) >= 0) {
        part->part_offset = disk_info.start * blk_dev->blksz;
        part->part_size = disk_info.size * blk_dev->blksz;
    } else if (strcmp(name, PART_LOOKUP_GPT) == 0) {
        part->part_offset = 0;
        part->part_size = CONFIG_UBNT_GPT_SIZE;
    } else {
        return -1;
    }
#else // jeffrey, FIXME: using GPT API to read GPT partition instead
    part->type = UBNT_PART_TYPE_UNKNOWN;
    if (strcmp(name, PART_LOOKUP_GPT) == 0) {
        part->part_offset = GET_PART_OFFSET(GPT);
        part->part_size = GET_PART_SIZE(GPT);
    } else if (strcmp(name, PART_LOOKUP_SBL2) == 0) {
        part->part_offset = GET_PART_OFFSET(SBL2);
        part->part_size = GET_PART_SIZE(SBL2);
        part->type = UBNT_PART_TYPE_EMMC_BOOT;
        part->cmd_name = "mtkupgrade";
    } else if (strcmp(name, PART_LOOKUP_UBOOT_ENV) == 0) {
        part->part_offset = GET_PART_OFFSET(UBOOT_ENV);
        part->part_size = GET_PART_SIZE(UBOOT_ENV);
    } else if (strcmp(name, PART_LOOKUP_UBOOT_ENV) == 0) {
        part->part_offset = GET_PART_OFFSET(UBOOT_ENV);
        part->part_size = GET_PART_SIZE(UBOOT_ENV);
    } else if (strcmp(name, PART_LOOKUP_FACTORY) == 0) {
        part->part_offset = GET_PART_OFFSET(FACTORY);
        part->part_size = GET_PART_SIZE(FACTORY);
    } else if (strcmp(name, PART_LOOKUP_UBOOT) == 0) {
        part->part_offset = GET_PART_OFFSET(UBOOT);
        part->part_size = GET_PART_SIZE(UBOOT);
    }  else if (strcmp(name, PART_LOOKUP_EEPROM) == 0) {
        part->part_offset = GET_PART_OFFSET(EEPROM);
        part->part_size = GET_PART_SIZE(EEPROM);
    } else if (strcmp(name, PART_LOOKUP_KERNEL0) == 0) {
        part->part_offset = GET_PART_OFFSET(KERNEL0);
        part->part_size = GET_PART_SIZE(KERNEL0);
    } else if (strcmp(name, PART_LOOKUP_KERNEL1) == 0) {
        part->part_offset = GET_PART_OFFSET(KERNEL1);
        part->part_size = GET_PART_SIZE(KERNEL1);
    } else if (strcmp(name, PART_LOOKUP_BOOTSELECT) == 0) {
        part->part_offset = GET_PART_OFFSET(BS);
        part->part_size = GET_PART_SIZE(BS);
    } else if (strcmp(name, PART_LOOKUP_CFG) == 0) {
        part->part_offset = GET_PART_OFFSET(CFG);
        part->part_size = GET_PART_SIZE(CFG);
    } else if (strcmp(name, PART_LOOKUP_LOG) == 0) {
        part->part_offset = GET_PART_OFFSET(LOG);
        part->part_size = GET_PART_SIZE(LOG);
    } else {
        return -1;
    }
#endif

    if (UBNT_PART_TYPE_UNKNOWN == part->type) {
        part->type = UBNT_PART_TYPE_EMMC;
        part->cmd_name = "mmc";
    }

#ifdef CONFIG_QCA_MMC
    part->cmd_blksize = blk_dev->blksz;
#else 
    //FIXME: jeffrey
    part->cmd_blksize = 512;
#endif

    return 0;
}

int ubnt_part_get(char *name, ubnt_part_t *part)
{
	int ret;

    if ((0 == strcmp(name, PART_LOOKUP_UBOOT_ENV_FLASH)) ||
            (0 == strcmp(name, PART_LOOKUP_EEPROM_FLASH)))
        ret = ubnt_part_get_nor_nand(name, part);
    else
        ret = ubnt_part_get_emmc(name, part);

    if (!ret) {
        UBNT_PRINTF(false, "[%s]Get part info : [%s]offset : %08X, size : %d\r\n",
            __FUNCTION__,
            name,
            part->part_offset,
            part->part_size);
    } else {
        UBNT_PRINTF(true, "[%s] Get part info fail\r\n", __FUNCTION__);
    }

    return ret;
}

static int handle_part_action_nor(ubnt_part_t *part, const char *sub_cmd, uint32_t action, uintptr_t ram_location, uint32_t offset, size_t size)
{
    int ret;
    static uint8_t sf_probe_done = 0;
    if (!sf_probe_done) {
        if (CMD_RET_SUCCESS != run_commandf(sub_cmd, "%s probe", part->cmd_name)) {
            return -1;
        }

        sf_probe_done = 1;
    }
    if (action == PART_ACTION_ERASE) {
        ret = run_commandf(sub_cmd, "%s %s 0x%x +0x%x",
                part->cmd_name,
                sub_cmd,
                offset,
                size);
    } else {
        ret = run_commandf(sub_cmd, "%s %s 0x%x 0x%x 0x%x",
                part->cmd_name,
                sub_cmd,
                ram_location,
                offset,
                size);
    }
    return (ret == CMD_RET_SUCCESS) ? 0 : -1;
}

#ifdef CONFIG_USE_NAND_UBI
#define UBI_DEFAULT_VOLUME "vol"
static int handle_part_action_ubi(ubnt_part_t *part, const char *sub_cmd, uint32_t action, uintptr_t ram_location, uint32_t offset, size_t size)
{
    int ret;
    static uint32_t last_offset = 0xffffffff;

    if (part->part_offset != offset) {
        UBNT_PRINTF(true, "%s: doesn't support offset (%x)\n", __func__, offset - part->part_offset);
        return -1;
    }

    if (action == PART_ACTION_ERASE) {
        UBNT_PRINTF(true, "%s: doesn't support erase\n", __func__);
        return -1;
    }

    if (last_offset != part->part_offset) {
        if (CMD_RET_SUCCESS != run_commandf(sub_cmd,
                "nand device 1 && "
                "setenv mtdids nand1=nand1 && "
                "setenv mtdparts mtdparts=nand1:0x%x@0x%x(fs) && "
                "ubi part fs",
                part->part_size, part->part_offset)) {
            return -1;
        }
        if (CMD_RET_SUCCESS != run_commandf(sub_cmd,
                "ubi check %s || "
                "ubi create %s",
                UBI_DEFAULT_VOLUME, UBI_DEFAULT_VOLUME)) {
            return -1;
        }
        last_offset = part->part_offset;
    }

    ret = run_commandf(sub_cmd, "%s %s 0x%x %s 0x%x",
            part->cmd_name,
            sub_cmd,
            ram_location,
            UBI_DEFAULT_VOLUME,
            size);
    return (ret == CMD_RET_SUCCESS) ? 0 : -1;
}
#endif

static int handle_part_action_emmc(ubnt_part_t *part, const char *sub_cmd, uint32_t action, uintptr_t ram_location, uint32_t offset, size_t size)
{
    uint32_t blk_offset;
    uint32_t blks;

    if (action == PART_ACTION_UPDATE) {
        /* on emmc 'update' is a synonym for 'write' */
        sub_cmd = "write";
    }


    if (!size)
        return 0;

    if (offset % part->cmd_blksize) {
        UBNT_PRINTF(true, "[%s]Offset(%d) is not a multiple of blcok(%d)\r\n", __FUNCTION__, offset, part->cmd_blksize);
        return -1;
    }

    /* This section handles full blocks */
    blk_offset = offset / part->cmd_blksize;
    blks = size / part->cmd_blksize;
    if (!blks) {
        blks = 1;
    } else if (size % part->cmd_blksize) {
        blks = blks + 1;
    }

    UBNT_PRINTF(false, "[%s]Blk_offset : %x, blks : %d\r\n", __FUNCTION__, blk_offset, blks);
    if (action == PART_ACTION_ERASE) {
        if (run_commandf(sub_cmd, "%s %s 0x%x 0x%x",
                part->cmd_name,
                sub_cmd,
                blk_offset,
                blks) != CMD_RET_SUCCESS) {
            return -1;
        }
    } else {
        if (run_commandf(sub_cmd, "%s %s 0x%x 0x%x 0x%x",
                part->cmd_name,
                sub_cmd,
                ram_location,
                blk_offset,
                blks) != CMD_RET_SUCCESS) {
            return -1;
        }
    }

    return 0;
}

static int handle_part_action_emmc_boot(ubnt_part_t *part, const char *sub_cmd, uint32_t action, uintptr_t ram_location, uint32_t offset, size_t size)
{
    if (run_commandf(sub_cmd, "%s %s 0x%x 0x%x",
            part->cmd_name,
            "bl2",
            ram_location,
            size) != CMD_RET_SUCCESS) {
        return -1;
    }

    return 0;
}

static int handle_part_action(ubnt_part_t *part, uint32_t action, uintptr_t ram_location, uint32_t offset, size_t size)
{
    const char *sub_cmd;
    offset += part->part_offset;
    switch (action) {
        case PART_ACTION_READ:
            sub_cmd = "read";
            break;
        case PART_ACTION_ERASE:
            sub_cmd = "erase";
            break;
        case PART_ACTION_WRITE:
            sub_cmd = "write";
            break;
        case PART_ACTION_UPDATE:
            sub_cmd = "update";
            break;
        default:
            UBNT_PRINTF(true, "Invalid part action\n");
            return -1;
    }
    if (part->type == UBNT_PART_TYPE_NOR) {
        return handle_part_action_nor(part, sub_cmd, action, ram_location, offset, size);
    } else if (part->type == UBNT_PART_TYPE_EMMC) {
        return handle_part_action_emmc(part, sub_cmd, action, ram_location, offset, size);
    } else if (part->type == UBNT_PART_TYPE_EMMC_BOOT) {
        if ((PART_ACTION_WRITE) == action || (PART_ACTION_UPDATE) == action)
            return handle_part_action_emmc_boot(part, sub_cmd, action, ram_location, offset, size);
    }
#ifdef CONFIG_USE_NAND_UBI
    if (part->type == UBNT_PART_TYPE_NAND_UBI)
        return handle_part_action_ubi(part, sub_cmd, action, ram_location, offset, size);
#endif
    return -1;
}

int ubnt_part_read(ubnt_part_t *part, uintptr_t destination, uint32_t offset, size_t size)
{
    return handle_part_action(part, PART_ACTION_READ, destination, offset, size);
}

int ubnt_part_write(ubnt_part_t *part, uintptr_t source, uint32_t offset, size_t size)
{
    return handle_part_action(part, PART_ACTION_WRITE, source, offset, size);
}

int ubnt_part_update(ubnt_part_t *part, uintptr_t source, uint32_t offset, size_t size)
{
    return handle_part_action(part, PART_ACTION_UPDATE, source, offset, size);
}

int ubnt_part_erase(ubnt_part_t *part, uint32_t offset, size_t size)
{
    return handle_part_action(part, PART_ACTION_ERASE, 0, offset, size);
}

int ubnt_part_erase_all(char *name)
{
    ubnt_part_t part;

    UBNT_PRINTF(false, "Erasing %s partition ...... ", name);
    if (0 != ubnt_part_get(name, &part)) {
        UBNT_PRINTF(true, " *WARNING*: Partition %s not found\n", name);
        return -1;
    }
    if (0 != ubnt_part_erase(&part, 0, part.part_size)) {
        UBNT_PRINTF(true, " *WARNING*: Failed erasing partition %s\n", name);
        return -1;
    }
    UBNT_PRINTF(false, "done.\n");

    return 0;
}

int ubnt_part_cache_in_ram(char *name, uintptr_t ram_location, uint32_t *size)
{
    ubnt_part_t part;

    if (0 != ubnt_part_get(name, &part)) {
        UBNT_PRINTF(false, " *WARNING*: Partition %s not found\n", name);
        return -1;
    }
    if (*size > part.part_size) {
        *size = part.part_size;
    }

    if (0 != ubnt_part_read(&part, ram_location, 0, *size)) {
        UBNT_PRINTF(false, " *WARNING*: Failed caching partition %s to %#x\n", name, ram_location);
        return -1;
    }
    return 0;
}

int ubnt_part_cache_in_emmc(char *dst_name, char *src_name, uintptr_t ram_location, uint32_t *size)
{
    ubnt_part_t part = {};

    if (0 != ubnt_part_get(src_name, &part)) {
        UBNT_PRINTF(true, " *WARNING*: Source partition %s not found\n", src_name);
        return -1;
    }
    if (*size > part.part_size) {
        *size = part.part_size;
    }
    if (0 != ubnt_part_read(&part, ram_location, 0, *size)) {
        UBNT_PRINTF(true, " *WARNING*: Failed reading source partition %s to RAM buffer %#x\n", src_name, ram_location);
        return -1;
    }

    if (0 != ubnt_part_get(dst_name, &part)) {
        UBNT_PRINTF(true, " *WARNING*: Destination partition %s not found\n", dst_name);
        return -1;
    }

    if (0 != ubnt_part_write(&part, ram_location, 0, *size)) {
        UBNT_PRINTF(true, " *WARNING*: Failed caching partition form RAM buffer %#x to %s\n", ram_location, dst_name);
        return -1;
    }

    return 0;
}

int ubnt_part_update_image(char *name, ubnt_app_partition_t *update_part)
{
    int tries = 5;
    ubnt_part_t part;

    if (update_part->addr == UBNT_INVALID_ADDR) {
        return 0;
    }

    UBNT_PRINTF(false, "Updating %s partition ......\n ", name);
    if (0 != ubnt_part_get(name, &part)) {
        UBNT_PRINTF(true, " *WARNING*: Partition %s not found\n", name);
        return -1;
    }
    if (update_part->size > part.part_size) {
        UBNT_PRINTF(true, " *WARNING*: Partition %s(%#x) cannot fit image of size %#x\n",
                name, part.part_size, update_part->size);
        return -1;
    }
    for (tries = 5; tries; tries--) {
        if (0 != ubnt_part_update(&part, update_part->addr, 0, update_part->size)) {
            UBNT_PRINTF(true, " *WARNING*: Failed updating partition %s from %#x\n",
                    name, update_part->addr);
            continue;
        }
        UBNT_PRINTF(false, "done.\n");
        return 0;
    }
    return -1;
}
