#include "server.h"

int main(){
  ServerInit();
  SocketCreate(&ServerSocket, &ServerAddr);
  SocketListen(ServerSocket, 20);
  //创建传输线程，互斥锁和条件变量
  pthread_mutex_init(&GameInitMutex, NULL);
  pthread_cond_init(&GameInitCond, NULL);
  BrickArrCreate(BrickOrder, 0);
  for (int i = 0; i < 2; i++) {
    SocketAccept(&ServerSocket, &ClientSocket[i], &ClientAddr[i]);
    ClientNumber++;
    pthread_create(&ClientThread[i], NULL, ServerTransmissionThread, &ThreadArg[i]);
    pthread_detach(ClientThread[i]);
  }
  pthread_exit(NULL);
}

void ServerInit(void){
  srand((UINT)time(NULL));
  //读取设置文件
  cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    if (fscanf(cfg, "record=%d", &record) < CFG_ITEM){   //读取设置失败 不影响游戏运行 只需要报告错误
      fprintf(stderr, "ServerInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ServerInit: Fail to find cfg.txt\n");
  if (record){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    LogFile = fopen(LOG_PATH, "a+");
    if (LogFile == NULL) {   //日志文件不能正常写入 不记录信息
      record = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ServerInit: Program start at %srecord = %d\n", ctime(&cur_time), record);
    }
  }
  //服务器初始化
  int init_value = WSAStartup(MAKEWORD(2, 2), &data);
  if (init_value){
    errorf("ServerInit: WSAStartup Failed, code %d!\n", init_value);
    if (record){
      time_t cur_time = time(NULL);
      recordf("ServerInit: Program quit with code %d at %s\n", init_value, ctime(&cur_time));
    }
    exit(init_value);
  }
  ServerIP_LAN(ServerIP);
}

void ServerIP_LAN(char *ip){    //获取本机局域网IP地址
  char HostName[BUF_SIZE] = {0};
  gethostname(HostName, BUF_SIZE);
  const struct hostent *host;
  host = gethostbyname(HostName);
  struct in_addr addr, temp;
  for (int i = 0; host->h_addr_list[i] != NULL; i++){   //筛选局域网IP
    temp.s_addr = *(u_long*)host->h_addr_list[i];
    char tmp[17] = {0};
    strncat(tmp, inet_ntoa(temp), 10);
    if (!strcmp(tmp, "192.168.1.")){
      addr.s_addr = *(u_long*)host->h_addr_list[i];
      break;
    }
  }
  strcat(ip, inet_ntoa(addr));
  if (*ip != 0) {
    recordf("ServerIP_LAN: ServerName: %s ServerIP: %s\n", HostName, ip);
    printf("ServerIP_LAN: %s\n", ip);
  }else{
    errorf("ServerIP_LAN: Fail to Get LAN IP\n");
    ServerQuit(IP_FAILURE);
  }
}

void ServerQuit(const int code){
  pthread_cond_destroy(&GameInitCond);
  pthread_mutex_destroy(&GameInitMutex);
  WSACleanup();
  if (record){
    time_t cur_time = time(NULL);
    recordf("ServerQuit: Program quit with code %d at %s\n", code, ctime(&cur_time));
  }
  exit(code);
}

void* ServerTransmissionThread(void* ThreadArgv){
  pthread_mutex_init(&TransmissionMutex[ARG], NULL);
  pthread_cond_init(&TransmissionCond[ARG], NULL);
  char SendBuf[BUF_SIZE] = {0}, RecBuf[BUF_SIZE] = {0};
  while (true) {
    SocketReceive(ClientSocket[ARG], RecBuf);
    if (!strcmp(RecBuf, "ConnectRequest")) {
      sprintf(SendBuf, "Client%d", ARG);
      SocketSend(ClientSocket[ARG], SendBuf);
    }else if (!strcmp(RecBuf, "ClientReady")){
      SocketSend(ClientSocket[ARG], BrickOrder);
      pthread_mutex_lock(&GameInitMutex);
      if (ARG) pthread_cond_signal(&GameInitCond);
      else pthread_cond_wait(&GameInitCond, &GameInitMutex);
      pthread_mutex_unlock(&GameInitMutex);
    }else if (!strcmp(RecBuf, "quit")) {
      ServerDataResolve(RecBuf, ARG, NET_TO_HOST);
      break;
    }else{
      ServerDataResolve(RecBuf, ARG, NET_TO_HOST);
    }
    memset(SendBuf, 0, BUF_SIZE);
    memset(RecBuf, 0, BUF_SIZE);
  }
}

char* ServerDataResolve(char* buf, int ThreadNum, bool flag){    //服务端数据解析
  static char rev[BUF_SIZE] = {0};
  memset(rev, 0, BUF_SIZE);
  if (flag == NET_TO_HOST) {
    if (!strcmp(buf, "quit")) {
      closesocket(ClientSocket[ThreadNum]);
      if (ClientNumber) {
        ClientNumber = 0;
        pthread_exit(NULL);
      }else ServerQuit(EXIT_SUCCESS);
    }else{
      pthread_mutex_lock(&TransmissionMutex[ThreadNum]);
      sscanf(buf, "Client%*d,LocalBall(%f,%f),LocalBoard(%d,%d)",
                      &BallX[ThreadNum], &BallY[ThreadNum], &BoardX[ThreadNum], &BoardY[ThreadNum]);
      pthread_mutex_unlock(&TransmissionMutex[ThreadNum]);
      pthread_mutex_lock(&TransmissionMutex[!ThreadNum]);
      sprintf(rev, "Client%d,NetBall(%f,%f),NetBoard(%d,%d)",
              !ThreadNum, BallX[!ThreadNum], BallY[!ThreadNum], BoardX[!ThreadNum], BoardY[!ThreadNum]);
      SocketSend(ClientSocket[ThreadNum], rev);
      pthread_mutex_unlock(&TransmissionMutex[!ThreadNum]);
      return rev;
    }
  }else if (flag == HOST_TO_NET){
    //TODO
  }
}

void BrickArrCreate(char* ret, int diff){
  bool choice[16][12] = {0};
  strcpy(ret, "BrickInitialize:");
  for (int i = 0; i < BrickNum[diff]; i++){
    char add[BUF_SIZE] = {0};
    int x = rand() % 15, y = rand() % 12, c = rand() % 5;
    while (choice[x][y]){
      x = rand() % 15;
      y = rand() % 12;
    }
    choice[x][y] = true;
    sprintf(add, "%d %d %d ", (x + 2) * 64, (y + 1) * 36, c);
    strcat(ret, add);
  }
}

void SocketCreate(SOCKET *soc, struct sockaddr_in *addr){
  *soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (*soc == INVALID_SOCKET) {
    errorf("SocketCreate: socket failed, return %llu, code %d\n", *soc, WSAGetLastError());
    ServerQuit(SOCKET_FAILURE);
  }
  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr(ServerIP);
  int i = 1024;
  for (; i < 65536; i++) {
    addr->sin_port = htons((u_short)i);  //端口
    int bv = bind(*soc, (struct sockaddr*)addr, sizeof(struct sockaddr));
    if (bv != SOCKET_ERROR) break;
  }
  if (i == 65536) {
    errorf("SocketCreate: bind failed, return %d, code %d\n", SOCKET_ERROR, WSAGetLastError());
    ServerQuit(BIND_FAILURE);
  }
  recordf("SocketCreate: socket %llu bind success(port %d)\n", *soc, i);
#ifdef DEBUG
  printf("SocketCreate: socket %llu bind success(port %d)\n", *soc, i);
#endif
}

void SocketListen(SOCKET soc, int backlog){
  int lv = listen(soc, backlog);
  if (lv) {
    errorf("SocketListen: listen failed, return %d, code %d\n", lv, WSAGetLastError());
    return;
  }
  recordf("SocketListen: listen success\n");
#ifdef DEBUG
  printf("SocketListen: listen success\n");
#endif
}

void SocketAccept(const SOCKET* ser, SOCKET* cli, struct sockaddr_in* cli_addr){
  int size = sizeof(struct sockaddr);
  *cli = accept(*ser, (struct sockaddr*)cli_addr, &size);
  if (*cli == INVALID_SOCKET) {
    errorf("SocketAccept: accept failed, return %llu, code %d\n", *cli, WSAGetLastError());
    return;
  }
  recordf("SocketAccept: accept success ser(soc%llu) cli(soc%llu)\n", *ser, *cli);
#ifdef DEBUG
  printf("SocketAccept: accept success ser(%llu) cli(%llu)\n", *ser, *cli);
#endif
}

void SocketReceive(SOCKET soc, char* buf){
  char temp[BUF_SIZE] = {0};
  int r, len = -1, len_temp, rf = 0, step = 0;
  while (true){
    r = recv(soc, temp, BUF_SIZE, 0);
    step++;
    if (step > RECEIVE_STEP){
      errorf("SocketReceive: receive failed, over step\n");
      ServerQuit(RECEIVE_FAILURE);
    }
    if (r == SOCKET_ERROR){
      errorf("SocketReceive: receive failed, return %d, code %d\n", r, WSAGetLastError());
      continue;
    }else if (rf == 0 || rf == 1) {
      int sr = sscanf(temp, "%d%s", &len_temp, temp);
      if (rf == 0 && sr == 1){
        rf = 1;
        len = len_temp;
      }else if (rf == 0 && sr == 2){
        rf = 2;
        len = len_temp;
        strcpy(buf, temp);
      }else if (rf == 1){
        int i = 1;
        for (; i <= MES_MAX_LEN; i++){
          len_temp /= 10;
          if (len_temp == 0) break;
        }
        len *= (int)pow(10, i);
        len += len_temp;
        if(sr == 2){
          rf = 2;
          strcpy(buf, temp);
        }
      }
    }else strcat(buf, temp);
    if (len == strlen(buf)) break;
  }
  recordf("SocketReceive: receive success(len = %d)\n", len);
#ifdef DEBUG
  printf("SocketReceive: receive success(len = %d)\n", len);
  printf("ReceiveMessage: %s\n", buf);
#endif
}

void SocketSend(SOCKET soc, const char* buf){
  char SendBuf[BUF_SIZE] = {0};
  strcpy(SendBuf, buf);
  int len = (int)strlen(SendBuf), len_temp = len, i = 1, step = 0, num[MES_MAX_LEN] = {0};
  for (; i <= MES_MAX_LEN; i++){
    num[i - 1] = len_temp / 10;
    len_temp /= 10;
    if (len_temp == 0) break;
  }
  len_temp = len;
  for (int j = 0; j < i; j++){
    int k = (int)pow(10, i - j - 1);
    num[j] = len_temp / k;
    len_temp %= k;
  }
  memmove(SendBuf + i, SendBuf, len + 1);
  for (int j = 0; j < i; j++) {
    *(SendBuf + j) = (char) (num[j] + 48);
  }
  //发送buf给soc
  len = (int)strlen(SendBuf);
  int s = send(soc, SendBuf, len + 1, 0), d = len - s;
  if (s == SOCKET_ERROR){
    errorf("SocketSend: send failed, return %d, code %d\n", s, WSAGetLastError());
    d = len;
  }
  //如果没有发送完就发送剩下的
  while (d > 0){
    if (step > SEND_STEP){
      errorf("SocketSend: send failed, over step\n");
      ServerQuit(SEND_FAILURE);
    }
    char temp[BUF_SIZE] = {0};
    for (int j = 0; j < d; j++){
      temp[j] = SendBuf[len - d + j];
    }
    s = send(soc, temp, (int)strlen(temp) + 1, 0);
    if (s == SOCKET_ERROR){
      errorf("SocketSend: send failed, return %d, code %d\n", s, WSAGetLastError());
      continue;
    }
    d -= s;
    step++;
  }
  recordf("SocketSend: send success(len = %d)\n", len);
#ifdef DEBUG
  printf("SocketSend: send success(len = %d)\nSendMessage: %s\n", len, SendBuf);
#endif
}

static inline void recordf(const char* format, ...){    //向日志文件中记录信息
  if (record){
    va_list ap;
    va_start(ap, format);
    vfprintf(LogFile, format, ap);
    va_end(ap);
    fflush(LogFile);
  }
}

static inline void errorf(const char* format, ...){   //在日志和标准误差流中记录错误
  if (record) {
    va_list rp;
    va_start(rp, format);
    vfprintf(LogFile, format, rp);
    va_end(rp);
    fflush(LogFile);
  }
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fflush(stderr);
}