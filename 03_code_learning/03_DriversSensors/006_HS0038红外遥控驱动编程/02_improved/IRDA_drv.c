// todo 针对IRDA HS0038红外遥控模块编写的外部中断驱动
// 硬件连接在gpio4_io19,PIN = 3 * 32 + 19 = 115
// bug:引导码接收的68个中断调试成功，但是连发码4个中断未解码成功
// 已解决上述bug
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/int-ll64.h"
#include "asm-generic/param.h"
#include "asm-generic/poll.h"
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
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "IRDA: " fmt, ##args)
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
    {115, 0, "irda"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;       /*主设备号*/
static struct class *g_s_class; /*设备类*/
static struct fasync_struct *g_s_fasync;
DECLARE_WAIT_QUEUE_HEAD(g_s_wait);

static u8 g_s_vals[BUF_LEN];
static int r, w;

static u64 g_s_irda_irq_times[68]; /*记录一次完整传输发生中断的时间点*/
static int g_s_irda_irq_count = 0;
/*----------------环形缓冲区/自定义函数---------------------*/
static int is_circle_buf_empty(void) { return (r == w); }

static int is_circle_buf_full(void) { return (r == NEXT_POS(w)); }

static void add_to_circle_buf(u8 input) {
  if (!is_circle_buf_full()) {
    g_s_vals[w] = input;
    w = NEXT_POS(w);
  }
}

static u8 get_from_circle_buf(void) {
  u8 ret = 0;
  if (!is_circle_buf_empty()) {
    ret = g_s_vals[r];
    r = NEXT_POS(r);
  }
  return ret;
}
/*irda解析重复码*/
static int get_irda_repeat_datas(void) {
  u64 time;
  /*1.判断前导码:9ms的低脉冲，2.25ms的高脉冲*/
  time = g_s_irda_irq_times[1] - g_s_irda_irq_times[0];
  if (time < 8000000 || time > 10000000) {
    return -1;
  }

  time = g_s_irda_irq_times[2] - g_s_irda_irq_times[1];
  if (time < 2000000 || time > 2500000) {
    return -1;
  }
  return 0;
}
/*irda解析数据*/
static void parse_irda_datas(void) {
  u64 time;
  int i, m, n;
  unsigned char datas[4];
  unsigned char data = 0;
  int bits = 0;
  int byte = 0;
  /*1.判断前导码:9ms的低脉冲，4.5ms的高脉冲*/
  time = g_s_irda_irq_times[1] - g_s_irda_irq_times[0];
  if (time < 8000000 || time > 10000000) {
    goto err;
  }
  time = g_s_irda_irq_times[2] - g_s_irda_irq_times[1];
  if (time < 3500000 || time > 5500000) {
    goto err;
  }

  /*2.解析数据*/
  for (i = 0; i < 32; i++) {
    m = 3 + i * 2;
    n = m + 1;
    time = g_s_irda_irq_times[n] - g_s_irda_irq_times[m];
    data <<= 1;
    bits++;
    if (time > 1000000) {
      /*bit 1*/
      data |= 1;
    }
    if (bits == 8) {
      datas[byte] = data;
      byte++;
      data = 0;
      bits = 0;
    }
  }

  /*判断数据是否有效*/
  datas[1] = ~datas[1];
  datas[3] = ~datas[3];

  if ((datas[0] != datas[1]) || (datas[2] != datas[3])) {
    DEBUG_PRINTK("data verify err: %#x %#x %#x %#x!\n", datas[0], datas[1],
                 datas[2], datas[3]);
    goto err;
  }

  add_to_circle_buf(datas[0]);
  add_to_circle_buf(datas[2]);
  wake_up_interruptible(&g_s_wait);
  kill_fasync(&g_s_fasync, SIGIO, POLL_IN);
  return;
err:
  g_s_irda_irq_count = 0;
  add_to_circle_buf(-1);
  add_to_circle_buf(-1);
  wake_up_interruptible(&g_s_wait);
  kill_fasync(&g_s_fasync, SIGIO, POLL_IN);
}

/*---------------------中断处理函数-------------------------*/
static void timer_expire(unsigned long data) {
  /*表示超时出错*/
  g_s_irda_irq_count = 0;
  add_to_circle_buf(-1);
  add_to_circle_buf(-1);
  wake_up_interruptible(&g_s_wait);
  kill_fasync(&g_s_fasync, SIGIO, POLL_IN);
}

static irqreturn_t top_isr(int fd, void *data) {
  struct gpio_describe *gpio_desc = data;
  u64 time;

  DEBUG_PRINTK("%s pin %d irq happened\n", __FUNCTION__, gpio_desc->gpio);

  /*1.记录中断发生的时刻*/
  time = ktime_get_ns();
  g_s_irda_irq_times[g_s_irda_irq_count] = time;

  /*2.累加中断次数*/
  g_s_irda_irq_count++;

  /*3.次数达标后(68次中断)，删除定时器，解析数据，放入buffer，唤醒APP*/
  if (g_s_irda_irq_count == 4) {
    /*是否是重复码*/
    if (0 == get_irda_repeat_datas()) {
      add_to_circle_buf(0);
      add_to_circle_buf(0);
      wake_up_interruptible(&g_s_wait);
      kill_fasync(&g_s_fasync, SIGIO, POLL_IN);
      del_timer(&gpio_desc->gpio_timer);
      g_s_irda_irq_count = 0;
      return IRQ_HANDLED;
    }
  }
  if (g_s_irda_irq_count == 68) {
    parse_irda_datas();
    del_timer(&gpio_desc->gpio_timer);
    g_s_irda_irq_count = 0;
    return IRQ_HANDLED;
  }

  /*4.启动定时器*/
  mod_timer(&gpio_desc->gpio_timer, jiffies + msecs_to_jiffies(100)); /*100ms*/
  return IRQ_HANDLED;
}

/*----------------file_operations接口函数--------------------*/
static ssize_t dev_drv_read(struct file *file, char __user *buf, size_t size,
                            loff_t *offset) {
  int err;
  u8 kernel_buf[2];

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (size != 2) {
    return -EINVAL;
  }

  if (is_circle_buf_empty() && (file->f_flags & O_NONBLOCK)) {
    return -EAGAIN;
  }
  wait_event_interruptible(g_s_wait, !is_circle_buf_empty());
  kernel_buf[0] = get_from_circle_buf(); /*device*/
  kernel_buf[1] = get_from_circle_buf(); /*data*/

  if ((kernel_buf[0] == (u8)-1) && (kernel_buf[1] == (u8)-1)) {
    return -EIO;
  }

  err = copy_to_user(buf, kernel_buf, 2);
  return 2;
}

static unsigned int dev_drv_poll(struct file *fp,
                                 struct poll_table_struct *table) {
  poll_wait(fp, &g_s_wait, table);
  return is_circle_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}

static int dev_drv_fasync(int fd, struct file *fp, int mode) {
  if (fasync_helper(fd, fp, mode, &g_s_fasync) >= 0)
    return 0;
  else
    return -EIO;
}
/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_drv_read,
    .poll = dev_drv_poll,
    .fasync = dev_drv_fasync,
};

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;
  int i;
  int count;

  count = sizeof(g_s_pins) / sizeof(g_s_pins[0]);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  for (i = 0; i < count; i++) {
    /*换算中断号*/
    g_s_pins[i].irq = gpio_to_irq(g_s_pins[i].gpio);

    setup_timer(&g_s_pins[i].gpio_timer, timer_expire,
                (unsigned long)&g_s_pins[i]);
    /*申请中断*/
    err = request_irq(g_s_pins[i].irq, top_isr,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      g_s_pins[i].name, &g_s_pins[i]);
  }

  g_s_major = register_chrdev(0, "sensors", &fops);

  g_s_class = class_create(THIS_MODULE, "class");
  if (IS_ERR(g_s_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_s_major, "sensors");
    return PTR_ERR(g_s_class);
  }

  device_create(g_s_class, NULL, MKDEV(g_s_major, 0), NULL,
                "IRDA"); /*/dev/IRDA*/

  return err;
}

/*------------------------出口函数---------------------------*/
static void __exit dev_drv_exit(void) {
  int i;
  int count;
  count = sizeof(g_s_pins) / sizeof(g_s_pins[0]);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_class, MKDEV(g_s_major, 0));
  class_destroy(g_s_class);
  unregister_chrdev(g_s_major, "sensors");

  for (i = 0; i < count; i++) {
    free_irq(g_s_pins[i].irq, &g_s_pins[i]);
    // del_timer(&g_s_pins[i].gpio_timer);
  }
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");