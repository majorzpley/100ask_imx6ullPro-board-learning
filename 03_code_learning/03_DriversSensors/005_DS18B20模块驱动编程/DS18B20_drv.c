// todo 针对DS18B20温度模块编写的外部中断驱动模板
// 硬件连接在gpio4_io19,PIN = 3 * 32 + 19 = 115
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "asm-generic/int-ll64.h"
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
#include "linux/jiffies.h"
#include "linux/kdev_t.h"
#include "linux/kern_levels.h"
#include "linux/kernel.h"
#include "linux/module.h"
#include "linux/netdevice.h"
#include "linux/poll.h"
#include "linux/printk.h"
#include "linux/sched.h"
#include "linux/spinlock.h"
#include "linux/spinlock_types.h"
#include "linux/stddef.h"
#include "linux/timekeeping.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*-------------------------自定义宏--------------------------*/
#define BUF_LEN 128
#define NEXT_POS(x) ((x + 1) % BUF_LEN)

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DS18B20: " fmt, ##args)
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
    {115, 0, "ds18b20_dio"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;           /*主设备号*/
static struct class *g_s_class;     /*设备类*/
static spinlock_t ds18b20_spinlock; /*锁，用于禁止中断使用*/

/*----------------环形缓冲区/自定义函数---------------------*/
static void ds18b20_udelay(int us) {
  u64 time = ktime_get_ns();
  while (ktime_get_ns() - time < us * 1000)
    ;
}

static int ds18b20_reset_and_wait_ack(void) {
  int timeout = 100;

  gpio_set_value(g_s_pins[0].gpio, 0);
  ds18b20_udelay(480);

  gpio_direction_input(g_s_pins[0].gpio);

  /*等待ACK*/
  while (gpio_get_value(g_s_pins[0].gpio) && timeout--) {
    ds18b20_udelay(1);
  }
  if (timeout == 0) {
    /*超时*/
    return -EIO;
  }

  /*等待ACK结束*/
  timeout = 300;
  while (!gpio_get_value(g_s_pins[0].gpio) && timeout--) {
    ds18b20_udelay(1);
  }
  if (timeout == 0) {
    return -EIO;
  }

  return 0;
}

static void ds18b20_send_cmd(u8 command) {
  int i;

  gpio_direction_output(g_s_pins[0].gpio, 1);

  /*注意是先发送LSB，低位*/
  for (i = 0; i < 8; i++) {
    if (command & (1 << i)) {
      /*发送1*/
      gpio_direction_output(g_s_pins[0].gpio, 0);
      ds18b20_udelay(2);

      gpio_direction_output(g_s_pins[0].gpio, 1);
      ds18b20_udelay(60);

    } else {
      /*发送0*/
      gpio_direction_output(g_s_pins[0].gpio, 0);
      ds18b20_udelay(60);
      gpio_direction_output(g_s_pins[0].gpio, 1);
    }
  }
}

static void ds18b20_read_data(u8 *buf) {
  int i;
  u8 data = 0;

  /*注意是先读取LSB，低位*/
  gpio_direction_output(g_s_pins[0].gpio, 1);
  for (i = 0; i < 8; i++) {
    gpio_direction_output(g_s_pins[0].gpio, 0);
    ds18b20_udelay(2);
    gpio_direction_input(g_s_pins[0].gpio);
    ds18b20_udelay(15);
    if (gpio_get_value(g_s_pins[0].gpio)) {
      data |= (1 << i);
    }
    ds18b20_udelay(50);
    gpio_direction_output(g_s_pins[0].gpio, 1);
  }
  buf[0] = data;
}

/*参考网站:https://www.cnblogs.com/yuanguanghui/p/12737740.html*/
static u8 calcrc_1byte(u8 abyte) {
  u8 i, crc_1byte;
  crc_1byte = 0;
  for (i = 0; i < 8; i++) {
    if ((crc_1byte ^ abyte) & 0x01) {
      crc_1byte ^= 0x18;
      crc_1byte >>= 1;
      crc_1byte |= 0x80;
    } else {
      crc_1byte >>= 1;
    }
    abyte >>= 1;
  }
  return crc_1byte;
}

static u8 calcrc_bytes(u8 *p, u8 len) {
  u8 crc = 0;
  while (len--) {
    crc = calcrc_1byte(crc ^ *p++);
  }
  return crc;
}

static int ds18b20_verify_crc(u8 *buf) {
  u8 crc;
  crc = calcrc_bytes(buf, 8);

  if (crc == buf[8]) {
    return 0;
  } else {
    return -1;
  }
}

static void ds18b20_calc_val(u8 *ds18b20_buf, int *result) {
  u8 tempL = 0, tempH = 0;
  u32 integer;
  u8 decimal1 = 0, decimal2 = 0, decimal = 0;

  tempL = ds18b20_buf[0];
  tempH = ds18b20_buf[1];

  if (tempH > 0x7f) {
    /*负温*/
    tempL = ~tempL;
    tempH = ~tempH + 1;                        /*补码转换:取反加一*/
    integer = tempL / 16 + tempH * 16;         /*整数部分*/
    decimal1 = (tempL & 0x0f) * 10 / 16;       /*小数第一位*/
    decimal1 = (tempL & 0x0f) * 100 / 16 % 10; /*小数第二位*/
    decimal = decimal1 * 10 + decimal2;
  } else {
    integer = tempL / 16 + tempH * 16;         /*整数部分*/
    decimal1 = (tempL & 0x0f) * 10 / 16;       /*小数第一位*/
    decimal1 = (tempL & 0x0f) * 100 / 16 % 10; /*小数第二位*/
    decimal = decimal1 * 10 + decimal2;
  }
  result[0] = integer;
  result[1] = decimal;
}
/*------------------------服务函数---------------------------*/

/*----------------file_operations接口函数--------------------*/
static ssize_t dev_drv_read(struct file *file, char __user *buf, size_t size,
                            loff_t *offset) {
  unsigned long flags;
  int err;
  u8 kernel_buf[9];
  int i;
  int result_buf[2];

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (size != 8) {
    return -EINVAL;
  }

  /*1.启动温度转换*/
  /*1.1 关中断*/
  spin_lock_irqsave(&ds18b20_spinlock, flags);

  /*1.2 发出reset信号并等待回应*/
  err = ds18b20_reset_and_wait_ack();
  if (err) {
    spin_unlock_irqrestore(&ds18b20_spinlock, flags);
    DEBUG_PRINTK("%s err 1\n", __FUNCTION__);
    return err;
  }

  /*1.3 发出命令:skip rom,0xcc*/
  ds18b20_send_cmd(0xcc);

  /*1.4 发出命令:启动温度转换,0x44*/
  ds18b20_send_cmd(0x44);

  /*1.5 恢复中断*/
  spin_unlock_irqrestore(&ds18b20_spinlock, flags);

  /*2.等待温度转换成功：可能长达1S*/
  // schedule_timeout 是基于内核的调度机制实现的延时，适合在任务上下文中使用。
  set_current_state(TASK_INTERRUPTIBLE);
  schedule_timeout(msecs_to_jiffies(1000));

  /*3.读取温度*/
  /*3.1 关中断*/
  spin_lock_irqsave(&ds18b20_spinlock, flags);

  /*3.2 发出reset信号并等待回应*/
  err = ds18b20_reset_and_wait_ack();
  if (err) {
    spin_unlock_irqrestore(&ds18b20_spinlock, flags);
    DEBUG_PRINTK("%s err 2\n", __FUNCTION__);
    return err;
  }

  /*3.3 发出命令:skip rom,0xcc*/
  ds18b20_send_cmd(0xcc);

  /*3.4 发出命令:read scratchpad,0xbe*/
  ds18b20_send_cmd(0xbe);

  /*3.5 读取9字节数据*/
  for (i = 0; i < 9; i++) {
    ds18b20_read_data(&kernel_buf[i]);
  }

  /*3.6 恢复中断*/
  spin_unlock_irqrestore(&ds18b20_spinlock, flags);

  /*3.7 计算crc，验证数据*/
  err = ds18b20_verify_crc(kernel_buf);
  if (err) {
    DEBUG_PRINTK("%s crc err\n", __FUNCTION__);
    return err;
  }

  /*4.copy_to_user*/
  ds18b20_calc_val(kernel_buf, result_buf);
  err = copy_to_user(buf, result_buf, sizeof(result_buf));

  return sizeof(result_buf);
}

/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_drv_read,
};

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;
  int i;
  int count;

  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*初始化锁*/
  spin_lock_init(&ds18b20_spinlock);

  for (i = 0; i < count; i++) {
    /*申请DHT11_GPIO引脚资源*/
    err = gpio_request(g_s_pins[i].gpio, g_s_pins[i].name);

    /*设置初始状态为高电平*/
    gpio_direction_output(g_s_pins[i].gpio, 1);
  }

  g_s_major = register_chrdev(0, "sensors", &fops);

  g_s_class = class_create(THIS_MODULE, "class");
  if (IS_ERR(g_s_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_s_major, "sensors");
    return PTR_ERR(g_s_class);
  }

  device_create(g_s_class, NULL, MKDEV(g_s_major, 0), NULL,
                "DS18B20"); /*/dev/DS18B20*/

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