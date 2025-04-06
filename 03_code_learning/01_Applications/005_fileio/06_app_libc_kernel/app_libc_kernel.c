//? 每个进程有自己的"文件句柄空间"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <file>\n", argv[0]);
  }
  int fd = open(argv[1], O_RDONLY);
  printf("fd = %d\n", fd);
  while (1) {
    sleep(100);
  }
  return 0;
}
