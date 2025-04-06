#ifndef _LED_OPR_H
#define _LED_OPR_H

struct led_operations {
  //- 初始化LED，which☞哪个LED
  int (*init)(int which);
  //- 控制LED，which☞哪个LED，status：1-亮，0-灭
  int (*ctl)(int which, char status);
  //- 获取LED电平,which☞哪个LED,返回值:0-低电平,1-高电平
  char (*getLevel)(int which);
  //- 取消IO映射,which☞哪个LED
  int (*unmap)(int which);
};

struct led_operations *get_board_led_opr(void);

#endif
