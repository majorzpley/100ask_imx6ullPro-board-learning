#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * ./process_excel data.csv result.csv
 * argc = 3
 * argv[0] = "./process_excel"
 * argv[1] = "data.csv"
 * argv[2] = "result.csv"
 */
/*
 * 返回值：n表示读到了一行数据的字节数(n>=0)
 *        -1表示读到文件尾部或者出错
 */
int read_line(int fd, unsigned char *buf) {
  // todo 循环读入一个字符
  // todo 如何判断读完一整行？读到0x0d，0x0a
  unsigned char c;
  int len;
  int i = 0;
  int err = 0;
  while (1) {
    len = read(fd, &c, 1);
    if (len <= 0) {
      err = -1;
      break;
    } else {
      if (c != '\n' && c != '\r') {
        buf[i] = c;
        i++;
      } else {
        // todo 遇到回车换行
        err = 0;
        break;
      }
    }
  }
  buf[i] = '\0';
  if (err && (i == 0)) {
    //- 读到文件尾部并且没有读到数据
    return -1;
  } else {
    return i;
  }
}

void process_data(unsigned char *data_buf, unsigned char *result_buf) {
  //- 示例1：data_buf = ",语文,数学,英语,总分,评价"
  //-       result_buf = ",语文,数学,英语,总分,评价"
  //- 示例2：data_buf = "张三,90,91,92,,"
  //-       result_buf = "张三,90,91,92,273,A+"
  char name[100];
  int scores[3];
  int sum;
  char *levels[] = {"A+", "A", "B"};
  int level;
  //- 前面三个字节是代表UTF-8编码格式的字节:hexdump -C score.csv查看0xef 0xbb
  // 0xbf
  if (data_buf[0] == 0xef) {
    strcpy(result_buf, data_buf);
  } else {
    sscanf(data_buf, "%[^,],%d,%d,%d", name, &scores[0], &scores[1],
           &scores[2]);
    // printf("result: %s,%d,%d,%d\n\r", name, scores[0], scores[1], scores[2]);
    // printf("result: %s -->get name---> %s\n", data_buf, name);
    sum = scores[0] + scores[1] + scores[2];
    if (sum >= 270) {
      level = 0;
    } else if (sum >= 240) {
      level = 1;
    } else {
      level = 2;
    }
    sprintf(result_buf, "%s,%d,%d,%d,%d,%s", name, scores[0], scores[1],
            scores[2], sum, levels[level]);
    printf("result: %s\r\n", result_buf);
  }
}

int main(int argc, char const *argv[]) {
  int fd_data, fd_result;
  int i;
  int len;
  unsigned char data_buf[1000];
  unsigned char result_buf[1000];
  if (argc != 3) {
    printf("Usage: %s <data csv file> <result csv file>\n", argv[0]);
    return -1;
  }

  fd_data = open(argv[1], O_RDONLY);
  if (fd_data < 0) {
    printf("can not open file %s\n", argv[1]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd_data = %d\n", fd_data);
  }

  fd_result = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd_result < 0) {
    printf("can not open file %s\n", argv[2]);
    printf("errno = %d\n", errno);
    printf("err: %s\n", strerror(errno));
    perror("open");
    return -1;
  } else {
    printf("fd_result = %d\n", fd_result);
  }
  while (1) {
    // todo 从数据文件读取1行
    len = read_line(fd_data, data_buf);
    if (len == -1) {
      break;
    }
    // if (len != 0) {
    //   printf("line: %s\n\r", data_buf);
    // }
    // todo 处理数据
    if (len != 0) {
      process_data(data_buf, result_buf);
      // todo 写入结果文件
      // write_data(fd_result, result_buf);
      write(fd_result, result_buf, strlen(result_buf));
      write(fd_result, "\r\n", 2);
    }
  }
  close(fd_data);
  close(fd_result);
  return 0;
}
