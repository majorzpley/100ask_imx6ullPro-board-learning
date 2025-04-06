#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ledtest /dev/myled on
 * ledtest /dev/myled off
 */
int main(int argc, char **argv) {
  int fd;
  char status = 0;

  if (argc != 3) {
    printf("Usage: %s <dev> <on|off>\n", argv[0]);
    printf("   eg: %s /dev/myled on\n", argv[0]);
    printf("   eg: %s /dev/myled off\n", argv[0]);
    return -1;
  }
  // open
  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    perror("open");
    return -1;
  }
  // write
  if (!strcmp(argv[2], "on")) {
    status = 1;
  }
  write(fd, &status, 1);
  close(fd);
  return 0;
}