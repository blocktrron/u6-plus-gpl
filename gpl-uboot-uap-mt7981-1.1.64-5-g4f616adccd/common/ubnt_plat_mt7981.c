// vim: ts=4:sw=4:expandtab
#include <common.h>
/* #include <dt-bindings/qcom/gpio-ipq40xx.h> */
#include <ubnt_util.h>
#if defined(CONFIG_UBNT_LED_ARRAY) || defined(CONFIG_UBNT_POE_CONTROL)
#include <pca953x.h>
#endif

int ubnt_plat_gpio_init(int gpio, int is_output)
{
#if 0
    struct qca_gpio_config gpio_config = {
        .gpio = gpio,
        .func = 0,
        .out = GPIO_OUT_HIGH,
        .pull = GPIO_PULL_UP,
        .drvstr = GPIO_8MA,
        .oe = is_output ? GPIO_OE_ENABLE : GPIO_OE_DISABLE,
    };

    gpio_tlmm_config(&gpio_config);
#endif
    return 0;
}

void ubnt_plat_gpio_set(int gpio, int value)
{
    gpio_set_value(gpio, value);
}

int ubnt_plat_gpio_get(int gpio)
{
    return gpio_get_value(gpio);
}

#if defined(CONFIG_UBNT_LED_ARRAY) || defined(CONFIG_UBNT_POE_CONTROL)
int ubnt_pca953x_gpios_set(uint gpios, uint vals)
{
	return pca953x_set_gpios_output_val(gpios, vals);
}
#endif
