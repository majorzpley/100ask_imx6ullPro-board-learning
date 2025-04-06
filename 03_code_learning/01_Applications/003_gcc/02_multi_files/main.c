//? 多文件编译链接
//- 整体编译：
//- gcc -o test main.c sub.c -v --> 预处理 --> 编译 --> main.o sub.o --> 汇编
//- 分别编译：
//- gcc -c -o main.o main.c
//- gcc -c -o sub.o sub.c
//- gcc -o test main.o sub.o
//? 静态库链接
//- gcc -o test main.o libsub.a --< test程序16760字节
//? 动态库链接
//- gcc -o test main.o -lsub -L./  --< test程序16728字节
//- 运行：export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./ && ./test_shared 
//--> 链接 --> test
#include "sub.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  int i;
  printf("Main fun!\n");
  sub_fun();
  return 0;
}
