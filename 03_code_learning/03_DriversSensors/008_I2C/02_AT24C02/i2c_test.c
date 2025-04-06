// todo 测试AT24C02读写
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
static int fd;
/*
./i2c_test /dev/AT24C02 string
./i2c_test /dev/AT24C02
*/
int main(int argc, char **argv) {
  int val;
  int ret;
  char buf[100];

  if (argc < 2) {
    printf("Usage: \n");
    printf("     %s <dev>,read at24c02\n", argv[0]);
    printf("     %s <dev> <string>,write at24c02\n", argv[0]);
    return -1;
  }

  /*打开文件*/
  fd = open(argv[1], O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  if (argc == 3) {
    ret = write(fd, argv[2], strlen(argv[2]) + 1);
  } else {
    ret = read(fd, buf, 100);
    printf("read: %s\n", buf);
  }

  /*关闭文件*/
  close(fd);
  return 0;
}
