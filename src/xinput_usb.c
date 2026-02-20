/*
 * Copyright (c) 2025 ZMK XInput Module
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <zmk/xinput.h>

LOG_MODULE_REGISTER(xinput_usb, CONFIG_ZMK_LOG_LEVEL);

#define XINPUT_VID 0x045E  /* Microsoft */
#define XINPUT_PID 0x028E  /* Xbox 360 Controller */

/* XInput interface descriptor indexes */
#define XINPUT_INTERFACE_NUM 2
#define XINPUT_IN_EP_ADDR 0x83
#define XINPUT_OUT_EP_ADDR 0x03

/* XInput packet structure for button states */
struct xinput_report {
	uint8_t report_id;
	uint8_t size;
	uint16_t buttons;
	uint8_t left_trigger;
	uint8_t right_trigger;
	int16_t left_stick_x;
	int16_t left_stick_y;
	int16_t right_stick_x;
	int16_t right_stick_y;
	uint8_t reserved[6];
} __packed;

static struct xinput_report current_report = {
	.report_id = 0x00,
	.size = 0x14,
	.buttons = 0x0000,
	.left_trigger = 0,
	.right_trigger = 0,
	.left_stick_x = 0,
	.left_stick_y = 0,
	.right_stick_x = 0,
	.right_stick_y = 0,
	.reserved = {0, 0, 0, 0, 0, 0}
};

static K_SEM_DEFINE(xinput_sem, 1, 1);
static bool xinput_configured = false;

/* USB device descriptor */
USBD_DEVICE_DESCR_DEFINE(primary) struct usb_device_descriptor xinput_dev_desc = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType = USB_DESC_DEVICE,
	.bcdUSB = 0x0200,  /* USB 2.0 */
	.bDeviceClass = 0xFF,  /* Vendor specific */
	.bDeviceSubClass = 0xFF,
	.bDeviceProtocol = 0xFF,
	.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
	.idVendor = XINPUT_VID,
	.idProduct = XINPUT_PID,
	.bcdDevice = 0x0114,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/* Configuration descriptor */
#define XINPUT_CONF_SIZE (USB_CFG_DESC_SIZE + \
			  USB_INTERFACE_DESC_SIZE + \
			  17 + /* Unknown descriptor */ \
			  USB_EP_DESC_SIZE + \
			  USB_EP_DESC_SIZE)

USBD_CLASS_DESCR_DEFINE(primary, 0) uint8_t xinput_cfg_desc[] = {
	/* Configuration descriptor */
	USB_CFG_DESC_SIZE,           /* bLength */
	USB_DESC_CONFIGURATION,      /* bDescriptorType */
	XINPUT_CONF_SIZE & 0xFF,     /* wTotalLength[0] */
	(XINPUT_CONF_SIZE >> 8) & 0xFF,  /* wTotalLength[1] */
	0x04,                        /* bNumInterfaces */
	0x01,                        /* bConfigurationValue */
	0x00,                        /* iConfiguration */
	USB_SCD_REMOTE_WAKEUP | USB_SCD_SELF_POWERED,  /* bmAttributes */
	0xFA,                        /* bMaxPower (500mA) */

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,     /* bLength */
	USB_DESC_INTERFACE,          /* bDescriptorType */
	XINPUT_INTERFACE_NUM,        /* bInterfaceNumber */
	0x00,                        /* bAlternateSetting */
	0x02,                        /* bNumEndpoints */
	0xFF,                        /* bInterfaceClass (Vendor) */
	0x5D,                        /* bInterfaceSubClass */
	0x01,                        /* bInterfaceProtocol */
	0x00,                        /* iInterface */

	/* Unknown descriptor (XInput specific) */
	0x11,                        /* bLength */
	0x21,                        /* bDescriptorType */
	0x10, 0x01, 0x01, 0x25, 0x81, 0x14, 0x03, 0x03,
	0x03, 0x04, 0x13, 0x02, 0x08, 0x03, 0x03,

	/* Endpoint descriptor (IN) */
	USB_EP_DESC_SIZE,            /* bLength */
	USB_DESC_ENDPOINT,           /* bDescriptorType */
	XINPUT_IN_EP_ADDR,           /* bEndpointAddress */
	USB_DC_EP_INTERRUPT,         /* bmAttributes */
	0x20, 0x00,                  /* wMaxPacketSize = 32 */
	0x04,                        /* bInterval (4ms) */

	/* Endpoint descriptor (OUT) */
	USB_EP_DESC_SIZE,            /* bLength */
	USB_DESC_ENDPOINT,           /* bDescriptorType */
	XINPUT_OUT_EP_ADDR,          /* bEndpointAddress */
	USB_DC_EP_INTERRUPT,         /* bmAttributes */
	0x20, 0x00,                  /* wMaxPacketSize = 32 */
	0x08,                        /* bInterval (8ms) */
};

/* String descriptors */
USBD_STRING_DESCR_USER_DEFINE(primary) struct usb_string_descs {
	struct usb_string_descriptor lang;
	struct usb_mfr_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[28];
	} __packed manufacturer;
	struct usb_product_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[48];
	} __packed product;
	struct usb_sn_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[26];
	} __packed sn;
} __packed xinput_strings = {
	.lang = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_DESC_STRING,
		.bString = 0x0409,  /* English (US) */
	},
	.manufacturer = {
		.bLength = 30,
		.bDescriptorType = USB_DESC_STRING,
		.bString = {
			'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 's', 0, 'o', 0, 'f', 0,
			't', 0, ' ', 0, 'C', 0, 'o', 0, 'r', 0, 'p', 0,
		},
	},
	.product = {
		.bLength = 50,
		.bDescriptorType = USB_DESC_STRING,
		.bString = {
			'C', 0, 'o', 0, 'n', 0, 't', 0, 'r', 0, 'o', 0, 'l', 0, 'l', 0,
			'e', 0, 'r', 0, ' ', 0, '(', 0, 'X', 0, 'B', 0, 'O', 0, 'X', 0,
			' ', 0, '3', 0, '6', 0, '0', 0, ' ', 0, 'F', 0, 'o', 0, 'r', 0,
		},
	},
	.sn = {
		.bLength = 28,
		.bDescriptorType = USB_DESC_STRING,
		.bString = {
			'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
			'0', 0, '0', 0, '0', 0, '0', 0, '1', 0,
		},
	},
};

/* USB status callback */
static void xinput_status_cb(struct usb_cfg_data *cfg,
			      enum usb_dc_status_code status,
			      const uint8_t *param)
{
	switch (status) {
	case USB_DC_CONFIGURED:
		xinput_configured = true;
		LOG_INF("XInput configured");
		break;
	case USB_DC_DISCONNECTED:
	case USB_DC_SUSPEND:
		xinput_configured = false;
		break;
	default:
		break;
	}
}

/* Vendor request handler */
static int xinput_vendor_handler(struct usb_setup_packet *setup,
				 int32_t *len, uint8_t **data)
{
	LOG_DBG("Vendor request: bRequest=0x%02x", setup->bRequest);
	
	if (setup->bRequest == 0x01) {
		/* XInput capabilities request */
		static uint8_t capabilities[] = {
			0x00, 0x14, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		*data = capabilities;
		*len = sizeof(capabilities);
		return 0;
	}
	
	return -ENOTSUP;
}

/* USB configuration structure */
USBD_CFG_DATA_DEFINE(primary, xinput) struct usb_cfg_data xinput_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &xinput_cfg_desc[9],
	.cb_usb_status = xinput_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = NULL,
		.vendor_handler = xinput_vendor_handler,
	},
	.num_endpoints = 2,
	.endpoint = {
		{
			.ep_addr = XINPUT_IN_EP_ADDR,
			.ep_cb = NULL,
		},
		{
			.ep_addr = XINPUT_OUT_EP_ADDR,
			.ep_cb = NULL,
		},
	},
};

/* Public API to send button state */
int zmk_xinput_send_report(uint16_t buttons)
{
	int ret;

	if (!xinput_configured) {
		return -ENOTCONN;
	}

	k_sem_take(&xinput_sem, K_FOREVER);

	current_report.buttons = buttons;

	ret = usb_write(XINPUT_IN_EP_ADDR, (uint8_t *)&current_report,
			sizeof(current_report), NULL);

	k_sem_give(&xinput_sem);

	if (ret < 0) {
		LOG_ERR("Failed to send XInput report: %d", ret);
		return ret;
	}

	return 0;
}

/* Initialize XInput USB - called at POST_KERNEL level BEFORE usb_enable() */
static int xinput_usb_init(void)
{
	int ret;

	LOG_INF("Initializing XInput USB interface");

	/* Register the USB configuration */
	ret = usb_set_config(&xinput_config);
	if (ret < 0) {
		LOG_ERR("Failed to set XInput USB config: %d", ret);
		return ret;
	}

	/* Do NOT call usb_enable() - ZMK does this at APPLICATION level */
	
	LOG_INF("XInput USB interface registered");
	return 0;
}

/* Initialize at POST_KERNEL level, priority 0 - BEFORE ZMK calls usb_enable() */
SYS_INIT(xinput_usb_init, POST_KERNEL, 0);
