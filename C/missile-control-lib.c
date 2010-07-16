/**
 * Library to control M&S USB Missile Launcher
 *
 * Copyright (C) Ian Jeffray 2005, all rights reserved.
 * Copyright (C) Joseph Heenan 2005, all rights reserved.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <errno.h>

#include "missile-control-lib.h"

/* usb vendor and product id for missile launcher */
#define VENDOR_ID  0x1130
#define PRODUCT_ID 0x0202

struct missile_usb
{
    struct usb_device *device;
    usb_dev_handle    *handle;
    int                debug;
    int                timeout;
    int                interface_claimed;
};

int missile_usb_initialise(void)
{
    usb_set_debug(0);
    usb_init();
    usb_find_busses();
    usb_find_devices();

    return 0;
}

void missile_usb_finalise(void)
{
}

missile_usb *missile_usb_create(int debug, int timeout)
{
    missile_usb *control = malloc(sizeof(*control));
    if (control == NULL)
        return NULL;

    control->device            = NULL;
    control->handle            = NULL;
    control->debug             = debug;
    control->timeout           = timeout;
    control->interface_claimed = 0;

    return control;
}

void missile_usb_destroy(missile_usb *control)
{
    if (control == NULL)
        return;

    if (control->handle != NULL)
        usb_close(control->handle);

    free(control);
}

/**
 * Attempt to find the missile launcher usb device
 *
 * Scans through all the devices on all the usb busses and finds the
 * 'device_num'th device.
 *
 * @param device_num 0 for first device, 1 for second, etc
 */
int missile_usb_finddevice(missile_usb *control, int device_num)
{
    int              device_count = 0;
    struct usb_bus  *bus;

    assert(control != NULL);
    assert(control->interface_claimed == 0);
    assert(device_num >= 0 && device_num < 255);

    for (bus = usb_get_busses(); bus != NULL; bus = bus->next)
    {
        struct usb_device *dev;

        for (dev = bus->devices; dev != NULL; dev = dev->next)
        {
            if (control->debug)
            {
                printf("Found device: %04x-%04x\n",
                       dev->descriptor.idVendor,
                       dev->descriptor.idProduct);
            }
            if (dev->descriptor.idVendor  == VENDOR_ID &&
                dev->descriptor.idProduct == PRODUCT_ID)
            {
                if (control->debug)
                    printf("Found control control num %d\n", device_count);
                if (device_count == device_num)
                {
                    control->device = dev;
                    control->handle = usb_open(control->device);
                    if (control->handle == NULL)
                    {
                        perror("usb_open failed");
                        return -1;
                    }

                    /* successfully opened device */
                    return 0;
                }
                device_count++;
            }
        }
    }

    fprintf(stderr, "missile_usb_finddevice(%d) failed to find device %04x-%04x\n",
            device_count, VENDOR_ID, PRODUCT_ID);

    return -1;
}

static int claim_interface(missile_usb *control)
{
    int  ret;

    if (control->interface_claimed == 0)
    {
        if (control->debug)
            printf("Trying to detach kernel driver\n");

        /* try to detach device in case usbhid has got hold of it */
        usb_detach_kernel_driver_np(control->handle, 0);
        usb_detach_kernel_driver_np(control->handle, 1);

        ret = usb_set_configuration(control->handle, 1);
        if (ret < 0)
        {
            perror("usb_set_configuration failed");
            return -1;
        }

        ret = usb_claim_interface(control->handle, 0);
        if (ret < 0)
        {
            perror("usb_claim_interface failed");
            return -1;
        }

        ret = usb_claim_interface(control->handle, 1);
        if (ret < 0)
        {
            perror("usb_claim_interface failed");
            return -1;
        }
        ret = usb_set_altinterface(control->handle, 0);
        if (ret < 0) {
        	perror("Unable to set alternate interface");
        	return -1;
        }
        control->interface_claimed = 1;
    }
    return 0;
}

int missile_usb_sendcommand(missile_usb *control, int a, int b, int c, int d, int e, int f, int g, int h)
{
    unsigned char buf[8];
    int  ret;

    assert(control != NULL);

    ret = claim_interface(control);
    if (ret != 0)
    {
        return -1;
    }

    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    buf[6] = g;
    buf[7] = h;

    if (control->debug)
    {
        printf("sending bytes %d, %d, %d, %d, %d, %d, %d, %d\n",
               a, b, c, d, e, f, g, h );
    }

    ret = usb_control_msg( control->handle, 0x21, 9, 0x2, 0x01, (char*) buf, 8,  control->timeout);
    if (ret != 8)
    {
        perror("usb_control_msg failed");
        return -1;
    }

    return 0;
}

/*
 * Command to control original M&S USB missile launcher.
 */
void missile_usb_sendcommand_foo(missile_usb *control, int cmd)
{
   char data[64];
   int ret;


	/* Detach kernel driver (usbhid) from device interface and claim */
	usb_detach_kernel_driver_np(control->handle, 0);
	usb_detach_kernel_driver_np(control->handle, 1);

	ret = usb_set_configuration(control->handle, 1);
	if (ret < 0) {
		perror("Unable to set device configuration");
		exit(EXIT_FAILURE);
	}

	ret = usb_claim_interface(control->handle, 1);
	if (ret < 0) {
		perror("Unable to claim interface");
		exit(EXIT_FAILURE);
	}

	ret = usb_set_altinterface(control->handle, 0);
	if (ret < 0) {
		perror("Unable to set alternate interface");
		exit(EXIT_FAILURE);
	}

	data[0] = 'U';
	data[1] = 'S';
	data[2] = 'B';
	data[3] = 'C';
	data[4] = 0;
	data[5] = 0;
	data[6] = 4;
	data[7] = 0;
	ret = usb_control_msg(control->handle,
			USB_DT_HID,			// request type
			USB_REQ_SET_CONFIGURATION,	// request
			USB_RECIP_ENDPOINT,		// value
			1,		// index
			data,		// data
			8,		// Length of data.
			500);		// Timeout
	if (ret != 8) {
		fprintf(stderr, "Error: %s\n", usb_strerror());
		exit(EXIT_FAILURE);
	}

	data[0] = 'U';
	data[1] = 'S';
	data[2] = 'B';
	data[3] = 'C';
	data[4] = 0;
	data[5] = 0x40;
	data[6] = 2;
	data[7] = 0;
	ret = usb_control_msg(control->handle,
			USB_DT_HID,
			USB_REQ_SET_CONFIGURATION,
			USB_RECIP_ENDPOINT,
			1,
			data,
			8,		// Length of data.
			500);		// Timeout
	if (ret != 8) {
		fprintf(stderr, "Error: %s\n", usb_strerror());
		exit(EXIT_FAILURE);
	}

	usb_set_altinterface(control->handle, 0);

	memset(data, 0, 64);
	data[1] = cmd & missile_left ? 1 : 0;
	data[2] = cmd & missile_right ? 1 : 0;
    data[3] = cmd & missile_up ? 1 : 0;
    data[4] = cmd & missile_down ? 1 : 0;
	data[5] = cmd & missile_fire ? 1 : 0;
	data[6] = 8;
	data[7] = 8;

	ret = usb_control_msg(control->handle,
			USB_DT_HID,
			USB_REQ_SET_CONFIGURATION,
			USB_RECIP_ENDPOINT,
			1,
			data,
			64,		// Length of data.
			5000);		// Timeout
	if (ret != 64) {
		fprintf(stderr, "Error: %s\n", usb_strerror());
		exit(EXIT_FAILURE);
	}

	usb_release_interface(control->handle, 1);
}

int missile_usb_sendcommand64(missile_usb *control, int a, int b, int c, int d, int e, int f, int g, int h)
{
    unsigned char buf[64];
    int  ret;

    assert(control != NULL);

    ret = claim_interface(control);
    if (ret != 0)
    {
        return -1;
    }

    memset(buf, 0, 64 );
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    buf[3] = d;
    buf[4] = e;
    buf[5] = f;
    buf[6] = g;
    buf[7] = h;

    if (control->debug)
    {
        printf("sending bytes %d, %d, %d, %d, %d, %d, %d, %d\n",
               a, b, c, d, e, f, g, h );
    }

    ret = usb_control_msg( control->handle, 0x21, 9, 0x2, 0x0, (char*) buf, 64,  control->timeout);
    if (ret != 64)
    {
        perror("usb_control_msg failed");
        return -1;
    }

    return 0;
}

/*
interface = config->interface;
altsetting = interface->altsetting;
endpoint = altsetting->endpoint;
interface_nr = altsetting->bInterfaceNumber;
*/

