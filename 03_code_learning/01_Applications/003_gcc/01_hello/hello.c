//? 学习GCC的编译
//- 1.gcc -E -o hello.i hello.c 预处理-->展开宏，寻找头文件，去掉无效的代码
//- 2.gcc -S -o hello.s hello.i 编译-->生成汇编文件(语法错误是在此步骤发现)
//- 3.gcc -c -o hello.o hello.s 汇编-->生成机器码
//- 4.gcc -o hello hello.o
#define MAX 20
#define MIN 10

#define _DEBUG
#define SetBit(x) (1 << x)

#include <stdio.h>
int main(int argc, char const *argv[]) {
  printf("Hello, World.\n");
  printf("MAX = %d, MIN = %d, MAX + MIN = %d\n", MAX, MIN, MAX + MIN);
#ifdef _DEBUG
  printf("SetBit(5) = %d, SetBit(6) = %d\n", SetBit(5), SetBit(6));
  printf("SetBit( SetBit(2) ) = %d\n", SetBit(SetBit(2)));
#endif
  return 0;
}
