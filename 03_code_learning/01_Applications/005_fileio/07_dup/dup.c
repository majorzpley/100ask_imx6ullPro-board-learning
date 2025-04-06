//? dup函数的使用
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  char buf1[10];
  char buf2[10];
  char buf3[10];
  if (argc != 2) {
    printf("Usage: %s <file>\n", argv[0]);
  }
  int fd1 = open(argv[1], O_RDONLY);
  int fd2 = open(argv[1], O_RDONLY);
  int fd3 = dup(fd1);

  printf("fd1= %d\n", fd1);
  printf("fd2= %d\n", fd2);
  printf("fd3= %d\n", fd3);

  if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
    perror("open");
    return -1;
  }

  read(fd1, buf1, 1);
  read(fd2, buf2, 1);
  read(fd3, buf3, 1);

  printf("data get from fd1:%c\n", buf1[0]);  // 1
  printf("data get from fd2:%c\n", buf2[0]);  // 1
  printf("data get from fd3:%c\n", buf3[0]);  // 2
  close(fd1);
  close(fd2);
  close(fd3);
  return 0;
}
