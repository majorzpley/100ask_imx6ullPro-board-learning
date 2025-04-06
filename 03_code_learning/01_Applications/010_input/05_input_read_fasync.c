//? 获取输入设备信息：使用异步通知方式(signal函数)
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int fd;
void my_sig_handler(int sig) {
  struct input_event event;  //- 参考"drivers\input\evdev.c"中的evdev_read函数
  while (read(fd, &event, sizeof(event)) == sizeof(event)) {
    printf("get event: type = %#x, code = %#x, value = %#x\n", event.type,
           event.code, event.value);
  }
}

/*
 * 。/02_input_read_fasync /dev/input/event1
 */
int main(int argc, char const *argv[]) {
  int err;
  int len;
  int ret;
  int bit;
  int flags;
  int count = 0;
  struct input_id id;
  unsigned int evbit[2];
  unsigned char byte;
  char *ev_names[] = {"EV_SYN", "EV_KEY", "EV_REL", "EV_ABS", "EV_MSC",
                      "EV_SW",  "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "NULL",   "NULL",   "NULL",
                      "NULL",   "NULL",   "EV_LED", "EV_SND", "NULL",
                      "EV_REP", "EV_FF",  "EV_PWR"};
  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }
  // todo 注册信号处理函数
  signal(SIGIO, my_sig_handler);
  // todo 打开驱动程序
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
  // todo 把APP的进程告诉驱动程序
  fcntl(fd, __F_SETOWN, getpid());
  // todo 使能"异步通知"
  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | FASYNC);
  while (1) {
    printf("main loop count = %d\n", count++);
    sleep(2);
  }

  close(fd);
  return 0;
}
