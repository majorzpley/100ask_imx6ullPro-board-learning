#ifndef _LED_DRV_H
#define _LED_DRV_H

#include "led_opr.h"

extern void led_class_device_create(int minor);
extern void led_class_device_destroy(int minor);
extern void register_led_operations(struct led_operations *opr);

#endif /* _LED_DRV_H */