#include <linux/kernel.h>
#include <linux/module.h>

/* Deal with CONFIG_MODVERSIONS */
#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/fs.h>
#include <linux/wrapper.h>
#include "psvismod.h"

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
#include <asm/uaccess.h>
#endif

#define SUCCESS 0


/* Device Declarations ******************************** */

/* The name for our device, as it will appear in /proc/devices */
#define DEVICE_NAME "psvisout"

#define PSVIS_MSG_MAX 8196

static int Device_Open = 0;

// PS Tree
static char Message[PSVIS_MSG_MAX];
static char *Message_Ptr;

// PID
static char PID[32];
static char *PID_Ptr;

// Boot Time
time64_t boot_time;

void ps_stat(struct task_struct* ts) {
  time64_t start_time = ts->start_time / 1000000000ULL + boot_time;
  sprintf(Message, "PID: %d, PPID: %d, CMD: %s, START: %lld\n", ts->pid, ts->parent->pid, ts->comm, start_time);
}

void psvis_traverse(struct task_struct* ts) {
  struct task_struct* child;
  struct list_head* list;
  list_for_each(list, &ts->children) {
    child = list_entry(list, struct task_struct, sibling);
    ps_stat(child);
    psvis_traverse(child);
  }
}

// Start from root
void psvis_init(void) {
  struct task_struct* root_ts;
  root_ts = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  boot_time = ktime_get_seconds();
  psvis_traverse(root_ts);
}


/* This function is called whenever a process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk ("device_open(%p)\n", file);
#endif

  // If device is open, return busy
  if (Device_Open)
    return -EBUSY;

  // Increment usage count, and remember that we're open
  Device_Open++;
  Message_Ptr = Message;
  MOD_INC_USE_COUNT;

  return SUCCESS;
}

/* This function is called when a process closes the device file */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int device_release(struct inode *inode, struct file *file)
#else
static void device_release(struct inode *inode, struct file *file)
#endif
{
#ifdef DEBUG
  printk ("device_release(%p,%p)\n", inode, file);
#endif
 
  /* We're now ready for our next caller */
  Device_Open --;
  MOD_DEC_USE_COUNT;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  return 0;
#endif
}


/* This function is called whenever a process which 
 * has already opened the device file attempts to 
 * read from it. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
#else
static int device_read(struct inode *inode, struct file *file, char *buffer, int length)
#endif
{
  /* Number of bytes actually written to the buffer */
  int bytes_read = 0;

#ifdef DEBUG
  printk("device_read(%p,%p,%d)\n", file, buffer, length);
#endif

  /* If we're at the end of the message, return 0 */
  if (*Message_Ptr == 0)
    return 0;

  /* Actually put the data into the buffer */
  while (length && *Message_Ptr)  {
    put_user(*(Message_Ptr++), buffer++);
    length --;
    bytes_read++;
  }

#ifdef DEBUG
   printk ("Read %d bytes, %d left\n", bytes_read, length);
#endif

   /* Read functions are supposed to return the number 
    * of bytes actually inserted into the buffer */
  return bytes_read;
}


/* This function is called when somebody tries to 
 * write into our device file. */ 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
#else
static int device_write(struct inode *inode, struct file *file, const char *buffer, int length)
#endif
{
  int i

#ifdef DEBUG
  printk ("device_write(%p,%s,%d)", file, buffer, length);
#endif

  for(i=0; i<length && i<BUF_LEN; i++)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
    get_user(PID[i], buffer+i);

#else
    PID[i] = get_user(buffer+i);
#endif  

  PID_Ptr = PID;



  return i;
}


/* This function is called whenever a process tries to do an ioctl on our device file. */
int device_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
  int i;
  char *temp;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  char ch;
#endif

  switch (ioctl_num) {
    case IOCTL_SET_MSG:
      temp = (char *) ioctl_param;
   
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
      get_user(ch, temp);
      for (i=0; ch && i<BUF_LEN; i++, temp++)
        get_user(ch, temp);
#else
      for (i=0; get_user(temp) && i<BUF_LEN; i++, temp++);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
      device_write(file, (char *) ioctl_param, i, 0);
#else
      device_write(inode, file, (char *) ioctl_param, i);
#endif
      break;

    case IOCTL_GET_MSG:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
      i = device_read(file, (char *) ioctl_param, 99, 0); 
#else
      i = device_read(inode, file, (char *) ioctl_param, 99); 
#endif
      put_user('\0', (char *) ioctl_param+i);
      break;
    case IOCTL_GET_NTH_BYTE:
      return Message[ioctl_param];
      break;
  }

  return SUCCESS;
}


/* Module Declarations *************************** */
struct file_operations Fops = {
  NULL,   /* seek */
  device_read, 
  device_write,
  NULL,   /* readdir */
  NULL,   /* select */
  device_ioctl,   /* ioctl */
  NULL,   /* mmap */
  device_open,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  NULL,  /* flush */
#endif
  device_release  /* a.k.a. close */
};

/* Initialize the module - Register the character device */
int init_module()
{
  int ret_val;
  ret_val = module_register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
  if (ret_val < 0) {
    printk ("Sorry, registering the character device failed with %d\n", ret_val);
    return ret_val;
  }
  printk ("Successfully registered. Major Device Number: %d.\n", MAJOR_NUM);

  return SUCCESS;
}

/* Cleanup - unregister the appropriate file from /proc */
void cleanup_module()
{
  int ret;
  ret = module_unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
  if (ret < 0)
    printk("Error in module_unregister_chrdev: %d\n", ret);
}