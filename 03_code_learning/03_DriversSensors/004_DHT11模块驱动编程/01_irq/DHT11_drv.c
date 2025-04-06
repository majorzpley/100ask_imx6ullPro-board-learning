// todo 针对DHT11温湿度模块编写的外部中断驱动模板
// 硬件连接在gpio4_io19,PIN = 3 * 32 + 19 = 115
// bug 由"cat /proc/interrupts"可知，中断次数并不一定是84次，实际上会低于84
// bug 经测试，由于linux的非实时性，中断读取并不可靠，经常会丢失数据
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
#include "linux/stddef.h"
#include "linux/timekeeping.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*-------------------------自定义宏--------------------------*/
#define BUF_LEN 128
#define NEXT_POS(x) ((x + 1) % BUF_LEN)

// #define DEBUG

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
    {115, 0, "dht11_dio"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;          /*主设备号*/
static struct class *g_s_class;    /*设备类*/
static u64 g_s_dht11_irq_time[84]; /*记录dht11发生中断的时刻*/
static int g_s_dht11_irq_count;    /*记录dht11发生中断的次数*/

static char g_vals[BUF_LEN];
static int r, w;

static DECLARE_WAIT_QUEUE_HEAD(
    g_s_wait_queue); /*APP使用阻塞或非阻塞访问需要的等待队列头*/

/*----------------环形缓冲区/自定义函数---------------------*/
static int is_circle_buf_empty(void) { return (r == w); }

static int is_circle_buf_full(void) { return (r == NEXT_POS(w)); }

static void add_to_circle_buf(char input) {
  if (!is_circle_buf_full()) {
    g_vals[w] = input;
    w = NEXT_POS(w);
  }
}

static char get_from_circle_buf(void) {
  char ret = 0;
  if (!is_circle_buf_empty()) {
    ret = g_vals[r];
    r = NEXT_POS(r);
  }
  return ret;
}

static void parse_dht11_datas(void) {
  int i;
  u64 high_time;
  u8 data = 0;
  int bits = 0;
  u8 datas[5];
  int byte = 0;
  u8 crc = 0;

  /*数据的个数：可能是81、82、83、84*/
  if (g_s_dht11_irq_count < 81) {
    /*出错*/
    add_to_circle_buf(-1);
    add_to_circle_buf(-1);
    /*唤醒APP*/
    wake_up_interruptible(&g_s_wait_queue);
    g_s_dht11_irq_count = 0;
    return;
  }

  /*解析数据*/
  for (i = g_s_dht11_irq_count - 80; i < g_s_dht11_irq_count; i += 2) {
    high_time = g_s_dht11_irq_time[i] - g_s_dht11_irq_time[i - 1];
    // low_time = g_s_dht11_irq_time[i - 1] - g_s_dht11_irq_time[i - 2];
    data <<= 1;

    /*low_time ≈ 50us = 50,000ns*/
    if (high_time > 50000) /*data 1*/ {
      data |= 1;
    }
    bits++;
    if (bits == 8) {
      datas[byte] = data;
      data = 0;
      bits = 0;
      byte++;
    }
  }
  /*放入环形buffer*/
  crc = datas[0] + datas[1] + datas[2] + datas[3];
  if (crc == datas[4]) {
    add_to_circle_buf(datas[0]); /*温度整数*/
    add_to_circle_buf(datas[2]); /*湿度整数*/
  } else {
    add_to_circle_buf(-1);
    add_to_circle_buf(-1);
  }

  g_s_dht11_irq_count = 0;
  /*唤醒APP*/
  wake_up_interruptible(&g_s_wait_queue);
}

/*------------------------服务函数---------------------------*/
// 定时器中断处理函数
static void timer_expire(unsigned long data) {

  /*解析数据，放入环形buffer，唤醒APP*/
  parse_dht11_datas();
}

// 中断顶半部处理函数
// 测试内核精确的时间：ktime_get_ns
static irqreturn_t top_isr(int irq, void *data) {
  struct gpio_describe *gpio_desc = data;
  u64 time;

  DEBUG_PRINTK("top_isr pin %d irq happened\n", gpio_desc->gpio);

  /*1.记录当前中断发生的时间*/
  time = ktime_get_ns();
  g_s_dht11_irq_time[g_s_dht11_irq_count] = time;

  /*2.累加次数*/
  g_s_dht11_irq_count++;

  /*3.次数足够：解析数据，放入环形buffer，唤醒APP*/
  if (g_s_dht11_irq_count == 84) {
    del_timer(&g_s_pins[0].gpio_timer);
    parse_dht11_datas();
  }

  return IRQ_HANDLED;
}

/*----------------file_operations接口函数--------------------*/
static ssize_t dev_drv_read(struct file *file, char __user *buf, size_t size,
                            loff_t *offset) {
  int err;
  char kernel_buf[2];

  if (size != 2) {
    return -EINVAL;
  }
  g_s_dht11_irq_count = 0;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*1.发送18ms的低脉冲*/
  err = gpio_request(g_s_pins[0].gpio, g_s_pins[0].name);
  gpio_direction_output(g_s_pins[0].gpio, 0);
  gpio_free(g_s_pins[0].gpio);
  mdelay(18);

  /*2.注册中断*/
  gpio_direction_input(
      g_s_pins[0].gpio); /*引脚切换为输入方向，由上拉电阻拉为高电平*/
  err = request_irq(g_s_pins[0].irq, top_isr,
                    IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                    g_s_pins[0].name, &g_s_pins[0]);
  mod_timer(&g_s_pins[0].gpio_timer, jiffies + 10); /*防止read函数阻塞*/

  /*3.休眠等待数据,直到环形缓冲区非空时退出休眠*/
  wait_event_interruptible(g_s_wait_queue, !is_circle_buf_empty());

  free_irq(g_s_pins[0].irq, &g_s_pins[0]);
  /*申请DHT11_GPIO引脚资源*/
  err = gpio_request(g_s_pins[0].gpio, g_s_pins[0].name);
  /*设置初始状态为高电平*/
  gpio_direction_output(g_s_pins[0].gpio, 1);
  /*为了后续使用中断资源，故需要释放掉*/
  gpio_free(g_s_pins[0].gpio);

  /*4.copy_to_user*/
  kernel_buf[0] = get_from_circle_buf();
  kernel_buf[1] = get_from_circle_buf();

  if (kernel_buf[0] == (char)-1 && kernel_buf[1] == (char)-1) {
    return -EIO;
  }
  err = copy_to_user(buf, kernel_buf, 2);

  return 2;
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
    g_s_pins[i].irq = gpio_to_irq(g_s_pins[i].gpio);

    /*申请DHT11_GPIO引脚资源*/
    err = gpio_request(g_s_pins[i].gpio, g_s_pins[i].name);

    /*设置初始状态为高电平*/
    gpio_direction_output(g_s_pins[i].gpio, 1);

    /*为了后续使用中断资源，故需要释放掉*/
    gpio_free(g_s_pins[i].gpio);

    /*设置定时器*/
    setup_timer(&g_s_pins[i].gpio_timer, timer_expire,
                (unsigned long)&g_s_pins[i]);
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
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");