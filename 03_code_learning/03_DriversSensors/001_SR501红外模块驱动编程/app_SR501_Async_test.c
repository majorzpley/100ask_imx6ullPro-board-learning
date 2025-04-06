//? 使用异步通知方式测试SR501驱动
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

static void sig_func(int sig) {
  int val;
  /*read*/
  read(fd, &val, sizeof(val));
  /*printf*/
  printf("get button : %#x\n", val);
}

/*
 * ./button_test /dev/sr501
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

  signal(SIGIO, sig_func);

  /* 2. 打开文件 */
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    printf("can not open file %s\n", argv[1]);
    return -1;
  }

  fcntl(fd, F_SETOWN, getpid()); /*传递进程ID*/
  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | FASYNC);

  while (1) {
    /*3.读文件*/
    printf("do somthing else!!\n");
    sleep(2);
  }

  close(fd);

  return 0;
}
