/*
 * Copyright (c) 2025 ZMK XInput Module
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

/**
 * @brief Send XInput button state report
 * 
 * @param buttons Button bitmask (see dt-bindings/zmk/xinput.h)
 * @return 0 on success, negative error code on failure
 */
int zmk_xinput_send_report(uint16_t buttons);
