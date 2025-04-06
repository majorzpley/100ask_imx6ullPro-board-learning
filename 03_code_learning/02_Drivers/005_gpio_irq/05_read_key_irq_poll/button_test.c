#include <fcntl.h>
#include <poll.h>
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
  struct pollfd fds[1];
  int timeout_ms = 5000;
  int ret;

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

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  while (1) {
    /*3.读文件*/
    ret = poll(fds, 1, timeout_ms);
    if (ret == 1 && (fds[0].revents & POLLIN)) {
      read(fd, &val, sizeof(val));
      printf("get button : %#x\n", val);
    } else {
      printf("timeout!\n");
    }
  }

  close(fd);

  return 0;
}