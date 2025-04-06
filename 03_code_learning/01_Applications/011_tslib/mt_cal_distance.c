//? 计算多点触控手指距离:参考ts_lib源码中的tests目录下的ts_test_mt.c来编写
//- arm-buildroot-linux-gnueabihf-gcc -o mt_cal_distance mt_cal_distance.c -lts
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/input.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tslib.h>
#include <unistd.h>

int distance(struct ts_sample_mt* point1, struct ts_sample_mt* point2) {
  int x = point1->x - point2->x;
  int y = point1->y - point2->y;
  return x * x + y * y;
}

int main(int argc, char const* argv[]) {
  int i;
  int ret;
  int max_slots = 1;
  int point_pressed[20];
  int touch_cnt = 0;
  struct tsdev* ts;
  struct input_absinfo slot;
  struct ts_sample_mt** samp_mt;
  struct ts_sample_mt** pre_samp_mt;

  ts = ts_setup(NULL, 0);
  if (ts == NULL) {
    printf("ts setup err!\n");
    return -1;
  }

  if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
    perror("ioctl EVIOGABS");
    ts_close(ts);
    return errno;
  }
  max_slots = slot.maximum + 1 - slot.minimum;  //- 此设备同时支持的触点数目

  samp_mt = malloc(sizeof(struct ts_sample_mt*));
  if (!samp_mt) {
    ts_close(ts);
    return -ENOMEM;
  }
  samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
  if (!samp_mt[0]) {
    free(samp_mt);
    ts_close(ts);
    return -ENOMEM;
  }
  pre_samp_mt = malloc(sizeof(struct ts_sample_mt*));
  if (!pre_samp_mt) {
    ts_close(ts);
    return -ENOMEM;
  }
  pre_samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
  if (!pre_samp_mt[0]) {
    free(pre_samp_mt);
    ts_close(ts);
    return -ENOMEM;
  }

  for (i = 0; i < max_slots; i++) {
    pre_samp_mt[0][i].valid = 0;
  }
  while (1) {
    ret = ts_read_mt(ts, samp_mt, max_slots, 1);

    if (ret < 0) {
      printf("ts_read_mt err!\n");
      ts_close(ts);
      return -1;
    }
    for (i = 0; i < max_slots; i++) {
      if (samp_mt[0][i].valid) {  //- 新触点更新
        memcpy(&pre_samp_mt[0][i], &samp_mt[0][i], sizeof(struct ts_sample_mt));
      }
    }
    touch_cnt = 0;
    for (i = 0; i < max_slots; i++) {
      //-老触点更新并且没有松开
      if (pre_samp_mt[0][i].valid && pre_samp_mt[0][i].tracking_id != -1) {
        point_pressed[touch_cnt++] = i;  //- 记录同时存在的触点数目
      }
    }
    if (touch_cnt == 2) {
      printf("distance:%08d\n", distance(&pre_samp_mt[0][point_pressed[0]],
                                         &pre_samp_mt[0][point_pressed[1]]));
    }
  }
  ts_close(ts);
  return 0;
}
