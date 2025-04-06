//? gcc -o pthread pthread.c -lpthread
//- 查看线程：ps -T或者ls /proc/pid/task
// brief 使用互斥量来进行同步操作
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static sem_t g_sem;
static char g_buf[1000];
static pthread_mutex_t g_tMutex = PTHREAD_MUTEX_INITIALIZER;

static void my_thread_func(void *data) {
  while (1) {
    // todo 等待信号量
    sem_wait(&g_sem);  //- 此处会进入休眠
    // todo 访问g_buf前必须加锁
    pthread_mutex_lock(&g_tMutex);
    // todo 打印
    printf("recv: %s\n", g_buf);
    // todo 访问g_buf后必须解锁
    pthread_mutex_unlock(&g_tMutex);
  }
  return NULL;
}

int main(int argc, char const *argv[]) {
  pthread_t tid;
  int ret;
  char buf[1000];
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
    fgets(buf, 1000, stdin);
    // todo 访问g_buf前必须加锁
    pthread_mutex_lock(&g_tMutex);
    memcpy(g_buf, buf, strlen(buf));
    // todo 访问g_buf后必须解锁
    pthread_mutex_unlock(&g_tMutex);
    // todo 通知接收的子线程
    sem_post(&g_sem);
  }
  return 0;
}
