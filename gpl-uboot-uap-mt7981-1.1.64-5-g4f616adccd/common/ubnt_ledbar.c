// vim: ts=4:sw=4:expandtab
#include <common.h>
#include <i2c.h>
#include <dm.h>
#include <ubnt_export.h>
#include <ubnt_ledbar.h>
#include <ubnt_util.h>

#define HT52241_APP_MODE 0
#define THUNDER_APP_MODE 1
#define DEFAULT_APP_MODE HT52241_APP_MODE

#define THUNDER_CMD_CONSTANT_LED 0x50
#define THUNDER_ACK 0x79
#define THUNDER_CODE_R 0x52
#define THUNDER_CODE_G 0x47
#define THUNDER_CODE_B 0x42

static int ht52241_write_color(struct udevice *dev, const ledbar_color_t *color);
static int thunder_write_color(struct udevice *dev, const ledbar_color_t *color);

static ledbar_dev_t *ledbar = NULL;

static ledbar_dev_t ledbar_table[] = {
    {
        .name = "ht52241",
        .addr = 0x30,
        .boot_app_mode_level = HT52241_APP_MODE,
        .write_color = ht52241_write_color,
    },
    {
        .name = "thunder",
        .addr = 0x38,
        .boot_app_mode_level = THUNDER_APP_MODE,
        .write_color = thunder_write_color,
    },
};

static void ledbar_reboot(int mode) {
    ubnt_plat_gpio_set(LEDBAR_GPIO_WRITE_EN, mode);
    ubnt_plat_gpio_set(LEDBAR_GPIO_RST, 0);
    mdelay(8);
    ubnt_plat_gpio_set(LEDBAR_GPIO_RST, 1);
}

int ledbar_reset(void)
{
    int ret;
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();

    if (!bdinfo->led_count) {
        // Not supported
        return 0;
    }

    ret = ubnt_plat_gpio_init(LEDBAR_GPIO_RST, 1);
    if (ret) {
        return ret;
    }

    ret = ubnt_plat_gpio_init(LEDBAR_GPIO_WRITE_EN, 1);
    if (ret) {
        return ret;
    }

    ledbar_reboot(DEFAULT_APP_MODE);

    return ret;
}

int ledbar_init(void)
{
    int tries, ret;
    int bus_no = LEDBAR_I2C_BUS;
    struct udevice *dev, *bus;
    int i;
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();

    if (!bdinfo->led_count) {
        // Not supported
        return 0;
    }

    ret = uclass_get_device_by_seq(UCLASS_I2C, bus_no, &bus);
    if (ret) {
        printf("%s: No I2C bus %d\n", __func__, bus_no);
        return ret;
    }

    for (tries = 100; tries; tries--) {
        for (i = 0; i < ARRAY_SIZE(ledbar_table); i++) {
            ret = dm_i2c_probe(bus, ledbar_table[i].addr, 0, &dev);
            if (ret == 0) {
                ledbar = &ledbar_table[i];
                break;
            }
        }
        if (ledbar) {
            break;
        }
        mdelay(LEDBAR_DELAY);
    }

    if (ret) {
        printf("WARNING: missing LEDBAR\n");
        return ret;
    }

    ret = i2c_set_chip_offset_len(dev, 0);
    if (ret) {
        printf("Error 'i2c_set_chip_offset_len': %d\n", ret);
        return ret;
    }

    if (ledbar->boot_app_mode_level != DEFAULT_APP_MODE) {
        ledbar_reboot(ledbar->boot_app_mode_level);
        mdelay(80);
        for (tries = 100; tries; tries--) {
            ret = dm_i2c_probe(bus, ledbar->addr, 0, &dev);
            if (ret == 0) {
                break;
            }
            mdelay(LEDBAR_DELAY);
        }
        if (ret) {
            printf("Error unable to set LEDBAR %s to app mode\n", ledbar->name);
            ledbar = NULL;
            return ret;
        }
    }

    printf("Found LEDBAR %s\n", ledbar->name);
    return ret;
}

static int ledbar_write_data(struct udevice *dev, const uint8_t *buffer, int len)
{
    int ret;
    ret = dm_i2c_write(dev, 0, buffer, len);
    if (ret) {
        printf("Error writing the ledbar: %d\n", ret);
    }
    return ret;
}

static int ledbar_read_byte(struct udevice *dev, uint8_t *value)
{
    int ret;
    ret = dm_i2c_read(dev, 0, value, 1);
    if (ret) {
        printf("Error reading the ledbar: %d\n", ret);
    }
    return ret;
}

static int ht52241_write_default_mode(struct udevice *dev, const ledbar_color_t *color)
{
    static const uint8_t led_bar_start[] = {0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    uint8_t led_bar_default_mode[] = {0x40, 0x00,
        color->blue,
        color->green,
        color->red,
        0x00, bdinfo->led_count, 0x00};
    uint8_t ledbar_response = 0;
    int ret;

    ret = ledbar_write_data(dev, led_bar_start, ARRAY_SIZE(led_bar_start));
    if (ret) {
        return -EAGAIN;
    }
    ret = ledbar_read_byte(dev, &ledbar_response);
    if (ret || (ledbar_response != STATUS_CHECK_PASS_BYTE)) {
        return -EAGAIN;
    }

    ret = ledbar_write_data(dev, led_bar_default_mode, ARRAY_SIZE(led_bar_default_mode));
    if (ret) {
        return -EAGAIN;
    }
    ret = ledbar_read_byte(dev, &ledbar_response);
    if (ret || (ledbar_response != STATUS_CHECK_PASS_BYTE)) {
        return -EAGAIN;
    }
    return 0;
}

static int ht52241_write_color(struct udevice *dev, const ledbar_color_t *color)
{
    int ret;
    ubnt_plat_gpio_set(LEDBAR_GPIO_WRITE_EN, 1);
    mdelay(LEDBAR_DELAY);
    ret = ht52241_write_default_mode(dev, color);
    ubnt_plat_gpio_set(LEDBAR_GPIO_WRITE_EN, 0);
    return ret;
}

static int thunder_cmd(struct udevice *dev, uint8_t cmd)
{
    uint8_t data[2] = {cmd, 0xFF - cmd};
    uint8_t ledbar_response = 0;
    int ret;

    ret = ledbar_write_data(dev, data, ARRAY_SIZE(data));
    if (ret) {
        return -EAGAIN;
    }
    ret = ledbar_read_byte(dev, &ledbar_response);
    if (ret || (ledbar_response != THUNDER_ACK)) {
        return -EAGAIN;
    }
    return 0;
}

static int thunder_write_color(struct udevice *dev, const ledbar_color_t *color)
{
    const ubnt_bdinfo_t *bdinfo = ubnt_bdinfo_get();
    uint8_t data[10] = {bdinfo->led_count, 0xFF - bdinfo->led_count,
        0, 0, 0, 0,
        THUNDER_CODE_B, color->blue,
        0, 0};
    uint8_t ledbar_response = 0;
    int ret;
    int is_rgb = 0;

    ret = thunder_cmd(dev, THUNDER_CMD_CONSTANT_LED);
    if (ret) {
        return -EAGAIN;
    }

    if (is_rgb) {
        data[2] = THUNDER_CODE_R;
        data[3] = color->red;
        data[4] = THUNDER_CODE_G;
        data[5] = color->green;
        data[8] = 27; // one_width
        data[9] = 7; // zero_width
    } else {
        data[2] = THUNDER_CODE_G;
        data[3] = color->green;
        data[4] = THUNDER_CODE_R;
        data[5] = color->red;
        data[8] = 33; // one_width
        data[9] = 12; // zero_width
    }

    ret = ledbar_write_data(dev, data, ARRAY_SIZE(data));
    if (ret) {
        return -EAGAIN;
    }
    ret = ledbar_read_byte(dev, &ledbar_response);
    if (ret || (ledbar_response != THUNDER_ACK)) {
        return -EAGAIN;
    }
    return 0;
}

void ledbar_color(const ledbar_color_t *color, int intensity)
{
    struct udevice *dev;
    int bus_no = LEDBAR_I2C_BUS;
    int tries;
    int ret;
    ledbar_color_t adjust_color;

    if (!ledbar) {
        return;
    }

    if (intensity == LEDBAR_INTENSITY_FROM_ENV) {
        intensity = getenv_ulong(LEDBAR_ENV_BRIGHT, 10, LEDBAR_BRIGHT_DEFAULT);
        if (intensity > LEDBAR_BRIGHT_MAX) {
            intensity = LEDBAR_BRIGHT_MAX;
        }
    }
    adjust_color.red = (color->red * intensity) >> 8;
    adjust_color.green = (color->green * intensity) >> 8;
    adjust_color.blue = (color->blue * intensity) >> 8;

    ret = i2c_get_chip_for_busnum(bus_no, ledbar->addr, 0, &dev);
    if (ret) {
        printf("%s: could not get chip for ledbar %d\n", __func__, bus_no);
        return;
    }

    ret = -EAGAIN;
    for (tries = LEDBAR_RETRIES; tries; tries--) {
        ret = ledbar->write_color(dev, &adjust_color);
        if (ret) {
            mdelay(LEDBAR_DELAY);
        } else {
            break;
        }
    }
    if (ret) {
        printf("%s: failed to write color\n", __func__);
    }
}

void ledbar_color_simple(uint8_t red, uint8_t green, uint8_t blue, int intensity)
{
    ledbar_color_t c = { .red = red, .green = green, .blue = blue };
    ledbar_color(&c, intensity);
}
