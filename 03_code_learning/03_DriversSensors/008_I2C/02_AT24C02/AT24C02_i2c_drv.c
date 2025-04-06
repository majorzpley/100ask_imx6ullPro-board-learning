// todo i2c总线(搭配设备树)设备驱动模板
/*  头文件-------------------------------------------*/
#include "asm-generic/errno-base.h"
#include "asm/gpio.h"
#include "asm/uaccess.h"
#include "linux/delay.h"
#include "linux/device.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/gfp.h"
#include "linux/i2c.h"
#include "linux/init.h"
#include "linux/interrupt.h"
#include "linux/ioport.h"
#include "linux/irqreturn.h"
#include "linux/kdev_t.h"
#include "linux/kernel.h"
#include "linux/mmc/core.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/of.h"
#include "linux/of_gpio.h"
#include "linux/platform_device.h"
#include "linux/poll.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stddef.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*  自定义宏-----------------------------------------*/
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DEV: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

/*  变量---------------------------------------------*/
static struct i2c_client *g_s_p_client;
static int g_major;
static struct class *g_s_p_class;
static DECLARE_WAIT_QUEUE_HEAD(g_s_p_wait);
static struct fasync_struct *g_s_p_fasync;

/*  自定义函数---------------------------------------*/
/*  中断处理函数-------------------------------------*/

/*  file_operations接口函数--------------------------*/
static ssize_t i2c_drv_read(struct file *fp, char __user *buf, size_t size,
                            loff_t *offset) {
  int err;
  struct i2c_msg msgs[2];
  u8 *kernel_buf;

  /*从0读取size字节*/
  kernel_buf = kmalloc(size, GFP_KERNEL);

  /*初始化i2c_msg*/
  /*1.发起一次写操作：把0发给AT24C02，表示要从地址0读数据*/
  /*2.发起一次读操作，得到数据*/
  msgs[0].addr = g_s_p_client->addr;
  msgs[0].flags = 0; /*表示写操作*/
  msgs[0].buf = kernel_buf;
  kernel_buf[0] = 0; /*把数据0发给设备*/
  msgs[0].len = 1;

  msgs[1].addr = g_s_p_client->addr;
  msgs[1].flags = I2C_M_RD; /*表示读操作*/
  msgs[1].buf = kernel_buf;
  msgs[1].len = size;
  err = i2c_transfer(g_s_p_client->adapter, msgs, ARRAY_SIZE(msgs));

  /*copy_to_user*/
  err = copy_to_user(buf, kernel_buf, size);

  kfree(kernel_buf);

  return size;
}
/*
bug AT24C02的写最小单位为一页(8byte)，超过8字节就会覆盖掉前面的数据，已修改
bug 这里的size是unsigned int类型，应该始终大于0，但是这样写依然没问题?
*/
static ssize_t i2c_drv_write(struct file *fp, const char __user *buf,
                             size_t size, loff_t *offset) {

  int err;
  struct i2c_msg msgs[1];
  u8 kernel_buf[9];
  int len;
  u8 addr = 0;

  /*从0写入size字节*/
  while (size > 0) {
    if (size > 8) {
      len = 8;
    } else {
      len = size;
    }
    size -= len;
    /*copy_from_user*/
    err = copy_from_user(kernel_buf + 1, buf, len);
    buf += len;

    /*初始化i2c_msg*/
    /*1.发起一次写操作：把0发给AT24C02，表示要从地址0读数据*/
    /*2.发起一次读操作，得到数据*/
    msgs[0].addr = g_s_p_client->addr;
    msgs[0].flags = 0; /*表示写操作*/
    msgs[0].buf = kernel_buf;
    kernel_buf[0] = addr; /*写AT24C02的地址*/
    msgs[0].len = len + 1;
    addr += len;
    err = i2c_transfer(g_s_p_client->adapter, msgs, ARRAY_SIZE(msgs));
    mdelay(20); /*等待烧写完成*/
  }

  return size;
}
static unsigned int i2c_drv_poll(struct file *fp,
                                 struct poll_table_struct *table) {
  return POLLIN | POLLRDNORM;
}

static int i2c_drv_fasync(int fd, struct file *fp, int mode) {
  if (fasync_helper(fd, fp, mode, &g_s_p_fasync) >= 0) {
    return 0;
  } else {
    return -EIO;
  }
}

/*  file_operations定义------------------------------*/
static struct file_operations g_s_fops = {
    .owner = THIS_MODULE,
    .poll = i2c_drv_poll,
    .fasync = i2c_drv_fasync,
    .read = i2c_drv_read,
    .write = i2c_drv_write,
};

/*  i2c_driver接口函数-------------------------------*/

static int i2c_drv_probe(struct i2c_client *client,
                         const struct i2c_device_id *id) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  // struct i2c_adapter *adapter = client->adapter;
  // struct device_node *np = client->dev.of_node; /*获得设备树节点信息*/

  /*记录client*/
  g_s_p_client = client;

  /*注册字符设备*/
  g_major = register_chrdev(0, "template_i2c_chrdev", &g_s_fops);
  g_s_p_class = class_create(THIS_MODULE, "template_class");
  if (IS_ERR(g_s_p_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_major, "template_i2c_chrdev");
    return PTR_ERR(g_s_p_class);
  }
  device_create(g_s_p_class, NULL, MKDEV(g_major, 0), NULL,
                "AT24C02"); /*/dev/AT24C02*/

  return 0;
}

static int i2c_drv_remove(struct i2c_client *client) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*反注册字符设备*/
  device_destroy(g_s_p_class, MKDEV(g_major, 0));
  class_destroy(g_s_p_class);
  unregister_chrdev(g_major, "template_i2c_chrdev");

  return 0;
}

/*  i2c_driver定义---------------------------------*/
static const struct i2c_device_id at24c02_ids[] = {
    {"xxxxyyy", (kernel_ulong_t)NULL},
    {/*END OF LIST*/},
};
static const struct of_device_id g_s_dt_ids[] = {
    {
        .compatible = "100ask,at24c02",
    },
    {},
};
static struct i2c_driver g_s_i2c_drv = {
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "template_i2c_drv",
            .of_match_table = g_s_dt_ids,

        },
    .probe = i2c_drv_probe,
    .remove = i2c_drv_remove,
    .id_table = at24c02_ids, /*这个必须加上*/
};

/*  入口函数-----------------------------------------*/
static int __init dev_drv_init(void) {
  /*注册i2c_driver*/
  return i2c_add_driver(&g_s_i2c_drv);
}

/*  出口函数-----------------------------------------*/
static void __exit dev_drv_exit(void) {
  /*卸载i2c_driver*/
  i2c_del_driver(&g_s_i2c_drv);
}

/*  注册入口出口-------------------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");
