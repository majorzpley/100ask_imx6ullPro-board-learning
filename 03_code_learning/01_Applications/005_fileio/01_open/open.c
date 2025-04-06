//? 查看open函数:man 2 open
//- 测试：./open ./open.c &
//- ls -al /proc/pid/fs可查看到文件句柄
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./open 1.txt
 * argv[0] = "./open"
 * argv[1] = "1.txt"
 */
int main(int argc, char const *argv[]) {
  int fd;
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return -1;
  }
  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd = %d\n", fd); //- fd = 3
  }
  while (1) {
    sleep(10);
  }
  close(fd);
  return 0;
}
