/**
 * Library to control M&S USB Missile Launcher
 *
 * Copyright (C) Ian Jeffray 2005, all rights reserved.
 * Copyright (C) Joseph Heenan 2005, all rights reserved.
 *
 */

#ifndef MISSILE_CONTROL_LIB_H
#define MISSILE_CONTROL_LIB_H

typedef enum
{
    missile_stop  = 0,
    missile_left  = 1,
    missile_right = 2,
    missile_up    = 4,
    missile_down  = 8,
    missile_fire  = 16
}
missile_command;

/** opaque handle for device */
typedef struct missile_usb missile_usb;

/**
 * Initialise library
 *
 * Must be called before calling any other functions
 *
 * @return 0 on success, -1 on failure
 */
int missile_usb_initialise(void);

/**
 * Finalise library
 *
 * Do not call any library functions after calling this.
 */
void missile_usb_finalise(void);

/**
 * Create a device handle
 *
 * @param debug   set to 1 to enable debug, 0 otherwise
 * @param timeout usb comms timeout in milliseconds (1000 would be reasonable)
 *
 * @return pointer to opaque device handle on success, NULL on failure
 */
missile_usb *missile_usb_create(int debug, int timeout);

/**
 * Destroy a device handle
 *
 * @param control device handle to destroy
 */
void missile_usb_destroy(missile_usb *control);

/**
 * Attempt to find the usb missile launcher device
 *
 * Scans through all the devices on all the usb busses and finds the
 * 'device_num'th device.
 *
 * @param control    library handle
 * @param device_num 0 for first device, 1 for second, etc
 *
 * @return 0 on success, -1 on failure
 */
int missile_usb_finddevice(missile_usb *control, int device_num);

/**
 * Send a usb command
 *
 * @param control    library handle
 * @param ...        (first) 8 bytes of packet
 *
 * @return 0 on success, -1 on failure
 */
int missile_usb_sendcommand(missile_usb *control, int, int, int, int, int, int, int, int );
int missile_usb_sendcommand64(missile_usb *control, int, int, int, int, int, int, int, int );

void missile_usb_sendcommand_foo(missile_usb *control, int cmd);

#endif /* MISSILE_CONTROL_LIB_H */
