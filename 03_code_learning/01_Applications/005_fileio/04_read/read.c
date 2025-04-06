//? write函数
//- umask 0002
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./read 1.txt
 * argv[0] = "./read"
 * argv[1] = "1.txt"
 */
int main(int argc, char const *argv[]) {
  int fd;
  int i;
  int len;
  unsigned char buffer[100];
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return -1;
  }
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd = %d\n", fd); //- fd = 3
  }
  // todo 读取文件并打印
  while (1) {
    len = read(fd, buffer, sizeof(buffer));
    if (len < 0) {
      perror("read");
      close(fd);
      return -1;
    } else if (len == 0) { //- 读到文件尾
      break;
    } else {
      /*
       * buf[0],buf[1],...,buf[len-1]含有读出的数据
       *buf[len]='\0'
       */
      buffer[len] = '\0';
      printf("%s", buffer);
    }
  }

  close(fd);
  return 0;
}
