//? 使用非阻塞阻塞方式测试SR501驱动
// 使用cat /sys/kernel/debug/gpio查看Trig pin的相关资源信息
// log 使用poll判断circlebuf是否为空改进read函数一直处于阻塞状态
// brief 1.在app程序中使用poll来查询数据是否正常
// brief 2.使用定时器在drv驱动中唤醒app程序
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
    printf("I am goning to read distance: \n");

    fds[0].fd = fd;
    fds[0].events = POLLIN;

    ret = poll(fds, 1, timeout_ms);
    if (ret == 1) {

      if (read(fd, &val, 4) == 4)
        printf("get distance: %d cm\n", val * 17 / 1000000);
      else
        printf("while get distance err: -1\n");
    } else {
      printf("timeout err: -1\n");
    }
    sleep(1);
  }

  close(fd);

  return 0;
}
