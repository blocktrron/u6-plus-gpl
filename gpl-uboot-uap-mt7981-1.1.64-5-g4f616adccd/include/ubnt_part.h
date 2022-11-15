#ifndef __UBNT_PART_H_
#define __UBNT_PART_H_
#include <ubnt_export.h>

enum ubnt_part_type {
    UBNT_PART_TYPE_UNKNOWN,
    UBNT_PART_TYPE_NOR,
    UBNT_PART_TYPE_EMMC,
    UBNT_PART_TYPE_EMMC_BOOT,
    UBNT_PART_TYPE_NAND_UBI,
};

typedef struct {
    enum ubnt_part_type type;
    const char *cmd_name;
    uint32_t cmd_blksize;
    uint32_t part_offset;
    uint32_t part_size;
} ubnt_part_t;

int ubnt_part_get_nor_nand(char *name, ubnt_part_t *part);
#ifdef CONFIG_QCA_MMC
int ubnt_part_get_emmc(char *name, ubnt_part_t *part);
#endif
int ubnt_part_get(char *name, ubnt_part_t *part);
int ubnt_part_read(ubnt_part_t *part, uintptr_t destination, uint32_t offset, size_t size);
int ubnt_part_write(ubnt_part_t *part, uintptr_t source, uint32_t offset, size_t size);
int ubnt_part_update(ubnt_part_t *part, uintptr_t source, uint32_t offset, size_t size);
int ubnt_part_erase(ubnt_part_t *part, uint32_t offset, size_t size);
int ubnt_part_erase_all(char *name);
int ubnt_part_cache_in_ram(char *name, uintptr_t ram_location, uint32_t *size);
int ubnt_part_cache_in_emmc(char *dst_name, char *src_name, uintptr_t ram_location, uint32_t *size);
int ubnt_part_update_image(char *name, ubnt_app_partition_t *update_part);
#endif /* __UBNT_PART_H_ */
