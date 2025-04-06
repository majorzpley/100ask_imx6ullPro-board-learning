#ifndef __BUTTON_DRV_H
#define __BUTTON_DRV_H

struct button_operations {
  int count;
  void (*init)(int which);
  int (*read)(int which);
};

///@brief 建立dev下的button设备
extern void register_button_operations(struct button_operations *opr);

///@brief 销毁dev下的button设备
extern void unregister_button_operations(void);

#endif // !__BUTTON_DRV_H