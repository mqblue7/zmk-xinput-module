/*
 * Copyright (c) 2025 ZMK XInput Module
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_xinput

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zmk/behavior.h>
#include <zmk/xinput.h>

LOG_MODULE_REGISTER(behavior_xinput, CONFIG_ZMK_LOG_LEVEL);

/* Track currently pressed XInput buttons */
static uint16_t pressed_buttons = 0;
static K_MUTEX_DEFINE(button_mutex);

struct behavior_xinput_config {
	uint16_t button_mask;
};

static int behavior_xinput_init(const struct device *dev)
{
	return 0;
}

static int on_xinput_binding_pressed(struct zmk_behavior_binding *binding,
				      struct zmk_behavior_binding_event event)
{
	const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
	const struct behavior_xinput_config *cfg = dev->config;

	k_mutex_lock(&button_mutex, K_FOREVER);
	
	/* Set the button bit */
	pressed_buttons |= cfg->button_mask;
	
	/* Send the updated state */
	int ret = zmk_xinput_send_report(pressed_buttons);
	
	k_mutex_unlock(&button_mutex);

	if (ret < 0) {
		LOG_ERR("Failed to send XInput press: %d", ret);
		return ret;
	}

	LOG_DBG("XInput button pressed: 0x%04x (total: 0x%04x)", 
		cfg->button_mask, pressed_buttons);
	
	return ZMK_BEHAVIOR_OPAQUE;
}

static int on_xinput_binding_released(struct zmk_behavior_binding *binding,
				       struct zmk_behavior_binding_event event)
{
	const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
	const struct behavior_xinput_config *cfg = dev->config;

	k_mutex_lock(&button_mutex, K_FOREVER);
	
	/* Clear the button bit */
	pressed_buttons &= ~cfg->button_mask;
	
	/* Send the updated state */
	int ret = zmk_xinput_send_report(pressed_buttons);
	
	k_mutex_unlock(&button_mutex);

	if (ret < 0) {
		LOG_ERR("Failed to send XInput release: %d", ret);
		return ret;
	}

	LOG_DBG("XInput button released: 0x%04x (total: 0x%04x)", 
		cfg->button_mask, pressed_buttons);
	
	return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_xinput_driver_api = {
	.binding_pressed = on_xinput_binding_pressed,
	.binding_released = on_xinput_binding_released,
};

#define XINPUT_INST(n)                                                                   \
	static struct behavior_xinput_config behavior_xinput_config_##n = {              \
		.button_mask = DT_INST_PROP(n, button),                                  \
	};                                                                               \
	BEHAVIOR_DT_INST_DEFINE(n, behavior_xinput_init, NULL, NULL,                     \
				&behavior_xinput_config_##n, POST_KERNEL,                \
				CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                     \
				&behavior_xinput_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XINPUT_INST)
