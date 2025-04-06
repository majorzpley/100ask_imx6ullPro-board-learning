//? mmap app测试
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * ./hello_drv_test
 */
int main(int argc, char const *argv[]) {
  int fd;
  char *buf;
  int len;
  char str[1024];

  // todo 1. 判断参数
  if (argc < 2) {
    printf("Usage: %s <-w <string>>\n", argv[0]);
    printf("       %s <-r>\n", argv[0]);
    return -1;
  }

  // todo 2.打开文件
  fd = open("/dev/hello", O_RDWR);
  if (fd == -1) {
    printf("can not opepn file /dev/hello\n");
    return -1;
  }

  /*mmap*/
  // buf = mmap(NULL, 1024 * 8, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  buf = mmap(NULL, 1024 * 8, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd,
             0); //写时拷贝
  if (buf == MAP_FAILED) {
    printf("can not mmap file /dev/hello!\n");
    close(fd);
    return -1;
  }

  printf("mmap address = %#x\n", buf);
  printf("buf origin data = %s\n", buf); /*old*/
  /*write*/
  strcpy(buf, "new");

  /*read & compare*/
  read(fd, str, 1024); /*str = "old"*/
  if (!strcmp(buf, str)) {
    printf("compare OK!\n");

  } else {
    printf("compare fail!\n");  /*cat /proc/<pid>/maps*/
    printf("str = %s!\n", str); /*old*/
    printf("buf = %s!\n", buf); /*new*/
  }
  while (1) {
    sleep(10);
  }

  /*munmap*/
  munmap(buf, 1024 * 8);
  close(fd);
  return 0;
}
