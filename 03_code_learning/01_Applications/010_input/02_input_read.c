//? 获取输入设备信息：以查询方式与休眠唤醒方式
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * 。/02_input_read /dev/input/event1 [noblock]
 */
int main(int argc, char const *argv[]) {
  int fd;
  int err;
  int len;
  int bit;
  struct input_id id;
  unsigned int evbit[2];
  struct input_event event;  //- 参考"drivers\input\evdev.c"中的evdev_read函数
  char *ev_names[] = {"EV_SYN", "EV_KEY", "EV_REL", "EV_ABS", "EV_MSC",
                      "EV_SW",  "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "EV_LED", "EV_SND", "NULL",
                      "EV_REP", "EV_FF",  "EV_PWR"};
  unsigned char byte;
  if (argc < 2) {
    printf("Usage: %s <dev> [noblock]\n", argv[0]);
    return -1;
  }
  if (argc == 3 && !strcmp(argv[2], "noblock")) {
    // todo 查询方式
    fd = open(argv[1], O_RDWR | O_NONBLOCK);
  } else {
    // todo 休眠唤醒方式(阻塞方式)
    fd = open(argv[1], O_RDWR);
  }
  if (fd < 0) {
    perror("open");
    return -1;
  }
  err = ioctl(fd, EVIOCGID, &id);
  if (err == 0) {
    printf("bustype = 0x%x\n", id.bustype);
    printf("vendor = 0x%x\n", id.vendor);
    printf("product = 0x%x\n", id.product);
    printf("version = 0x%x\n", id.version);
  }
  len = ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
  if (len > 0 && len <= sizeof(evbit)) {
    printf("support ev type: ");
    for (size_t i = 0; i < len; i++) {
      byte = ((unsigned char *)evbit)[i];
      for (bit = 0; bit < 8; bit++) {
        if (byte & (1 << bit)) {
          printf("%s ", ev_names[i * 8 + bit]);
        }
      }
    }
    printf("\n");
  }
  while (1) {
    len = read(
        fd, &event,
        sizeof(event));  //- 查询方式此处会立即返回，休眠唤醒方式此处会进入休眠
    if (len == sizeof(event)) {
      printf("get event: type = %#x, code = %#x, value = %#x\n", event.type,
             event.code, event.value);
    } else {
      printf("read err %d\n", len);
    }
  }

  close(fd);
  return 0;
}
