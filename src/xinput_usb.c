#include <zmk/xinput.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(zmk_xinput, CONFIG_ZMK_XINPUT_LOG_LEVEL);

#define XINPUT_EP_IN    0x81
#define XINPUT_EP_OUT   0x01
#define XINPUT_EP_MPS   0x20
#define XINPUT_POLL_INT 0x04

static K_MUTEX_DEFINE(xinput_mutex);
static bool xinput_configured = false;
static uint8_t out_buf[XINPUT_EP_MPS];

static struct xinput_report current_report = {
    .type   = 0x00,
    .length = 0x14,
};

static void xinput_status_cb(struct usb_cfg_data *cfg,
                              enum usb_dc_status_code status,
                              const uint8_t *param)
{
    ARG_UNUSED(cfg);
    ARG_UNUSED(param);
    switch (status) {
    case USB_DC_CONFIGURED:
        xinput_configured = true;
        usb_transfer(XINPUT_EP_OUT, out_buf, sizeof(out_buf),
                     USB_TRANS_READ, NULL, NULL);
        break;
    case USB_DC_DISCONNECTED:
    case USB_DC_RESET:
        xinput_configured = false;
        break;
    default:
        break;
    }
}

static int xinput_vendor_handler(struct usb_setup_packet *setup,
                                  int32_t *len, uint8_t **data)
{
    ARG_UNUSED(setup);
    ARG_UNUSED(len);
    ARG_UNUSED(data);
    return -ENOTSUP;
}

struct xinput_config_desc {
    struct usb_if_descriptor  if0;
    uint8_t                   custom[16];
    struct usb_ep_descriptor  ep_in;
    struct usb_ep_descriptor  ep_out;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct xinput_config_desc xinput_desc = {
    .if0 = {
        .bLength            = sizeof(struct usb_if_descriptor),
        .bDescriptorType    = USB_DESC_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = 0xFF,
        .bInterfaceSubClass = 0x5D,
        .bInterfaceProtocol = 0x01,
        .iInterface         = 0x00,
    },
    .custom = {
        0x10, 0x21, 0x10, 0x01, 0x01, 0x24,
        0x81, 0x14, 0x00, 0x00, 0x00, 0x00,
        0x13, 0x01, 0x01, 0x08,
    },
    .ep_in = {
        .bLength            = sizeof(struct usb_ep_descriptor),
        .bDescriptorType    = USB_DESC_ENDPOINT,
        .bEndpointAddress   = XINPUT_EP_IN,
        .bmAttributes       = USB_DC_EP_INTERRUPT,
        .wMaxPacketSize     = XINPUT_EP_MPS,
        .bInterval          = XINPUT_POLL_INT,
    },
    .ep_out = {
        .bLength            = sizeof(struct usb_ep_descriptor),
        .bDescriptorType    = USB_DESC_ENDPOINT,
        .bEndpointAddress   = XINPUT_EP_OUT,
        .bmAttributes       = USB_DC_EP_INTERRUPT,
        .wMaxPacketSize     = XINPUT_EP_MPS,
        .bInterval          = XINPUT_POLL_INT,
    },
};

static struct usb_ep_cfg_data xinput_ep_data[] = {
    { .ep_cb = NULL, .ep_addr = XINPUT_EP_IN  },
    { .ep_cb = NULL, .ep_addr = XINPUT_EP_OUT },
};

static struct usb_cfg_data xinput_usb_cfg = {
    .usb_device_description = NULL,
    .interface_descriptor   = &xinput_desc.if0,
    .cb_usb_status          = xinput_status_cb,
    .interface = {
        .class_handler  = NULL,
        .custom_handler = xinput_vendor_handler,
        .vendor_handler = xinput_vendor_handler,
    },
    .num_endpoints = ARRAY_SIZE(xinput_ep_data),
    .endpoint      = xinput_ep_data,
};

static void xinput_send_report(void)
{
    if (!xinput_configured) {
        return;
    }
    uint8_t buf[sizeof(struct xinput_report)];
    k_mutex_lock(&xinput_mutex, K_FOREVER);
    memcpy(buf, &current_report, sizeof(buf));
    k_mutex_unlock(&xinput_mutex);
    usb_write(XINPUT_EP_IN, buf, sizeof(buf), NULL);
}

void zmk_xinput_press(uint16_t buttons)
{
    k_mutex_lock(&xinput_mutex, K_FOREVER);
    current_report.buttons |= buttons;
    k_mutex_unlock(&xinput_mutex);
    xinput_send_report();
}

void zmk_xinput_release(uint16_t buttons)
{
    k_mutex_lock(&xinput_mutex, K_FOREVER);
    current_report.buttons &= ~buttons;
    k_mutex_unlock(&xinput_mutex);
    xinput_send_report();
}

void zmk_xinput_set_axes(int16_t lx, int16_t ly, int16_t rx, int16_t ry)
{
    k_mutex_lock(&xinput_mutex, K_FOREVER);
    current_report.left_x  = lx;
    current_report.left_y  = ly;
    current_report.right_x = rx;
    current_report.right_y = ry;
    k_mutex_unlock(&xinput_mutex);
    xinput_send_report();
}

void zmk_xinput_set_triggers(uint8_t left, uint8_t right)
{
    k_mutex_lock(&xinput_mutex, K_FOREVER);
    current_report.left_trigger  = left;
    current_report.right_trigger = right;
    k_mutex_unlock(&xinput_mutex);
    xinput_send_report();
}

/*
 * Register at POST_KERNEL so XInput class is registered BEFORE
 * ZMK calls usb_enable() at APPLICATION level.
 * Do NOT call usb_enable() here - let ZMK do it.
 */
int zmk_xinput_usb_init(void)
{
    /* USBD_CLASS_DESCR_DEFINE registers the class automatically at link time.
     * No explicit registration call needed in Zephyr 4.1. */
    return 0;
}

SYS_INIT(zmk_xinput_usb_init, POST_KERNEL, 0);
