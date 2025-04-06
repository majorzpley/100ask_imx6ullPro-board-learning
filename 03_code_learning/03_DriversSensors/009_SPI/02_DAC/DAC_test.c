#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 *DAC_test /dev/DAC <val>
 */
int main(int argc, char **argv) {
  int fd;
  int buf[2];
  unsigned short dac_val = 0;

  if (argc != 3) {
    printf("Usage: %s <dev> <dac_value>\n", argv[0]);
    return -1;
  }

  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    perror("open");
    return -1;
  }

  dac_val = strtoul(argv[2], NULL, 0);

  //   while (1) {
  write(fd, &dac_val, sizeof(dac_val));
  // dac_val += 50;
  //   }

  close(fd);
  return 0;
}