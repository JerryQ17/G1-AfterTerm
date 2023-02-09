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
  FILE* cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    if (fscanf(cfg, "RecordFlag=%d", &RecordFlag) < CFG_ITEM){   //读取设置失败 不影响游戏运行 只需要报告错误
      fprintf(stderr, "ServerInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ServerInit: Fail to find cfg.txt\n");
  if (RecordFlag){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    LogFilePtr = fopen(LOG_PATH, "a+");
    if (LogFilePtr == NULL) {   //日志文件不能正常写入 不记录信息
      RecordFlag = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ServerInit: Program start at %srecord = %d\n", ctime(&cur_time), RecordFlag);
    }
  }
  //服务器初始化
  int init_value = WSAStartup(MAKEWORD(2, 2), &data);
  if (init_value){
    errorf("ServerInit: WSAStartup Failed, code %d!\n", init_value);
    if (RecordFlag){
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
  if (RecordFlag){
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
    if (PlayerQuit){
      SocketSend(ClientSocket[ARG], "quit");
      ServerQuit(EXIT_SUCCESS);
    }
    /*ServerDataResolve(RecBuf, ARG, SERVER_TO_CLIENT);*/
    if (!strcmp(RecBuf, "ConnectRequest")) {
      sprintf(SendBuf, "%d", ARG);
      SocketSend(ClientSocket[ARG], SendBuf);
    }else if (!strcmp(RecBuf, "ClientReady")) {
      pthread_mutex_lock(&GameInitMutex);
      if (ARG) pthread_cond_signal(&GameInitCond);
      else pthread_cond_wait(&GameInitCond, &GameInitMutex);
      sprintf(SendBuf, "GameStart%d", 0);
      SocketSend(ClientSocket[ARG], SendBuf);
      pthread_mutex_unlock(&GameInitMutex);
    }else if (!strcmp(RecBuf, "BrickOrder")) {
      SocketSend(ClientSocket[ARG], BrickOrder);
    }else if (!strcmp(RecBuf, "quit")) {
      ServerDataResolve(RecBuf, ARG, SERVER_TO_CLIENT);
      break;
    }else{
      ServerDataResolve(RecBuf, ARG, SERVER_TO_CLIENT);
    }
    /*ServerDataResolve(SendBuf, ARG, CLIENT_TO_SERVER);
    SocketSend(ClientSocket[ARG], SendBuf);*/
    memset(SendBuf, 0, BUF_SIZE);
    memset(RecBuf, 0, BUF_SIZE);
  }
}

void ServerDataResolve(char* buf, int ThreadNum, bool flag){    //服务端数据解析
  if (flag == SERVER_TO_CLIENT) {
    if (!strcmp(buf, "quit")) {
      closesocket(ClientSocket[ThreadNum]);
      if (ClientNumber) {
        ClientNumber = 0;
        PlayerQuit = true;
        pthread_exit(NULL);
      }else ServerQuit(EXIT_SUCCESS);
    }else{
      pthread_mutex_lock(&TransmissionMutex[ThreadNum]);
      char* ptr = buf;
      if (strtol(ptr, &ptr, 10) == ThreadNum){
        BoardX[ThreadNum] = strtol(ptr, &ptr, 10);
        BoardY[ThreadNum] = strtol(ptr, &ptr, 10);
        BallX[ThreadNum] = strtof(ptr, &ptr);
        BallY[ThreadNum] = strtof(ptr, &ptr);
        for (int i = 0; i < BrickNum[difficulty]; i++){
          BrickLife[i] = strtol(ptr, &ptr, 10);
        }
      }else{
        errorf("Wrong ClientNum\n");
        //TODO
      }
      pthread_mutex_unlock(&TransmissionMutex[ThreadNum]);
    }
  }else if (flag == CLIENT_TO_SERVER) {
    pthread_mutex_lock(&TransmissionMutex[!ThreadNum]);
    sprintf(buf, "%d %d %d %f %f ", !ThreadNum,
            BoardX[!ThreadNum], BoardY[!ThreadNum], BallX[!ThreadNum], BallY[!ThreadNum]);
    for (int i = 0; i < BrickNum[difficulty]; i++){
      char temp[3] = {(char)(BrickLife[i] + 48), ' '};
      strcat(buf, temp);
    }
    pthread_mutex_unlock(&TransmissionMutex[!ThreadNum]);
  }
}

void BrickArrCreate(char* ret, int diff){
  memset(ret, 0, BUF_SIZE);
  bool choice[16][12] = {0};
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

void SocketReceive(SOCKET soc, char* buf){    //从soc接受buf
  recv(soc, buf, BUF_SIZE, 0);
  recordf("SocketReceive: soc = %llu\nReceiveMessage(%llu):\n%s\n", soc, strlen(buf), buf);
  Debug("ReceiveMessage(%llu):\n%s\n", strlen(buf), buf);
}

void SocketSend(SOCKET soc, const char* buf){   //发送buf给soc
  send(soc, buf, (int)(strlen(buf)) + 1, 0);
  recordf("SocketSend: soc = %llu\nSendMessage(%llu):\n%s\n", soc, strlen(buf), buf);
  Debug("SendMessage(%llu):\n%s\n", strlen(buf), buf);
}
