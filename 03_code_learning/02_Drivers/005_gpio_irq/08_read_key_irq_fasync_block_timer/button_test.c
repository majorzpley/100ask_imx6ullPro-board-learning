//? 使用阻塞与非阻塞方式读取
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

/*
 * ./button_test /dev/100ask_gpio_key
 */
int main(int argc, char **argv) {
  int val;
  int flags;
  int i;

  /*1.判断参数*/
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }

  /*2.打开文件*/
  fd = open(argv[1], O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  for (i = 0; i < 10; i++) {
    if (read(fd, &val, 4) == 4) {
      printf("get button : %#x\n", val);
    } else {
      printf("get button : -1\n", val); //会立刻打印10次
    }
  }

  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK); //设置未阻塞方式

  while (1) {
    /*3.读文件*/
    if (read(fd, &val, 4) == 4) {
      printf("get button : %#x\n", val);
    } else {
      printf("while get button : -1\n", val);
    }
  }

  close(fd);

  return 0;
}