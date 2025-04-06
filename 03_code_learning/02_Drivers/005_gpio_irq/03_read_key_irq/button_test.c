#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * ./button_test /dev/100ask_gpio_key
 */
int main(int argc, char **argv) {
  int fd;
  int val;

  /*1.判断参数*/
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }

  /*2.打开文件*/
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  while (1) {
    /*3.读文件*/
    read(fd, &val, sizeof(val));
    printf("get button : %#x\n", val);
  }

  close(fd);

  return 0;
}