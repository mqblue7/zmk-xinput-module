#pragma once

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

struct xinput_report {
    uint8_t  type;
    uint8_t  length;
    uint16_t buttons;
    uint8_t  left_trigger;
    uint8_t  right_trigger;
    int16_t  left_x;
    int16_t  left_y;
    int16_t  right_x;
    int16_t  right_y;
    uint8_t  reserved[6];
} __packed;

void zmk_xinput_press(uint16_t buttons);
void zmk_xinput_release(uint16_t buttons);
void zmk_xinput_set_axes(int16_t lx, int16_t ly, int16_t rx, int16_t ry);
void zmk_xinput_set_triggers(uint8_t left, uint8_t right);
int  zmk_xinput_usb_init(void);
