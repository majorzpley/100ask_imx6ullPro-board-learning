//? 测试步进电机驱动；"./app_MOTOR_test /dev/MOTOR +4096 1"转动360°
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * ./button_test /dev/MOTOR -100/+100
 *
 */
int main(int argc, char **argv) {

  int fd;
  int i;
  int buf[2];
  int ret;

  /* 1. 判断参数 */
  if (argc != 4) {
    printf("Usage: %s <dev> <step_number> <mdelay_number>\n", argv[0]);
    return -1;
  }

  /* 2. 打开文件 */
  fd = open(argv[1], O_RDWR | O_NONBLOCK); /*设置为非阻塞方式*/
  if (fd == -1) {
    perror("open");
    return -1;
  }

  buf[0] = strtol(argv[2], NULL, 0);
  buf[1] = strtol(argv[3], NULL, 0);

  ret = write(fd, buf, 8);

  close(fd);

  return 0;
}
