//? write函数
//? 在中间写入字符串
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./write str1 str2
 * argv[0] = "./write"
 * argv[1] = str1
 * argv[2] = str2
 */
int main(int argc, char const *argv[]) {
  int fd;
  int i;
  int len;
  if (argc < 3) {
    printf("Usage: %s <filename> <str1> <str2> ...\n", argv[0]);
    return -1;
  }
  fd = open(argv[1], O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd = %d\n", fd); //- fd = 3
  }
  printf("lseek to offset 3 from file head\n");
  lseek(fd, 3, SEEK_SET);
  write(fd, "123", 3);
  close(fd);
  return 0;
}
