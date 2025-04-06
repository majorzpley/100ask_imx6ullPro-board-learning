// todo 针对DHT11温湿度模块编写查询方式驱动模板
// 硬件连接在gpio4_io19,PIN = 3 * 32 + 19 = 115
// bug 由"cat /proc/interrupts"可知，中断次数并不一定是84次，实际上会低于84
// bug经测试，由于linux的非实时性，中断读取并不可靠，经常会丢失数据,故修改为查询模式
// 比中断模式稳定得多
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "asm-generic/int-ll64.h"
#include "asm-generic/siginfo.h"
#include "asm/delay.h"
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
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DHT11: " fmt, ##args)
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
  spinlock_t lock;
};
static struct gpio_describe g_s_pins[] = {
    {115, 0, "dht11_dio"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;       /*主设备号*/
static struct class *g_s_class; /*设备类*/

/* 参考:https://blog.csdn.net/qq_36413982/article/details/122674822 */
#define DHT11_PIN g_s_pins[0].gpio
#define DHT11_IO_OUT() gpio_direction_output(DHT11_PIN, 1);
#define DHT11_IO_IN() gpio_direction_input(DHT11_PIN)
#define DHT11_WRITE(bit) gpio_set_value(DHT11_PIN, bit)
#define DHT11_READ() gpio_get_value(DHT11_PIN)

/*----------------环形缓冲区/自定义函数---------------------*/
/*dht11信号函数*/
static int dht11_wait_for_ready(void) {
  int timeout;
  timeout = 400;
  /*等待低电平到来*/
  while (DHT11_READ() && timeout) {
    udelay(1);
    --timeout;
  }
  if (!timeout) {
    DEBUG_PRINTK("%s line %d timeout!\n", __FUNCTION__, __LINE__);
    return -1;
  }

  /*等待高电平到来*/
  timeout = 1000;
  while (!DHT11_READ() && timeout) {
    udelay(1);
    --timeout;
  }
  if (!timeout) {
    DEBUG_PRINTK("%s line %d timeout!\n", __FUNCTION__, __LINE__);
    return -1;
  }

  /*等待高电平结束*/
  while (DHT11_READ() && timeout) {
    udelay(1);
    --timeout;
  }
  if (!timeout) {
    DEBUG_PRINTK("%s line %d timeout!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  return 0;
}

static int dht11_start(void) {
  DHT11_IO_OUT();
  DHT11_WRITE(0);
  mdelay(18);
  DHT11_WRITE(1);
  udelay(30);
  DHT11_IO_IN(); /*设置为输入*/
  udelay(2);

  if (dht11_wait_for_ready())
    return -1;
  return 0;
}

static int dht11_read_byte(u8 *byte) {
  u8 i, bit = 0, data = 0;
  int timeout = 0;

  for (i = 0; i < 8; i++) {
    timeout = 1000;
    /*等待低电平*/
    while (DHT11_READ() && timeout) {
      udelay(1);
      --timeout;
    }
    if (!timeout) {
      DEBUG_PRINTK("%s line %d timeout!\n", __FUNCTION__, __LINE__);
      return -1;
    }

    /* 等待变为高电平 */
    timeout = 1000;
    while (!DHT11_READ() && timeout) {
      udelay(1);
      --timeout;
    }
    if (!timeout) {
      DEBUG_PRINTK("%s line %d timeout!\n", __FUNCTION__, __LINE__);
      return -1; /* 超时 */
    }
    udelay(40);

    bit = DHT11_READ();

    data <<= 1;
    if (bit) {
      data |= 0x01;
    }
  }
  *byte = data;
  return 0;
}
/*------------------------服务函数---------------------------*/

/*----------------file_operations接口函数--------------------*/
static ssize_t dev_drv_read(struct file *file, char __user *buf, size_t size,
                            loff_t *offset) {
  int ret, i;
  u8 data[5] = {0};
  unsigned long flags;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (size != 5) {
    return -EINVAL;
  }

  /*申请DHT11_GPIO引脚资源*/
  ret = gpio_request(g_s_pins[0].gpio, g_s_pins[0].name);

  /*关闭中断，防止时序被中断破坏*/
  spin_lock_irqsave(&g_s_pins[0].lock, flags);

  /*1.启动信号*/
  if (dht11_start()) {
    DEBUG_PRINTK("dht11 start failed!\n");
    ret = -EFAULT;
    goto failed1;
  }

  /*2.读取5字节的数据*/
  for (i = 0; i < 5; i++) {
    if (dht11_read_byte(&data[i])) {
      DEBUG_PRINTK("dht11 read data failed!\n");
      ret = -EAGAIN;
      goto failed1;
    }
  }

  /*3.恢复中断*/
  spin_unlock_irqrestore(&g_s_pins[0].lock, flags);

  /*4.校验数据*/
  if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
    DEBUG_PRINTK("dht11 check data crc failed!\n");
    ret = -EAGAIN;
    goto failed1;
  }

  /*5.将数据拷贝至用户空间*/
  if (copy_to_user(buf, data, 5)) {
    ret = -EFAULT;
  } else {
    ret = 5;
  }

  /*6.释放GPIO资源*/
  gpio_free(g_s_pins[0].gpio);
  return ret;
failed1:
  gpio_free(g_s_pins[0].gpio);
  spin_unlock_irqrestore(&g_s_pins[0].lock, flags);
  return ret;
}

static int dev_drv_close(struct inode *node, struct file *fp) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  return 0;
}

/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_drv_read,
    .release = dev_drv_close,
};

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;
  int i;
  int count;

  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  for (i = 0; i < count; i++) {
    /*申请DHT11_GPIO引脚资源*/
    err = gpio_request(g_s_pins[i].gpio, g_s_pins[i].name);
  }

  g_s_major = register_chrdev(0, "sensors", &fops);

  g_s_class = class_create(THIS_MODULE, "class");
  if (IS_ERR(g_s_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_s_major, "sensors");
    return PTR_ERR(g_s_class);
  }

  device_create(g_s_class, NULL, MKDEV(g_s_major, 0), NULL,
                "DHT11"); /*/dev/DHT11*/

  spin_lock_init(&g_s_pins[0].lock); /* 初始化自旋锁 */
  return err;
}

/*------------------------出口函数---------------------------*/
static void __exit dev_drv_exit(void) {
  int count;
  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_class, MKDEV(g_s_major, 0));
  class_destroy(g_s_class);
  unregister_chrdev(g_s_major, "sensors");
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");