#ifndef PSVIS_H
#define PSVIS_H

#include <linux/ioctl.h>

/* The name of the device file */
#define DEVICE_FILE_NAME "psvisout"

/* Device major number */
#define MAJOR_NUM 304

/* Set the message of the device driver */
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)

/* Get the message of the device driver */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)

/* Get the n'th byte of the message */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)

#endif

