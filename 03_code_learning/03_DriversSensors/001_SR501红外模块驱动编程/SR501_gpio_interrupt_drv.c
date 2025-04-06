//? 针对SR501红外感应模块编写的外部中断驱动模板
// 硬件连接在gpio4_io19,PIN = 3 * 32 + 19 = 115
/*-------------------------头文件----------------------------*/
#include "asm-generic/errno-base.h"
#include "asm-generic/siginfo.h"
#include "asm/gpio.h"
#include "asm/signal.h"
#include "asm/uaccess.h"
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
static struct gpio_describe g_s_pins[2] = {
    {115, 0, "sr501"},
    {},
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

  /*等待缓冲区circle_buffer不为空*/
  wait_event_interruptible(g_s_wait_queue, !is_circle_buf_empty());

  val = get_from_circle_buf();
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

/*------------------file_operations定义---------------------*/
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_drv_read,
    .poll = dev_drv_poll,
    .fasync = dev_drv_fasync,
};

/*------------------------服务函数---------------------------*/
//- 中断顶半部处理函数
static irqreturn_t top_isr(int irq, void *data) {
  struct gpio_describe *gpio_desc = data;
  int logic_level;
  int input_param;

  DEBUG_PRINTK("top_isr pin %d irq happened\n", gpio_desc->gpio);

  logic_level = gpio_get_value(gpio_desc->gpio);
  input_param = (gpio_desc->data << 8) | (logic_level);
  add_to_circle_buf(input_param);

  wake_up_interruptible(&g_s_wait_queue);   /*唤醒中断内核线程*/
  kill_fasync(&g_s_fasync, SIGIO, POLL_IN); /*发送SIGIO信号到APP*/

  return IRQ_HANDLED;
}

/*------------------------入口函数---------------------------*/
static int __init dev_drv_init(void) {
  int err;
  int i;
  int count;

  count = ARRAY_SIZE(g_s_pins);

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  for (i = 0; i < count; i++) {
    g_s_pins[i].irq = gpio_to_irq(g_s_pins[i].gpio);
    err = request_irq(g_s_pins[i].irq, top_isr,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
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
                "SR501"); /*/dev/SR501*/

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
    free_irq(g_s_pins[i].irq, &g_s_pins[i]);
  }
}

/*----------------------注册入口出口-------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");