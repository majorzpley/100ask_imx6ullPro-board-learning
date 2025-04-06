//? 使用非阻塞阻塞方式测试SR501驱动
// 使用cat /sys/kernel/debug/gpio查看Trig pin的相关资源信息
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CMD_TRIG 0x64

static int fd;

/*
 * ./button_test /dev/SR04
 *
 */
int main(int argc, char **argv) {
  int val;
  struct pollfd fds[1];
  int timeout_ms = 5000;
  int ret;
  int flags;

  int i;

  /* 1. 判断参数 */
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }

  /* 2. 打开文件 */
  fd = open(argv[1], O_RDWR); /*设置为阻塞方式*/
  if (fd == -1) {
    perror("open");
    return -1;
  }

  while (1) {
    /*发出trig电平*/
    ioctl(fd, CMD_TRIG);

    if (read(fd, &val, 4) == 4)
      printf("get distance: %d cm\n", val * 17 / 1000000);
    else
      printf("while get distance err: -1\n");
    sleep(1);
  }

  close(fd);

  return 0;
}
