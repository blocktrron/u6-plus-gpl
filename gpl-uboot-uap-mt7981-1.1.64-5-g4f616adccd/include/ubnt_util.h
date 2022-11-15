#ifndef __UBNT_UTIL_H_
#define __UBNT_UTIL_H_

#define ENV_BOOTARG "bootargs"

int append_bootargs(char * fmt, ...);
int run_commandf(const char *description, const char *fmt, ...);
int ubnt_use_default_env(void);

int ubnt_plat_gpio_init(int gpio, int is_output);
void ubnt_plat_gpio_set(int gpio, int value);
int ubnt_plat_gpio_get(int gpio);

#endif /* __UBNT_UTIL_H_ */
