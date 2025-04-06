//? gcc -o pthread pthread.c -lpthread
//- 查看线程：ps -T或者ls /proc/pid/task
// bug 此程序非常占用CPU资源
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static char g_buf[1000];
static int g_hasData = 0;

static void my_thread_func(void *data) {
  while (1) {
    // sleep(1);
    // todo 等待通知
    while (g_hasData == 0);
    // todo 打印
    printf("recv: %s\n", g_buf);
    g_hasData = 0;
  }
  return NULL;
}

int main(int argc, char const *argv[]) {
  pthread_t tid;
  int ret;
  // todo 1.创建”接收线程“
  ret = pthread_create(&tid, NULL, my_thread_func, NULL);
  if (ret != 0) {
    printf("pthread_create err!\n");
    return -1;
  }

  // todo 2.主线程读取标准输入，发给“接收线程”
  while (1) {
    fgets(g_buf, 1000, stdin);
    // todo 通知接收的子线程
    g_hasData = 1;
  }
  return 0;
}
