//? gcc -o pthread pthread.c -lpthread
//- 查看线程：ps -T或者ls /proc/pid/task
// bug:此程序没有考虑同步情况，如果在子进程输出的同时，父进程进行读取操作，数据就会乱掉
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

static sem_t g_sem;
static char g_buf[1000];

static void my_thread_func(void *data) {
  while (1) {
    // todo 等待信号量
    sem_wait(&g_sem);  //- 此处会进入休眠
    // todo 打印
    printf("recv: %s\n", g_buf);
  }
  return NULL;
}

int main(int argc, char const *argv[]) {
  pthread_t tid;
  int ret;
  // todo 使用信号量来改善CPU资源占用过高的bug
  sem_init(&g_sem, 0, 0);
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
    sem_post(&g_sem);
  }
  return 0;
}
