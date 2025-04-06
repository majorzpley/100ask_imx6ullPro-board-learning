//? 针对28BYJ-48步进电机模块编写的驱动模板
// GPIO0:GPIO4_19,编号:3 * 32 +19 = 115
// GPIO1:GPIO4_20,编号:3 * 32 +20 = 116
// GPIO2:GPIO4_21,编号:3 * 32 +21 = 117
// GPIO3:GPIO4_22,编号:3 * 32 +22 = 118
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "asm-generic/siginfo.h"
#include "asm/gpio.h"
#include "asm/signal.h"
#include "asm/uaccess.h"
#include "linux/delay.h"
#include "linux/device.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/gpio/consumer.h"
#include "linux/init.h"
#include "linux/interrupt.h"
#include "linux/irqreturn.h"
#include "linux/kdev_t.h"
#include "linux/kern_levels.h"
#include "linux/kernel.h"
#include "linux/module.h"
#include "linux/poll.h"
#include "linux/printk.h"
#include "linux/stddef.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*-------------------------自定义宏--------------------------*/
#define BUF_LEN 128
#define NEXT_POS(x) ((x + 1) % BUF_LEN)

#define DEBUG 1

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "SR501: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

/*------------------------全局变量---------------------------*/
struct gpio_describe {
  int gpio;
  int irq;
  char *name;
  int data;
  struct timer_list gpio_timer;
};
static struct gpio_describe g_s_pins[] = {
    {115, 0, "motor_gpio0"},
    {116, 0, "motor_gpio1"},
    {117, 0, "motor_gpio2"},
    {118, 0, "motor_gpio3"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;       /*主设备号*/
static struct class *g_s_class; /*设备类*/

static int g_s_motor_pin_ctrl[8] = {0x02, 0x03, 0x01, 0x09, 0x08,
                                    0x0c, 0x04, 0x06}; /*马达引脚设置数组*/
static int g_s_motor_index = 0;

/*-------------------环形缓冲区/自定义函数--------------------*/
static void set_pins_for_motor(int index) {
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(g_s_pins[i].gpio,
                   g_s_motor_pin_ctrl[index] & (1 << i) ? 1 : 0);
  }
}

/*将所有引脚置于高阻态*/
static void disable_motor(void) {
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(g_s_pins[i].gpio, 0);
  }
}
/*----------------file_operations接口函数--------------------*/
/*
 * int buf[2]
 * buf[0] = 步进的次数，> 0：逆时针步进，< 0：顺时针步进
 * buf[1] = mdelay的时间(调速)
 */
static ssize_t dev_drv_write(struct file *fp, const char __user *buf,
                             size_t size, loff_t *offset) {
  int kernel_buf[2];
  int err;
  int step;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (size != 8) {
    return -EINVAL;
  }

  err = copy_from_user(kernel_buf, buf, size);

  if (kernel_buf[0] > 0) {
    /*逆时针旋转*/
    for (step = 0; step < kernel_buf[0]; step++) {
      set_pins_for_motor(g_s_motor_index);
      mdelay(kernel_buf[1]);
      g_s_motor_index--;
      if (g_s_motor_index == -1) {
        g_s_motor_index = 7;
      }
    }
  } else {
    /*顺时针旋转*/
    kernel_buf[0] = 0 - kernel_buf[0];
    for (step = 0; step < kernel_buf[0]; step++) {
      set_pins_for_motor(g_s_motor_index);
      mdelay(kernel_buf[1]);
      g_s_motor_index++;
      if (g_s_motor_index == 8) {
        g_s_motor_index = 0;
      }
    }
  }
  /*改进：选抓到位后，让马达不再消耗电源*/
  disable_motor();

  return size;
}

/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = dev_drv_write,
};

/*------------------------服务函数---------------------------*/
//- 中断顶半部处理函数

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;
  int i;
  int count;

  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*申请GPIO资源*/
  for (i = 0; i < count; i++) {
    err = gpio_request(g_s_pins[i].gpio, g_s_pins[i].name);
    /*输出低电平，电机ABCD开路高阻态，防止电机过热*/
    gpio_direction_output(g_s_pins[i].gpio, 0);
  }

  g_s_major = register_chrdev(0, "sensors", &fops);

  g_s_class = class_create(THIS_MODULE, "class");
  if (IS_ERR(g_s_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_s_major, "sensors");
    return PTR_ERR(g_s_class);
  }

  device_create(g_s_class, NULL, MKDEV(g_s_major, 0), NULL,
                "MOTOR"); /*/dev/MOTOR*/

  return err;
}

/*------------------------出口函数---------------------------*/
static void __exit dev_drv_exit(void) {
  int i;
  int count;
  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_class, MKDEV(g_s_major, 0));
  class_destroy(g_s_class);
  unregister_chrdev(g_s_major, "sensors");

  for (i = 0; i < count; i++) {
    gpio_free(g_s_pins[i].gpio);
  }
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");