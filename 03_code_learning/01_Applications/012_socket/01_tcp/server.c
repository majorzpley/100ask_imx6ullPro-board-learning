//? man 7 ip 搜索/sockaddr_in
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 8888
#define BACKLOG 10
/*
 *socket
 *bind
 *listen
 *accept
 *send/recv
 */
int main(int argc, char const *argv[]) {
  int iSocketServer;
  int iRet;
  int iSocketClient;
  int iAddrLen;
  struct sockaddr_in tSocketServerAddr;  //- 服务端的地址信息
  struct sockaddr_in tSocketClientAddr;  //- 客户端的地址信息
  unsigned char ucRecvBuffer[1000];
  ssize_t ucRecvLen;
  int iClientNum = -1;

  signal(SIGCHLD, SIG_IGN);  //- 忽略僵尸进程发出的孤儿信号

  // todo 创建socket套接字
  iSocketServer = socket(AF_INET, SOCK_STREAM, 0);
  if (iSocketServer == -1) {
    printf("socket error!\n");
    return -1;
  }

  // todo 绑定ip和端口号
  tSocketServerAddr.sin_family = AF_INET;
  tSocketServerAddr.sin_port = htons(SERVER_PORT);  // todo host to net, short
  tSocketServerAddr.sin_addr.s_addr = INADDR_ANY;   // todo 表示本机所有ip
  memset(tSocketServerAddr.sin_zero, 0, 8);
  iRet = bind(iSocketServer, (const struct sockaddr *)&tSocketServerAddr,
              sizeof(struct sockaddr));
  if (iRet == -1) {
    printf("bind error!\n");
    return -1;
  }

  // todo 开始侦听
  iRet = listen(iSocketServer, BACKLOG);
  if (iRet == -1) {
    printf("listen error!\n");
    return -1;
  }

  while (1) {
    iAddrLen = sizeof(struct sockaddr);
    iSocketClient = accept(iSocketServer, (struct sockaddr *)&tSocketClientAddr,
                           (socklen_t *)&iAddrLen);
    if (iSocketClient != -1) {
      iClientNum++;  //- 建立连接的客户端数目加一
      printf("Get connect from client%d : %s!\n", iClientNum,
             inet_ntoa(tSocketClientAddr.sin_addr));
      if (!fork()) {
        // todo 子进程代码
        while (1) {
          // todo 接收客户端的数据并显示在终端上
          ucRecvLen =
              recv(iSocketClient, ucRecvBuffer, sizeof(ucRecvBuffer) - 1, 0);
          if (ucRecvLen <= 0) {
            close(iSocketClient);
            return -1;
          } else {
            ucRecvBuffer[ucRecvLen] = '\0';
            printf("Get Message from Clinet id %d: %s\n", iClientNum,
                   ucRecvBuffer);
          }
        }
      }
    }
  }
  close(iSocketServer);
  return 0;
}
