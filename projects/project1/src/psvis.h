#ifndef PSVIS_H
#define PSVIS_H

/* The name of the device file */
#define PSVIS_DEVICE_FILENAME "psvisout"

/* File for module */
#define PSVIS_MODULE_FILENAME "psvismod.ko"

/* Device major number */
#define MAJOR_NUM 304

/* Set the message of the device driver */
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)

/* Get the message of the device driver */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)

/* Get the n'th byte of the message */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)


/* Interface Functions */

#define PSTREE_DATA_MAX 8192

int visualize_pstree(char* spid);

#endif

