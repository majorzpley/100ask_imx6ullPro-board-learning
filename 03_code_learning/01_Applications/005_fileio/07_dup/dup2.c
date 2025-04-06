//? dup函数的使用：此程序其实已经默认打开3个文件stdin,stdout,stderr
//- 将stdout重定向到文件fd中
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./dup2 1.txt
 * argv[0] = "./dup2"
 * argv[1] = "1.txt"
 */
int main(int argc, char const *argv[]) {
  int fd;
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return -1;
  }
  fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd = %d\n", fd);  //- fd = 3
  }
  dup2(fd, 1);  //- 关闭stdout(1)并重定向到fd = 1的地址指向fd=3的文件
  printf("Hello, World, fd = %d Demo!\n", fd);
  close(fd);
  return 0;
}
