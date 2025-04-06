//?IMX6ULL用/dev/ttymxc5，相关管脚复用信息在"MYC-Y6ULX_Pin_list_V13.xlsx"中可以找到，对应串口6
//?STM32MP157用/dev/ttySTM3
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop) {
  struct termios newtio, oldtio;
  //- 获取驱动程序中的终端默认参数
  if (tcgetattr(fd, &oldtio) != 0) {
    perror("SetupSerial 1");
    return -1;
  }

  bzero(&newtio, sizeof(struct termios));
  newtio.c_cflag |= CLOCAL | CREAD;
  newtio.c_cflag &= ~CSIZE;

  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  newtio.c_oflag &= ~OPOST;
  //- 设置数据位的个数
  switch (nBits) {
    case 7:
      newtio.c_cflag |= CS7;
      break;
    case 8:
      newtio.c_cflag |= CS8;
      break;
  }
  //- 设置奇偶校验位
  switch (nEvent) {
    case 'O':
      //- 奇校验
      newtio.c_cflag |= PARENB;
      newtio.c_cflag |= PARODD;
      newtio.c_iflag |= (INPCK | ISTRIP);
      break;
    case 'E':
      //- 偶校验
      newtio.c_cflag |= PARENB;
      newtio.c_cflag &= ~PARODD;
      newtio.c_iflag |= (INPCK | ISTRIP);
      break;
    case 'N':
      newtio.c_cflag &= ~PARENB;
      break;
  }
  //- 设置波特率
  switch (nSpeed) {
    case 2400:
      cfsetispeed(&newtio, B2400);
      cfsetospeed(&newtio, B2400);
      break;
    case 4800:
      cfsetispeed(&newtio, B4800);
      cfsetospeed(&newtio, B4800);
      break;
    case 9600:
      cfsetispeed(&newtio, B9600);
      cfsetospeed(&newtio, B9600);
      break;
    case 115200:
      cfsetispeed(&newtio, B115200);
      cfsetospeed(&newtio, B115200);
      break;
    default:
      cfsetispeed(&newtio, B9600);
      cfsetospeed(&newtio, B9600);
      break;
  }
  //- 设置停止位
  if (nStop == 1) {
    newtio.c_cflag &= ~CSTOPB;
  } else if (nStop == 2) {
    newtio.c_cflag |= CSTOPB;
  }

  /*
   *读取数据时的最小字节数：没读到这些数据我就不返回！
   */
  newtio.c_cc[VMIN] = 1;
  /*
   *等待第1个数据的时间：
   *比如VMIN设为10表示至少读到10个数据才返回，
   *但是没有数据总不能一直等吧？可以设置VTIME(单位是10秒)
   *假设VTIME=1，表示：
   *    10秒内一个数据都没有的话就返回
   *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
   */
  newtio.c_cc[VTIME] = 0;  //-0表示没有数据一直等待

  tcflush(fd, TCIFLUSH);

  if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
    perror("com set error");
    return -1;
  }
  //   printf("com set done!\n");
  return 0;
}

int open_port(const char *com) {
  int fd;
  //   fd = open(com, O_RDWR | O_NOCTTY | O_NDELAY);
  //- 可读可写，O_NOCTTY不要将此文件作为控制终端使用
  fd = open(com, O_RDWR | O_NOCTTY);
  if (fd == -1) {
    perror("open_port");
    return -1;
  }
  //- 设置串口为非阻塞状态(读数据时不等待，无数据则返回0)
  ////   if (fcntl(fd, F_SETFL, FNDELAY) < 0) {
  //- 设置串口为阻塞状态(读数据时，无数据则休眠)
  if (fcntl(fd, F_SETFL, 0) < 0) {
    printf("fcntl failed!\n");
    return -1;
  }
  return fd;
}
/*
 * ./serial_send_recv /dev/ttymxc5
 */
int main(int argc, char const *argv[]) {
  int fd;
  int iRet;
  char c;
  if (argc != 2) {
    printf("Usage: \n");
    printf("%s </dev/ttySAC1 or other>\n", argv[0]);
    return -1;
  }

  // todo 1.open
  fd = open_port(argv[1]);
  if (fd < 0) {
    printf("open %s err!\n", argv[1]);
    return -1;
  }

  // todo 2.setup 115200,8N1,RAW mode,return data imediately
  iRet = set_opt(fd, 115200, 8, 'N', 1);
  if (iRet) {
    printf("set port err!\n");
    return -1;
  }

  // todo 3.write and read
  printf("Enter a char: ");
  while (1) {
    scanf("%c", &c);
    iRet = write(fd, &c, 1);
    iRet = read(fd, &c, 1);  //- 无数据时返回0
    if (iRet == 1) {
      printf("get: %#x %c\n", c, c);
    } else {
      printf("can not get data\n");
    }
  }
  close(fd);
  return 0;
}
