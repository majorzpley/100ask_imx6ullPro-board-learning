//? 针对SR04超声波测距模块编写的外部中断驱动模板
// 一个脉冲触发引脚(Trig 引脚),这里为GPIO4_19,3 * 32 + 19 = 115
// 一个回响接收引脚(Echo 引脚),这里为GPIO4_20,3 * 32 +20 = 116
// bug WARNING: "__aeabi_uldivmod"[SR04_gpio_interrupt_drv.ko] undefined!
// brief 解决办法：需要传递时间到app计算
// tips:在中断isr和read中尽量不要打印信息
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
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
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "SR04: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

#define CMD_TRIG 0x64

/*------------------------全局变量---------------------------*/
struct gpio_describe {
  int gpio;
  int irq;
  char *name;
  int data;
  struct timer_list timer;
};
static struct gpio_describe g_s_pins[2] = {
    {115, 0, "sr04_Trig"},
    {116, 0, "sr04_Echo"},
}; /*根据实际pin脚值修改*/

static int g_s_major = 0;       /*主设备号*/
static struct class *g_s_class; /*设备类*/

static int g_vals[BUF_LEN];
static int r, w;

static DECLARE_WAIT_QUEUE_HEAD(
    g_s_wait_queue); /*APP使用阻塞或非阻塞访问需要的等待队列头*/

static struct fasync_struct *g_s_fasync; /*APP使用异步通知需要的变量*/

/*---------------------环形缓冲区函数-------------------------*/
static int is_circle_buf_empty(void) { return (r == w); }

static int is_circle_buf_full(void) { return (r == NEXT_POS(w)); }

static void add_to_circle_buf(int input) {
  if (!is_circle_buf_full()) {
    g_vals[w] = input;
    w = NEXT_POS(w);
  }
}

static int get_from_circle_buf(void) {
  int ret = 0;
  if (!is_circle_buf_empty()) {
    ret = g_vals[r];
    r = NEXT_POS(r);
  }
  return ret;
}

/*----------------file_operations接口函数--------------------*/
static ssize_t dev_drv_read(struct file *file, char __user *buf, size_t size,
                            loff_t *offset) {
  int err;
  int val;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (is_circle_buf_empty() && (file->f_flags & O_NONBLOCK)) {
    return -EAGAIN;
  }

  /*等待缓冲区circle_buffer不为空*/
  wait_event_interruptible(g_s_wait_queue, !is_circle_buf_empty());

  val = get_from_circle_buf();

  if (val == -1) {
    return -ENODATA;
  }

  err = copy_to_user(buf, &val, sizeof(val));

  return 4;
}

static unsigned int dev_drv_poll(struct file *file,
                                 struct poll_table_struct *table) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  poll_wait(file, &g_s_wait_queue, table);
  return is_circle_buf_empty() ? 0 : (POLLIN | POLLRDNORM);
}

static int dev_drv_fasync(int fd, struct file *file, int mode) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  if (fasync_helper(fd, file, mode, &g_s_fasync) >= 0) {
    return 0;
  } else {
    return -EIO;
  }
}

static long dev_drv_ioctl(struct file *file, unsigned int command,
                          unsigned long arg) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*send trig*/
  switch (command) {
  case CMD_TRIG:
    gpio_set_value(g_s_pins[0].gpio, 1);
    udelay(20); /*至少需要10us*/
    gpio_set_value(g_s_pins[0].gpio, 0);

    /*start timer bug:
     * 这里一直在刷新定时器，所以应用程序的poll超时可能并不会触发*/
    mod_timer(&g_s_pins[1].timer, jiffies + msecs_to_jiffies(50)); /*50ms*/

    break;
  }
  return 0;
}

/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_drv_read,
    .poll = dev_drv_poll,
    .fasync = dev_drv_fasync,
    .unlocked_ioctl = dev_drv_ioctl,
};

/*-------------------中断/超时服务函数------------------------*/
//- 中断顶半部处理函数
static irqreturn_t top_isr(int irq, void *data) {
  struct gpio_describe *gpio_desc = data;
  int logic_level;
  static u64 rising_time = 0;
  u64 time;

  DEBUG_PRINTK("top_isr pin %d irq happened\n", gpio_desc->gpio);

  logic_level = gpio_get_value(gpio_desc->gpio);
  DEBUG_PRINTK("SR04 isr echo pin is %d\n", logic_level);

  if (logic_level) {
    /*上升沿记录起始时间*/
    rising_time = ktime_get_ns();
  } else {
    if (rising_time == 0) {
      DEBUG_PRINTK("missing rising interrupt\n");
      return IRQ_HANDLED;
    }
    /*停止定时器计时*/
    del_timer(&g_s_pins[1].timer);

    /*下降沿记录结束时间并计算时间差和距离*/
    time = ktime_get_ns() - rising_time;
    rising_time = 0;

    add_to_circle_buf(time);

    wake_up_interruptible(&g_s_wait_queue); /*唤醒应用程序*/

    kill_fasync(&g_s_fasync, SIGIO, POLL_IN); /*发送SIGIO信号到APP*/
  }

  return IRQ_HANDLED;
}

// 定时器超时处理函数
static void timeout_expire(unsigned long data) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  add_to_circle_buf(-1);

  wake_up_interruptible(&g_s_wait_queue); /*唤醒应用程序*/

  kill_fasync(&g_s_fasync, SIGIO, POLL_IN); /*发送SIGIO信号到APP*/
}

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*trig pin*/
  err = gpio_request(g_s_pins[0].gpio, g_s_pins[0].name);
  gpio_direction_output(g_s_pins[0].gpio, 0);

  /*echo pin*/
  g_s_pins[1].irq = gpio_to_irq(g_s_pins[1].gpio);
  err = request_irq(g_s_pins[1].irq, top_isr,
                    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                    g_s_pins[1].name, &g_s_pins[1]);

  setup_timer(&g_s_pins[1].timer, timeout_expire, (unsigned long)&g_s_pins[1]);

  g_s_major = register_chrdev(0, "sensors", &fops);

  g_s_class = class_create(THIS_MODULE, "class");
  if (IS_ERR(g_s_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_s_major, "sensors");
    return PTR_ERR(g_s_class);
  }

  device_create(g_s_class, NULL, MKDEV(g_s_major, 0), NULL,
                "SR04"); /*/dev/SR04*/

  return err;
}

/*------------------------出口函数---------------------------*/
static void __exit dev_drv_exit(void) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_class, MKDEV(g_s_major, 0));
  class_destroy(g_s_class);
  unregister_chrdev(g_s_major, "sensors");

  /*trig pin*/
  gpio_free(g_s_pins[0].gpio);

  /*echo pin*/
  free_irq(g_s_pins[1].irq, &g_s_pins[1]);

  /*停止定时器计时*/
  del_timer(&g_s_pins[1].timer);
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");