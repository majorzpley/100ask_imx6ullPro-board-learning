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
 *sendto/recvfrom
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

  // todo 创建udp socket套接字
  iSocketServer = socket(AF_INET, SOCK_DGRAM, 0);
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

  while (1) {
    iAddrLen = sizeof(struct sockaddr);
    ucRecvLen =
        recvfrom(iSocketServer, ucRecvBuffer, sizeof(ucRecvBuffer) - 1, 0,
                 (struct sockaddr *)&tSocketClientAddr, (socklen_t *)&iAddrLen);
    if (ucRecvLen > 0) {
      ucRecvBuffer[ucRecvLen] = '\0';
      printf("Get message from %s : %s", inet_ntoa(tSocketClientAddr.sin_addr),
             ucRecvBuffer);
    }
  }
  close(iSocketServer);
  return 0;
}
