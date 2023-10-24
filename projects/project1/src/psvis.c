#include "psvis.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "common.h"


int visualize_pstree(char* spid) {

  char* psvis_data = malloc(PSTREE_DATA_MAX * sizeof(char));

  // open chdev file
  int file_desc;
  if ((file_desc = open(PSVIS_DEVICE_FILENAME, 0)) < 0) {
    return ERROR;
  }

  // send pid to device driver
  if (ioctl(file_desc, IOCTL_SET_MSG, spid) < 0) {
    return ERROR;
  }

  // get data from device driver
  if (ioctl(file_desc, IOCTL_GET_MSG, psvis_data) < 0) {
    return ERROR;
  }

  // print data
  printf("%s\n", psvis_data);

  // exit
  close(file_desc);
  free(psvis_data);
  return SUCCESS;

}


