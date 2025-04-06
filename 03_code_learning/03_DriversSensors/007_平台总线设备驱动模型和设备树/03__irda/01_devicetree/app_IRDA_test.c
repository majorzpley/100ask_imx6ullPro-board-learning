#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

/*
 * ./button_test /dev/IRDA
 *
 */
int main(int argc, char **argv) {

  unsigned char buf[2];

  /* 1. 判断参数 */
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }

  /* 2. 打开文件 */
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    printf("can not open file %s\n", argv[1]);
    return -1;
  }

  while (1) {
    if (read(fd, buf, 2) == 2)
      printf("get irda: device:%#x data:%#x\n", buf[0], buf[1]);
    else
      printf("get irda: -1\n");
  }

  close(fd);

  return 0;
}
