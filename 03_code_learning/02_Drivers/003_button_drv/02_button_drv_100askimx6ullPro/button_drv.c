#include "button_drv.h"
#include "asm/io.h"
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/stddef.h>

static int major = 0;
static struct button_operations *p_button_opr;
static struct class *button_class;

static int button_open(struct inode *inode, struct file *file) {

  int minor = iminor(inode);
  p_button_opr->init(minor);
  return 0;
}

static ssize_t button_read(struct file *file, char __user *buf, size_t len,
                           loff_t *offset) {

  int minor = iminor(file_inode(file));
  char level;
  int err;
  level = p_button_opr->read(minor);
  err = copy_to_user(buf, &level, 1);
  return 1;
}

int button_release(struct inode *inode, struct file *file) {
  int minor = iminor(inode);
  p_button_opr->unmap(minor);
}

static struct file_operations button_fops = {
    .open = button_open,
    .read = button_read,
    .release = button_release,
};

void register_button_operations(struct button_operations *opr) {
  int i;
  p_button_opr = opr;
  for (i = 0; i < opr->count; i++) {
    device_create(button_class, NULL, MKDEV(major, i), NULL,
                  "majorzpley_button%d", i);
  }
}

void unregister_button_operations(void) {
  int i;
  for (i = 0; i < p_button_opr->count; i++) {
    device_destroy(button_class, MKDEV(major, i));
  }
}

EXPORT_SYMBOL(register_button_operations);
EXPORT_SYMBOL(unregister_button_operations);

int __init button_init(void) {
  major = register_chrdev(0, "majorzpley_button", &button_fops);

  button_class = class_create(THIS_MODULE, "button_class");
  if (IS_ERR(button_class)) {
    return -1;
  }

  return 0;
}

void __exit button_exit(void) {
  class_destroy(button_class);
  unregister_chrdev(major, "majorzpley_button");
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");