#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd_fb;
static struct fb_var_screeninfo var;
static int screen_size;
static unsigned char *fb_base;
static unsigned int line_width;
static unsigned int pixel_width;

/// @brief 在LCD指定位置上输出指定颜色(描点)
/// @param x x坐标
/// @param y y坐标
/// @param color 颜色
void lcd_put_pixel(int x, int y, unsigned int color) {
  unsigned char *pen_8 =
      fb_base + y * line_width + x * pixel_width;  //- 定位到(x,y)坐标地址
  unsigned short *pen_16;
  unsigned int *pen_32;

  unsigned int red, green, blue;

  pen_16 = (unsigned short *)pen_8;
  pen_32 = (unsigned int *)pen_8;

  switch (var.bits_per_pixel) {
    case 8: {
      *pen_8 = color;
      break;
    }
    case 16: {
      /* 565 */
      red = (color >> 16) & 0xff;
      green = (color >> 8) & 0xff;
      blue = (color >> 0) & 0xff;
      color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
      *pen_16 = color;
      break;
    }
    case 32: {
      *pen_32 = color;
      break;
    }
    default: {
      printf("can't surport %dbpp\n", var.bits_per_pixel);
      break;
    }
  }
}

int main(int argc, char const *argv[]) {
  int i;

  fd_fb = open("/dev/fb0", O_RDWR);
  if (fd_fb < 0) {
    printf("can't open /dev/fb0");
    return -1;
  }
  if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var)) {
    printf("can't get var\n");
    return -1;
  }

  line_width = var.xres * var.bits_per_pixel / 8;   //- 每一行的字节数
  pixel_width = var.bits_per_pixel / 8;             //- 每一个像素的字节数
  screen_size = var.xres * var.yres * pixel_width;  //- 一帧数据的字节数
  fb_base = (unsigned char *)mmap(NULL, screen_size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd_fb, 0);  //- 设置共享内存
  if (fb_base == (unsigned char *)-1) {
    printf("can't mmap");
    return -1;
  }

  // todo 清屏：全部设置为白色
  memset(fb_base, 0xff, screen_size);

  // todo 测试：设置100个像素为红色
  for (i = 0; i < 100; i++)
    lcd_put_pixel(var.xres / 2 + i, var.yres / 2, 0xFF0000);

  munmap(fb_base, screen_size);  //- 解除映射
  close(fd_fb);
  return 0;
}
