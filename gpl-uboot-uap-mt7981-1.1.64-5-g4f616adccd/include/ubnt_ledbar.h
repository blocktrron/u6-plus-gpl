#ifndef __UBNT_LEDBAR_H_
#define __UBNT_LEDBAR_H_

#define LEDBAR_ENV_BRIGHT "ledbar_bright"
#define LEDBAR_BRIGHT_MAX 256
#define LEDBAR_INTENSITY_FROM_ENV -1

#define LEDBAR_RETRIES 30
#define LEDBAR_DELAY 10 /* milliseconds */

#ifndef LEDBAR_BRIGHT_DEFAULT
#define LEDBAR_BRIGHT_DEFAULT LEDBAR_BRIGHT_MAX
#endif

#define STATUS_CHECK_PASS_BYTE 0xaa

typedef struct ledbar_color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ledbar_color_t;

typedef struct ledbar_dev ledbar_dev_t;
struct ledbar_dev {
    const char *name;
    uint8_t addr;
    uint8_t boot_app_mode_level;
    int (*write_color)(struct udevice *dev, const ledbar_color_t *color);
};

int ledbar_reset(void);
int ledbar_init(void);
void ledbar_color(const ledbar_color_t *color, int intensity);
void ledbar_color_simple(uint8_t red, uint8_t green, uint8_t blue, int intensity);

#endif /* __UBNT_LEDBAR_H_ */
