#ifndef LED_RESOURCE_H
#define LED_RESOURCE_H

/*
 * 假设GPIO3_0
 * bit[31:16] = group：GPIO3
 * bit[15:0] = which pin：0
 */
#define GROUP(x) (x >> 16)
#define PIN(x) (x & 0xFFFF)
#define GROUP_PIN(group, pin) ((group << 16) | pin)

#endif // !LED_RESOURCE_H