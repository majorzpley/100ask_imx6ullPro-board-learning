//? man 7 ip 搜索/sockaddr_in
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 8888
/*
 *socket
 *connect
 *send/recv
 */

int main(int argc, char const *argv[]) {
  int iRet;
  int iSocketClient;
  int iSendLen;
  unsigned char ucSendBuf[1000];
  struct sockaddr_in tSocketServerAddr;

  if (argc != 2) {
    printf("Usage:\n");
    printf("%s <server ip>\n", argv[0]);
    return -1;
  }

  iSocketClient = socket(AF_INET, SOCK_STREAM, 0);

  // todo 绑定ip和端口号
  tSocketServerAddr.sin_family = AF_INET;
  // todo host to net, short
  tSocketServerAddr.sin_port = htons(SERVER_PORT);
  // todo 获取ip地址
  if (0 == inet_aton(argv[1], &tSocketServerAddr.sin_addr)) {
    printf("invalid server_ip!\n");
    return -1;
  }

  memset(tSocketServerAddr.sin_zero, 0, 8);
  iRet = connect(iSocketClient, (const struct sockaddr *)&tSocketServerAddr,
                 sizeof(struct sockaddr));
  if (iRet == -1) {
    printf("connect error!\n");
    return -1;
  }
  while (1) {
    if (fgets(ucSendBuf, 999, stdin)) {
      iSendLen = send(iSocketClient, ucSendBuf, strlen(ucSendBuf), 0);
      if (iSendLen <= 0) {
        close(iSocketClient);
        return -1;
      }
    }
  }
  close(iSocketClient);
  return 0;
}
