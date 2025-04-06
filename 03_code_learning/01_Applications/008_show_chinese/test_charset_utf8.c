//? 使用UTF-8保存
//> gcc -o test_charset_utf8 test_charset_utf8.c
//- str's len = 4
//- Hex code: 41 e4 b8 ad
//> gcc -finput-charset=UTF-8 -fexec-charset=GB2312 -o test_charset_utf8
// test_charset_utf8.c
//- str's len = 3
//- Hex code: 41 d6 d0
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