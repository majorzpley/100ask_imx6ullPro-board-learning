#include <stdio.h>
extern void func_b();
extern void func_c();
int main() {
  func_b();
  func_c();
  return 0;
}
