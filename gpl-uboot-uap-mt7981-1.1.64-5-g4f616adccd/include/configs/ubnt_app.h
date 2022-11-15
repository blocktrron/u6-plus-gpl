#ifndef _UBNT_APP_H
#define _UBNT_APP_H

#ifndef CONFIG_UBNT_APP_SIZE
#define CONFIG_UBNT_APP_SIZE (96 << 10)
#endif

#ifndef CONFIG_UBNT_APP_ADDR
#define CONFIG_UBNT_APP_ADDR 0x41000000
#endif

#define UBNT_APP_ADDR ((void *)CONFIG_UBNT_APP_ADDR)

#endif /* _UBNT_APP_H */
