#include <common.h>
#include <command.h>
#include <ubnt_export.h>
#include <ubnt_util.h>

#define MAX_BOOTARG_STRING_LENGTH 256

int append_bootargs(char * fmt, ...)
{
    int w_idx = 0;
    char *env_var_str;
    va_list args;
    char bootargs[MAX_BOOTARG_STRING_LENGTH + 1];
    char tmp[MAX_BOOTARG_STRING_LENGTH + 1];

    env_var_str = env_get(ENV_BOOTARG);

    if(env_var_str) {
        w_idx = snprintf(bootargs, sizeof(bootargs) - 1, "%s ", env_var_str);
        if (w_idx >= sizeof(bootargs)) {
            UBNT_PRINTF(true, "**WARNING**, bootargs do not fit in buffer.\n");
            return CMD_RET_FAILURE;
        }
    }
    bootargs[w_idx] = 0;

    w_idx = 0;
    va_start(args, fmt);
    w_idx = vsnprintf(tmp, (size_t)sizeof(tmp) - 1, fmt, args);
    va_end(args);
    tmp[w_idx] = 0;

    if ((sizeof(bootargs) - 1) < (strlen(tmp) + strlen(bootargs))) {
        UBNT_PRINTF(true, "**WARNING**, cannot append bootargs, buffer too small.\n");
        return CMD_RET_FAILURE;
    } else {
        strcat(bootargs, tmp);
    }

    env_set(ENV_BOOTARG, bootargs);

    return CMD_RET_SUCCESS;
}

int run_commandf(const char *description, const char *fmt, ...)
{
    char runcmd[256];
    va_list args;
    int res;
    va_start(args, fmt);
    res = vsnprintf(runcmd, sizeof(runcmd), fmt, args);
    va_end(args);
    if (res >= sizeof(runcmd)) {
        UBNT_PRINTF(true, "ERROR not enough buffer for cmd %s\n", description);
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        putc('\n');
        return CMD_RET_FAILURE;
    }

    UBNT_PRINTF(false, "run command %s\n", runcmd);

    res = run_command(runcmd, 0);
    if (res != CMD_RET_SUCCESS) {
        UBNT_PRINTF(true, "ERROR %s: %s\n", description, runcmd);
    }
    return res;
}

int ubnt_use_default_env(void)
{
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    if (bdinfo->reset_button_gpio == GPIO_NOT_AVAILABLE) {
        return 0;
    }
    if (0 != ubnt_plat_gpio_init(bdinfo->reset_button_gpio, 0)) {
        UBNT_PRINTF(true, "FAILED to init reset GPIO\n");
        return 0;
    }
    if (0 != ubnt_plat_gpio_get(bdinfo->reset_button_gpio)) {
        return 0;
    }
    UBNT_PRINTF(false, "Reset button pressed, using default env\n");
    return 1;
}
