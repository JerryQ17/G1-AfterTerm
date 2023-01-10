#include "client.h"

int SDL_main(int argc, char* argv[]){
  //客户端初始化
  ClientConfigInit();
  if (argc == 3) ClientInitAddr(argv[1], (unsigned short)strtol(argv[2], NULL, 10), LAN);
  else ClientInitAddr(NULL, 0, LAN);
  ClientLibInit();
  ClientLoadResource();
  ClientEventLoop();
  ClientQuit(EXIT_SUCCESS);
}

void ClientConfigInit(void){
  //读取设置文件
  cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    //读取设置失败 不影响游戏运行 只需要报告错误
    if (fscanf(cfg, "record=%d\nServerIP=%s\nServerPort=%hd", &record, ServerIP, &ServerPort) < CFG_ITEM){
      fprintf(stderr, "ClientConfigInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ClientConfigInit: Fail to find cfg.txt\n");
  if (record){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    log_file = fopen(LOG_PATH, "a+");
    if (log_file == NULL) {   //日志文件不能正常写入 不记录信息
      record = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ClientConfigInit: Program start at %srecord = %d\n", ctime(&cur_time), record);
    }
  }
  memset(&GameCondition, 0, sizeof(GameCondition));
  GameCondition.GameState = MAIN;
}

void ClientLibInit(void){
  //客户端网络初始化
  int WSAStartupValue = WSAStartup(MAKEWORD(2, 2), &data);
  if (WSAStartupValue){
    errorf("ClientLibInit: WSAStartup Failed, code %d\n", WSAStartupValue);
    time_t cur_time = time(NULL);
    recordf("ClientLibInit: Program quit with code %d at %s\n", WSAStartupValue, ctime(&cur_time));
    exit(WSAStartupValue);
  }
  //客户端SDL初始化
  if (SDL_Init(SDL_INIT_VIDEO)) {   //SDL初始化失败
    errorf("ClientLibInit: Cannot init video, %s\n", SDL_GetError());
    ClientQuit(SDL_INIT_ERROR);
    exit(SDL_INIT_ERROR);
  }
  Window = SDL_CreateWindow(TITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIN_WIDTH,WIN_HEIGHT,SDL_WINDOW_SHOWN);
  if (Window == NULL) {   //窗口初始化失败
    errorf("ClientLibInit: Cannot create Window, %s\n", SDL_GetError());
    ClientQuit(WINDOW_CREATE_ERROR);
    exit(WINDOW_CREATE_ERROR);
  }
  Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
  if (Renderer == NULL) {   //渲染器初始化失败
    errorf("ClientLibInit: Cannot create Renderer, %s\n", SDL_GetError());
    ClientQuit(RENDERER_CREATE_ERROR);
    exit(RENDERER_CREATE_ERROR);
  }
  if (TTF_Init()) {   //字体初始化失败
    errorf("ClientLibInit: Cannot init TTF, %s\n", TTF_GetError());
    ClientQuit(TTF_INIT_ERROR);
    exit(TTF_INIT_ERROR);
  }
  Font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
  NumberFont = TTF_OpenFont(FONT_PATH, NUMBER_SIZE);
  if (Font == NULL || NumberFont == NULL) {   //字体打开失败
    errorf("ClientLibInit: Cannot open Font, %s\n", TTF_GetError());
    ClientQuit(TTF_OPEN_FONT_ERROR);
    exit(TTF_OPEN_FONT_ERROR);
  }
  if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) == -1) {
    errorf("ClientLibInit: Cannot open Audio, %s\n", SDL_GetError());   //音频打开失败
    ClientQuit(MIX_OPEN_AUDIO_ERROR);
    exit(MIX_OPEN_AUDIO_ERROR);
  }
}

void ClientInitAddr(char *IP, unsigned short port, bool flag){      //获取服务器的IP地址以及端口,flag控制局域网还是广域网
  char tmpIP[8] = {0};
  strncat(tmpIP, ServerIP, 7);
  if ((!flag || !strcmp(tmpIP, "192.168")) && ServerPort > 1023) {  //cfg文件
    recordf("Valid Initial Network Config, IP = %s, Port = %hd\n", ServerIP, ServerPort);
#ifdef DEBUG
    printf("Valid Initial Network Config, IP = %s, Port = %hd\n", ServerIP, ServerPort);
#endif
    return;
  }
  if (IP != NULL && port > 1023) {                                  //命令行参数
    strncpy(tmpIP, IP, 7);
    if ((!flag || !strcmp(tmpIP, "192.168")) && port > 1023) {
      strcpy(ServerIP, IP);
      ServerPort = port;
      return;
    }
  }
  errorf("Invalid Initial Network Config, current IP = %s, current Port = %hd\n", IP, port);
  int step = 0;
  while (true) {                                                    //用户直接输入
    step++;
    if (step > ADDR_STEP){
      errorf("ClientInitAddr: init addr failed, over step\n");
      ClientQuit(IP_FAILURE);
    }
    printf("Input Server IP:\n");
    scanf("%s", ServerIP);
    printf("Input Server Port:\n");
    scanf("%hd", &ServerPort);
    strncpy(tmpIP, ServerIP, 7);
    if ((!flag || !strcmp(tmpIP, "192.168")) && ServerPort > 1023){
      recordf("Valid Network Config, IP = %s, Port = %hd\n", IP, port);
      printf("Valid Network Config, IP = %s, Port = %hd\n", IP, port);
      return;
    }
    errorf("Invalid Network Config, current IP = %s, current Port = %hd\n", IP, port);
  }
}

void ClientLoadResource(void){
  //mainUI
  MainSurface = IMG_Load(MAIN_UI_PATH);
  MainTexture = SDL_CreateTextureFromSurface(Renderer, MainSurface);
  MainRect = (SDL_Rect){0, 0, WIN_WIDTH, WIN_HEIGHT};
  /*//gameUI
  GameSurface = IMG_Load("img/gameUI.png");
  GameTexture = SDL_CreateTextureFromSurface(Renderer, GameSurface);
  GameRect = (SDL_Rect){0, 0, WIN_WIDTH, WIN_HEIGHT};
  //chess
  RedSurface = IMG_Load("img/RedChess.png");
  RedTexture = SDL_CreateTextureFromSurface(Renderer, RedSurface);
  GreenSurface = IMG_Load("img/GreenChess.png");
  GreenTexture = SDL_CreateTextureFromSurface(Renderer, GreenSurface);
  YellowSurface = IMG_Load("img/YellowChess.png");
  YellowTexture = SDL_CreateTextureFromSurface(Renderer, YellowSurface);
  BlueSurface = IMG_Load("img/BlueChess.png");
  BlueTexture = SDL_CreateTextureFromSurface(Renderer, BlueSurface);
  recordf("LoadPicture: Complete!\n");*/
  //bgm
  bgm = Mix_LoadMUS(MAIN_BGM_PATH);
  Mix_VolumeMusic(45);
  Mix_PlayMusic(bgm, -1);
}

void ClientEventLoop(void){
  SDL_Event event;
  while (SDL_WaitEvent(&event)) {
    //渲染mainUI
    ClientUIRender();
    switch (event.type) {
      case SDL_QUIT:  //关闭窗口
        recordf("ClientEventLoop: Quit by SDL_QUIT\n");
        ClientQuit(EXIT_SUCCESS);
        break;
      case SDL_KEYDOWN: //按下键盘
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE: //Esc
            recordf("ClientEventLoop: Quit by Esc\n");
            ClientQuit(EXIT_SUCCESS);
            break;
          default:break;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        recordf("ClientEventLoop: Mouse button down (%d, %d)\n", event.button.x, event.button.y);
#ifdef DEBUG
        printf("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
#endif
        break;
      case SDL_MOUSEBUTTONUP:
        recordf("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
        if (GameCondition.GameState == MAIN) {    //主界面
          if (event.button.x > SINGLE_MIN_X && event.button.x < SINGLE_MAX_X
              && event.button.y > SINGLE_MIN_Y && event.button.y < SINGLE_MAX_Y) {  //开始
            recordf("ClientEventLoop: Game start, one player\n");
            ClientGameInit(SINGLE);
          } else if (event.button.x > DOUBLE_MIN_X && event.button.x < DOUBLE_MAX_X
              && event.button.y > DOUBLE_MIN_Y && event.button.y < DOUBLE_MAX_Y) {
            recordf("ClientEventLoop: Game start, two player\n");
            ClientGameInit(DOUBLE);
          } else if (event.button.x > README_MIN_X && event.button.x < README_MAX_X
              && event.button.y > README_MIN_Y && event.button.y < README_MAX_Y) { //帮助
            recordf("ClientEventLoop: Press README button\n");
            int sv = system("start README.md");
            if (sv) {
              ClientUIRender();
              ClientDrawText("Failed to get help:(", 400, 569, true);
              errorf("ClientEventLoop: Failed to open readme, return %d\n", sv);
            }
          }
        }else {     //游戏界面
          //TODO
        }
        break;
      default:break;
    }
  }
}

void ClientGameInit(bool flag){
  if (flag == SINGLE){
    GameCondition.GameState = ONE_PLAYER;
  }else{
    GameCondition.GameState = TWO_PLAYER;
    pthread_create(&TransmissionThread, NULL, ClientTransmissionThread, NULL);
  }
}

void ClientQuit(int code){
  SDL_DestroyTexture(MainTexture);
  SDL_FreeSurface(MainSurface);
  Mix_HaltMusic();
  Mix_FreeMusic(bgm);
  Mix_Quit();
  TTF_CloseFont(Font);
  TTF_CloseFont(NumberFont);
  TTF_Quit();
  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  WSACleanup();
  time_t cur_time = time(NULL);
  recordf("ClientQuit: Program quit with code %d at %s\n", code, ctime(&cur_time));
  exit(code);
}

void ClientConnect(SOCKET *server, struct sockaddr_in *server_addr){
  *server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  memset(server_addr, 0, sizeof(struct sockaddr_in));  //每个字节都用0填充
  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = inet_addr(ServerIP);
  server_addr->sin_port = htons(ServerPort);  //端口
  int cv, step = 0;
  while (true){
    step++;
    if (step >= CONNECT_STEP){
      errorf("ClientConnect: connect Server failed, over step\n");
      ClientDrawText("Server Connect Failed", USER_TIP_X, USER_TIP_Y, true);
      SDL_Delay(CONNECT_DELAY);
      ClientQuit(CONNECT_FAILURE);
    }
    cv = connect(ServerSocket, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
    if (cv == SOCKET_ERROR){
      errorf("ClientConnect: connect failed, return %d, code %d\n", cv, WSAGetLastError());
      continue;
    }else {
      ClientDrawText("Server Connected", USER_TIP_X, USER_TIP_Y, true);
      SDL_Delay(CONNECT_DELAY);
      break;
    }
  }
  recordf("ClientConnect: connect success return %d code %d\n", cv, WSAGetLastError());
#ifdef DEBUG
  printf("ClientConnect: connect success return %d code %d\n", cv, WSAGetLastError());
#endif
}

void* ClientTransmissionThread(void* arg){
  ClientDrawText("Server Connecting...", USER_TIP_X, USER_TIP_Y, true);
  //向服务器发起请求
  ClientConnect(&ServerSocket, &ServerAddr);
  //获取用户输入的字符串并发送给服务器
  char Send[BUF_SIZE] = {0}, Receive[BUF_SIZE] = {0};
  while (true) {
    printf("Input a string:\n");
    scanf("%s", Send);
    SocketSend(ServerSocket, Send);
    //接收服务器传回的数据
    SocketReceive(ServerSocket, Receive);
    //输出接收到的数据
    printf("Message form server: %s\n", Receive);
    if (!strcmp("4quit", Send)) ClientQuit(EXIT_SUCCESS);
    memset(Send, 0, BUF_SIZE);
    memset(Receive, 0, BUF_SIZE);
  }
  return NULL;
}

void ClientUIRender(void){
  SDL_RenderClear(Renderer);
  if (GameCondition.GameState == MAIN) {
    SDL_RenderCopy(Renderer, MainTexture, NULL, &MainRect);
  }else {
    //TODO
  }
  SDL_RenderPresent(Renderer);
}

void SocketReceive(SOCKET soc, char* buf){
  char temp[BUF_SIZE] = {0};
  int r, len, len_temp, rf = 0, step = 0;
  while (true){
    r = recv(soc, temp, BUF_SIZE, 0);
    step++;
    if (step > RECEIVE_STEP){
      errorf("SocketReceive: receive failed, over step\n");
      ClientQuit(RECEIVE_FAILURE);
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
  printf("Message: %s\n", buf);
#endif
}

void SocketSend(SOCKET soc, char* buf){
  //在buf前加上自身长度
  int len = (int)strlen(buf), len_temp = len, i = 1, step = 0, num[MES_MAX_LEN] = {0};
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
  memmove(buf + i, buf, len);
  for (int j = 0; j < i; j++) {
    *(buf + j) = (char) (num[j] + 48);
  }
  //发送buf给soc
  len = (int)strlen(buf);
  int s = send(soc, buf, len + 1, 0), d = len - s;
  if (s == SOCKET_ERROR){
    errorf("SocketSend: send failed, return %d, code %d\n", s, WSAGetLastError());
    d = len;
  }
  //如果没有发送完就发送剩下的
  while (d > 0){
    if (step > SEND_STEP){
      errorf("SocketSend: send failed, over step\n");
      ClientQuit(SEND_FAILURE);
    }
    char temp[BUF_SIZE] = {0};
    for (int j = 0; j < d; j++){
      temp[j] = buf[len - d + j];
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
  printf("SocketSend: send success(len = %d)\nMessage: %s", len, buf);
#endif
}

void ClientDrawText(char *text, int x, int y, bool pre){   //根据参数渲染文本
  FontSurface = TTF_RenderUTF8_Blended(Font, text, FontColor);
  FontTexture = SDL_CreateTextureFromSurface(Renderer, FontSurface);
  FontRect = (SDL_Rect){x, y, FontSurface->w, FontSurface->h};
  SDL_RenderCopy(Renderer, FontTexture, NULL, &FontRect);
  if (pre) SDL_RenderPresent(Renderer);
  //末处理
  SDL_FreeSurface(FontSurface);
  SDL_DestroyTexture(FontTexture);
}

void ClientDrawNumber(int num, int x, int y, bool pre){   //根据参数渲染数字
  //处理数字
  int flag = 1, text_num = 0, number[11] = {0};
  char text[11] = {0};
  for (int i = 0; i < 11; i++){
    number[i] = num / (int)pow(10, 10 - i);
    num -= number[i] * (int)pow(10, 10 - i);
  }
  for (int i = 0; i < 11; i++){
    if (number[i] && flag) flag = 0;
    if (!flag) text[text_num++] = (char)(number[i] + 48);
  }
  if (!text[0]) text[0] = '0';
  //渲染数字
  NumberFontSurface = TTF_RenderUTF8_Blended(NumberFont, text, NumberFontColor);
  NumberFontTexture = SDL_CreateTextureFromSurface(Renderer, NumberFontSurface);
  NumberFontRect = (SDL_Rect){x, y, NumberFontSurface->w, NumberFontSurface->h};
  SDL_RenderCopy(Renderer, NumberFontTexture, NULL, &NumberFontRect);
  if (pre) SDL_RenderPresent(Renderer);
  //末处理
  SDL_FreeSurface(NumberFontSurface);
  SDL_DestroyTexture(NumberFontTexture);
}

void recordf(const char* format, ...){    //向日志文件中记录信息
  if (record){
    va_list ap;
    va_start(ap, format);
    vfprintf(log_file, format, ap);
    va_end(ap);
  }
}

void errorf(const char* format, ...){   //在日志和标准误差流中记录错误
  if (record) {
    va_list rp;
    va_start(rp, format);
    vfprintf(log_file, format, rp);
    va_end(rp);
    fflush(log_file);
  }
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}