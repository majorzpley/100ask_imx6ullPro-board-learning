//? 装载驱动:insmod hello_drv.ko
//? 卸载驱动:rmmod hello_drv
//? 查看驱动信息:modinfo hello_drv.ko
//? 查看驱动日志:dmesg | grep hello_drv
//? 查看驱动设备:ls /dev/hello | cat /proc/devices
//? 临时修改内核终端的日志打印级别: cat /proc/sys/kernel/printk
//- echo "7 4 1 3">/proc/sys/kernel/printk
//? 永久修改内核终端的日志打印级别: vim /etc/sysctl.conf
//? 永久修改内核终端的日志打印级别:
//- echo "kernel.printk = 7 4 1 3">>/etc/sysctl.conf
// bug通过ssh或者telent建立的命令行无法接受printk的输出，可以通过串口命令工具查看到。
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/uaccess.h>

// todo 1.确定主设备号，也可以让内核分配
static int major = 0;
static char kernel_buf[1024] = {0};
static struct class *hello_class;

// todo 3.实现对应的drv_open/drv_read/drv_write等函数，填入file_operations结构体
static int hello_drv_open(struct inode *node, struct file *file) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}
static int hello_drv_close(struct inode *node, struct file *file) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}
static ssize_t hello_drv_read(struct file *file, char __user *buf, size_t size,
                              loff_t *offset) {
  int err;
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  err = copy_to_user((void *__user)buf, (const void *)kernel_buf,
                     min(size, sizeof(kernel_buf)));
  return min(size, sizeof(kernel_buf));
}
static ssize_t hello_drv_write(struct file *file, const char __user *buf,
                               size_t size, loff_t *offset) {
  int err;
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  err = copy_from_user(kernel_buf, buf, min(size, sizeof(kernel_buf)));
  return min(size, sizeof(kernel_buf));
}

// todo 2.定义自己的file_operations结构体 -
static struct file_operations hello_drv_fops = {
    .owner = THIS_MODULE,
    .open = hello_drv_open,
    .release = hello_drv_close,
    .read = hello_drv_read,
    .write = hello_drv_write,
};

// todo 4.把 file_operations 结构体告诉内核：注册驱动程序

// todo 5.谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
static int __init hello_init(void) {
  int err;

  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  major = register_chrdev(0, "hello", &hello_drv_fops);

  //- 创建/dev/hello设备
  hello_class = class_create(THIS_MODULE, "hello_class");
  err = PTR_ERR(hello_class);
  if (IS_ERR(hello_class)) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    printk("class_create failed\n");
    unregister_chrdev(major, "hello");
    return -1;
  }
  //- 次设备号
  // brief 此函数位于include/linux/device.h 1153行
  device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello");
  return 0;
}

// todo 6.有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用unregister_chrdev
static void __exit hello_exit(void) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  device_destroy(hello_class, MKDEV(major, 0));
  class_destroy(hello_class);
  unregister_chrdev(major, "hello");
}

// todo 6.其他完善：提供设备信息，自动创建设备节点：class_create, device_create
module_init(hello_init);
MODULE_AUTHOR("majorzpley wyx1214844230@outlook.com");
module_exit(hello_exit);
MODULE_LICENSE("GPL");