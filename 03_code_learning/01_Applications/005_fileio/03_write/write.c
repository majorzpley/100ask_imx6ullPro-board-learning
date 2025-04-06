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
  fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd = %d\n", fd); //- fd = 3
  }
  for (i = 2; i < argc; i++) {
    len = write(fd, argv[i], strlen(argv[i]));
    if (len != strlen(argv[i])) {
      perror("write");
      return -1;
    }
    write(fd, "\r\n", 2);
  }

  close(fd);
  return 0;
}
