//? 使用GBK2312保存
//> gcc -o test_charset_ansi test_charset_ansi.c
//- str's len = 3
//- Hex code : 41 d6 d0
//> gcc -finput-charset=GB2312 -fexec-charset=UTF-8 -o test_charset_ansi
// test_charset_ansi.c
//- str's len = 4
//- Hex code: 41 e4 b8 ad

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  char *str = "A中";
  int i;

  printf("str's len = %d\n", (int)strlen(str));
  printf("Hex code: ");
  for (i = 0; i < strlen(str); i++) {
    printf("%02x ", (unsigned char)str[i]);
  }
  printf("\n");
  return 0;
}