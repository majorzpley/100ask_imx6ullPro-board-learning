//? 制作静态库:
//- gcc -c -o sub.o sub.c
//- ar crs libsub.a sub.o
//? 制作动态库:
//- gcc -c -o sub.o sub.c
//- gcc -shared -o liubsub.so sub.o
void sub_fun(void) { printf("Sub fun!\n"); }
