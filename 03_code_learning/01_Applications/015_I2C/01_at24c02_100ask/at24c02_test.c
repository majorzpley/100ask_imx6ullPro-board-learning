#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "i2cbusses.h"
#include "include/i2c/smbus.h"
/*
 * ./at24c02 <i2c_bus_NO> w "100ask.taobao.com"
 * ./at24c02 <i2c_bus_NO> r
 */
int main(int argc, char const *argv[]) {
  unsigned char dev_addr = 0x50;
  unsigned char mem_addr = 0x00;
  unsigned char buf[32];

  int file;
  char filename[20];
  unsigned char *str;
  struct timespec req;
  int ret;

  if (argc != 3 && argc != 4) {
    printf("Usage: \n");
    printf("write eeprom:%s <i2c_bus_number> w string\n", argv[0]);
    printf("read eeprom:%s <i2c_bus_number> r\n", argv[0]);
    return -1;
  }

  file = open_i2c_dev(argv[1][0] - '0', filename, sizeof(filename), 0);
  if (file < 0) {
    printf("can not open %s\n", filename);
    return -1;
  }

  if (set_slave_addr(file, dev_addr, 1)) {
    printf("can not set slave_addr\n");
    return -1;
  }

  if (argv[2][0] == 'w') {
    //- write
    str = (char *)argv[3];

    req.tv_sec = 0;
    req.tv_nsec = 20000000; //- 20ms = 20000000ns

    while (*str) {
      // todo mem_addr,*struct MyStruct
      ret = i2c_smbus_write_byte_data(file, mem_addr, *str);
      if (ret != 0) {
        printf("i2c_smbus_write_byte_data err!\n");
        return -1;
      }

      // todo wait tWR
      nanosleep(&req, NULL);
      // todo mem_addr++.str++
      mem_addr++;
      str++;
    }
    ret = i2c_smbus_write_byte_data(file, mem_addr, '\0'); // string EOF
    if (ret != 0) {
      printf("i2c_smbus_write_byte_data err!\n");
      return -1;
    }
  } else {
    //- read
    ret = i2c_smbus_read_i2c_block_data(file, mem_addr, sizeof(buf), buf);
    if (ret < 0) {
      printf("i2c_smbus_read_i2c_block_data err!\n");
      return -1;
    }

    buf[31] = '\0'; //- 防止写入字符太多而导致一直打印
    printf("get data: %s\n", buf);
  }
  return 0;
}
