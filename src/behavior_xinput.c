#define DT_DRV_COMPAT zmk_behavior_xinput

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/xinput.h>

LOG_MODULE_DECLARE(zmk_xinput, CONFIG_ZMK_XINPUT_LOG_LEVEL);

static int behavior_xinput_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event)
{
    zmk_xinput_press((uint16_t)binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_xinput_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event)
{
    zmk_xinput_release((uint16_t)binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_xinput_driver_api = {
    .binding_pressed  = behavior_xinput_pressed,
    .binding_released = behavior_xinput_released,
};

static int behavior_xinput_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    return 0;
}

#define XINPUT_INST(n) \
    BEHAVIOR_DT_INST_DEFINE(n,                         \
        behavior_xinput_init,                          \
        NULL, NULL, NULL,                              \
        POST_KERNEL,                                   \
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,           \
        &behavior_xinput_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XINPUT_INST)
