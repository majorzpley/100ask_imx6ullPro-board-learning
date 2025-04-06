//? 获取输入设备信息：使用select函数
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * 。/02_input_read_select /dev/input/event1
 */
int main(int argc, char const *argv[]) {
  int fd;
  int err;
  int len;
  int i;
  int ret;
  int bit;
  unsigned char byte;
  struct input_id id;
  unsigned int evbit[2];
  struct input_event event;  //- 参考"drivers\input\evdev.c"中的evdev_read函数
  int nfds;
  fd_set readfds;
  struct timeval timeout;
  char *ev_names[] = {"EV_SYN", "EV_KEY", "EV_REL", "EV_ABS", "EV_MSC",
                      "EV_SW",  "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "EV_LED", "EV_SND", "NULL",
                      "EV_REP", "EV_FF",  "EV_PWR"};
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }
  // todo 查询方式
  fd = open(argv[1], O_RDWR | O_NONBLOCK);
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
    // todo 设置超时时间:5S
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    // todo 设置监测文件
    FD_ZERO(&readfds);     //- 清零
    FD_SET(fd, &readfds);  //- 将fd列入监测对象readfds
    nfds = fd +
           1; /* nfds 是最大的文件句柄+1, 注意: 不是文件个数, 这与poll不一样 */
    ret = select(nfds, &readfds, NULL, NULL, &timeout);
    if (ret > 0) {
      if (FD_ISSET(fd, &readfds)) {
        while (read(fd, &event, sizeof(event)) == sizeof(event)) {
          printf("get event: type = %#x, code = %#x, value = %#x\n", event.type,
                 event.code, event.value);
        }
      }
    } else if (ret == 0) {
      printf("time out\n");
    } else {
      printf("select error\n");
    }
  }
  close(fd);
  return 0;
}
