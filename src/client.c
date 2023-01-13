#include "client.h"

int SDL_main(int argc, char* argv[]){
  //客户端初始化
  ClientCfgInit();
  if (argc == 3) ClientInitAddr(argv[1], (u_short)strtol(argv[2], NULL, 10), LAN);
  else {
    errorf("Invalid argv\n");
    ClientInitAddr(NULL, 0, LAN);
  }
  ClientLibInit();
  ClientLoadResource();
  ClientBGM();
  ClientEventLoop();
  ClientQuit(EXIT_SUCCESS);
}

void ClientCfgInit(void){
  //读取设置文件
  cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    //读取设置失败 不影响游戏运行 只需要报告错误
    if (fscanf(cfg, "record=%d\nServerIP=%s\nServerPort=%hd", &record, ServerIP, &ServerPort) < CFG_ITEM){
      fprintf(stderr, "ClientCfgInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ClientCfgInit: Fail to find cfg.txt\n");
  if (record){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    log_file = fopen(LOG_PATH, "a+");
    if (log_file == NULL) {   //日志文件不能正常写入 不记录信息
      record = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ClientCfgInit: Program start at %srecord = %d\n", ctime(&cur_time), record);
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

void ClientInitAddr(char *IP, u_short port, bool flag){      //获取服务器的IP地址以及端口,flag控制局域网还是广域网
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
    char PortInput[10] = {0};
    printf("Input Server Port:\n");
    scanf("%s", PortInput);
    ServerPort = (u_short)strtol(PortInput, NULL, 10);
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
  MainRect = (SDL_Rect){0, 0, WIN_WIDTH, WIN_HEIGHT};
  //gameUI
  GameSurface = IMG_Load("img/gameUI.png");
  GameRect = (SDL_Rect){0, 0, WIN_WIDTH, WIN_HEIGHT};
  recordf("LoadPicture: Complete!\n");
}

void ClientEventLoop(void){
  SDL_Event event;
  while (SDL_WaitEvent(&event)) {
    //渲染mainUI
    ClientRender();
    switch (event.type) {
      case SDL_QUIT:  //关闭窗口
        recordf("ClientEventLoop: Quit by SDL_QUIT\n");
        if (GameCondition.GameState != MAIN) ClientGameQuit();
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
        printf("ClientEventLoop: Mouse button down (%d, %d)\n", event.button.x, event.button.y);
#endif
        break;
      case SDL_MOUSEBUTTONUP:
        recordf("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
        if (GameCondition.GameState == MAIN) {    //主界面
          if (event.button.x > SINGLE_MIN_X && event.button.x < SINGLE_MAX_X
              && event.button.y > SINGLE_MIN_Y && event.button.y < SINGLE_MAX_Y) {  //单人
            recordf("ClientEventLoop: Game start, one player\n");
            GameCondition.GameState = ONE_PLAYER;
            ClientGameInit();
          } else if (event.button.x > DOUBLE_MIN_X && event.button.x < DOUBLE_MAX_X //多人
              && event.button.y > DOUBLE_MIN_Y && event.button.y < DOUBLE_MAX_Y) {
            recordf("ClientEventLoop: Game start, two player\n");
            GameCondition.GameState = TWO_PLAYER;
            ClientGameInit();
          } else if (event.button.x > README_MIN_X && event.button.x < README_MAX_X
              && event.button.y > README_MIN_Y && event.button.y < README_MAX_Y) { //帮助
            recordf("ClientEventLoop: Press README button\n");
            int sv = system("start README.md");
            if (sv) {
              ClientRender();
              ClientDrawText("Failed to get help:(", 400, 569, true);
              errorf("ClientEventLoop: Failed to open readme, return %d\n", sv);
            }
          }
        }else {     //游戏界面
          if (event.button.x > 0 && event.button.x < RETURN_MAX_X && event.button.y > 0 && event.button.y < RETURN_MAX_Y) {  //返回主界面
            recordf("ClientEventLoop: Return mainUI\n");
            ClientGameQuit();
          }
        }
        break;
      default:break;
    }
  }
}

void ClientGameInit(void){
  GameCondition.GameDifficulty = EASY;
  LocalBoard = calloc(1, sizeof(Board));
  NetBoard = calloc(1, sizeof(Board));
  NetBoard->life = false;
  if (GameCondition.GameState == ONE_PLAYER){
    BoardCreate(LocalBoard, RED);
    ClientRender();
  }else{
    //创建传输线程，互斥锁和条件变量
    pthread_mutex_init(&GameInitMutex, NULL);
    pthread_cond_init(&GameInitCond, NULL);
    pthread_create(&TransmissionThread, NULL, ClientTransmissionThread, &ThreadArg);
    //等待服务器告知自己的编号，并创建本地挡板
    pthread_mutex_lock(&GameInitMutex);
    pthread_cond_wait(&GameInitCond, &GameInitMutex);
    BoardCreate(LocalBoard, GameCondition.LocalNum);
    pthread_mutex_unlock(&GameInitMutex);
    ClientRender();
    pthread_mutex_destroy(&GameInitMutex);
    pthread_cond_destroy(&GameInitCond);
    //等待另一个客户端上线
    if (!GameCondition.LocalNum) {
      pthread_mutex_init(&GameWaitMutex, NULL);
      pthread_cond_init(&GameWaitCond, NULL);
      pthread_mutex_lock(&GameWaitMutex);
      pthread_cond_wait(&GameWaitCond, &GameWaitMutex);
      pthread_mutex_unlock(&GameWaitMutex);
      pthread_mutex_destroy(&GameWaitMutex);
      pthread_cond_destroy(&GameWaitCond);
    }
    BoardCreate(NetBoard, GameCondition.NetNum);
    ClientRender();
  }
  ClientBGM();
}

void ClientGameQuit(void){
  if (GameCondition.GameState != ONE_PLAYER) {
    SocketSend(ServerSocket, "quit");
    pthread_cancel(TransmissionThread);
    BoardDestroy(NetBoard);
  }
  BoardDestroy(LocalBoard);
  GameCondition.GameState = MAIN;
  ClientRender();
  ClientBGM();
}

void ClientQuit(int code){
  Mix_HaltMusic();
  //free

  //纹理和表面
  SDL_DestroyTexture(MainTexture);
  SDL_FreeSurface(MainSurface);
  SDL_DestroyTexture(GameTexture);
  SDL_FreeSurface(GameSurface);
  //TTF
  TTF_CloseFont(Font);
  TTF_CloseFont(NumberFont);
  TTF_Quit();
  //Mix
  Mix_FreeMusic(bgm);
  Mix_Quit();
  //SDL
  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  //WSA
  WSACleanup();
  time_t cur_time = time(NULL);
  recordf("ClientQuit: Program quit with code %d at %s\n", code, ctime(&cur_time));
  exit(code);
}

void ClientConnect(SOCKET *server, SOCKADDR_IN* server_addr){
  *server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  memset(server_addr, 0, sizeof(SOCKADDR_IN));  //每个字节都用0填充
  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = inet_addr(ServerIP);
  server_addr->sin_port = htons(ServerPort);  //端口
  int cv, step = 0;
  while (true){
    step++;
    if (step >= CONNECT_STEP) {
      errorf("ClientConnect: connect Server failed, over step\n");
      ClientDrawText("Server Connect Failed", USER_TIP_X, USER_TIP_Y, true);
      SDL_Delay(CONNECT_DELAY);
      ClientQuit(CONNECT_FAILURE);
    }
    cv = connect(ServerSocket, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
    if (cv == SOCKET_ERROR) {
      errorf("ClientConnect: connect failed, return %d, code %d\n", cv, WSAGetLastError());
    } else break;
  }
  recordf("ClientConnect: connect success return %d code %d\n", cv, WSAGetLastError());
#ifdef DEBUG
  printf("ClientConnect: connect success return %d code %d\n", cv, WSAGetLastError());
#endif
}

void* ClientTransmissionThread(void* ThreadArgv){
  ClientConnect(&ServerSocket, &ServerAddr);    //连接服务器
  while (true) {    //传输的循环
    if (GameCondition.GameState == MAIN){
      pthread_exit(NULL);
      break;    //细节break，虽然没用，但是可以少一个Endless Loop的Warning
    }
    if (ARG == 0) {   //从服务器获得自己的编号
      pthread_mutex_lock(&GameInitMutex);
      SocketSend(ServerSocket, "ConnectRequest");
      SocketReceive(ServerSocket, Receive);
      GameCondition.LocalNum = (strrchr(Receive, '0') == NULL);
      GameCondition.NetNum = !GameCondition.LocalNum;
      pthread_cond_signal(&GameInitCond);
      pthread_mutex_unlock(&GameInitMutex);
      ARG++;
#ifdef DEBUG
      printf("LocalNum: %d\n", GameCondition.LocalNum);
#endif
    }else if (ARG == 1) {   //等待另一个客户端准备好
      pthread_mutex_lock(&GameWaitMutex);
      SocketSend(ServerSocket, "ClientReady");
      SocketReceive(ServerSocket, Receive);
      pthread_cond_signal(&GameWaitCond);
      pthread_mutex_unlock(&GameWaitMutex);
      ARG++;
    }else if (ARG == 2) {
      //TODO
      //ARG++;
    }
    if (!strcmp("4quit", Send)) ClientQuit(EXIT_SUCCESS);
    memset(Send, 0, BUF_SIZE);
    memset(Receive, 0, BUF_SIZE);
  }
}

void ClientRender(void){
  SDL_RenderClear(Renderer);
  if (GameCondition.GameState == MAIN) {
    MainTexture = SDL_CreateTextureFromSurface(Renderer, MainSurface);
    SDL_RenderCopy(Renderer, MainTexture, NULL, &MainRect);
    SDL_RenderPresent(Renderer);
    SDL_DestroyTexture(MainTexture);
  } else {
    GameTexture = SDL_CreateTextureFromSurface(Renderer, GameSurface);
    SDL_RenderCopy(Renderer, GameTexture, NULL, &GameRect);
    if (LocalBoard->life) {
      LocalBoard->tex = SDL_CreateTextureFromSurface(Renderer, LocalBoard->sur);
      SDL_RenderCopy(Renderer, LocalBoard->tex, &LocalBoard->SourceRect, &LocalBoard->DestRect);
      SDL_DestroyTexture(LocalBoard->tex);
    }
    if (NetBoard->life) {
      NetBoard->tex = SDL_CreateTextureFromSurface(Renderer, NetBoard->sur);
      SDL_RenderCopy(Renderer, NetBoard->tex, &NetBoard->SourceRect, &NetBoard->DestRect);
      SDL_DestroyTexture(NetBoard->tex);
    }
    SDL_RenderPresent(Renderer);
    SDL_DestroyTexture(GameTexture);
  }
}

void ClientBGM(void){
  Mix_HaltMusic();
  bgm = Mix_LoadMUS(BgmPath[GameCondition.GameState]);
  Mix_VolumeMusic(BgmVolume[GameCondition.GameState]);
  Mix_PlayMusic(bgm, -1);
}

void BoardCreate(Board* board, bool color){   //创建Board对象
  board->life = true;
  board->SourceRect.h = BOARD_INIT_H;
  board->DestRect.y = BOARD_INIT_Y;
  board->sur = IMG_Load(color == RED ? RED_BOARD_PATH : BLUE_BOARD_PATH);
  board->tex = SDL_CreateTextureFromSurface(Renderer, board->sur);
  board->DestRect.h = BOARD_INIT_H;
  if (GameCondition.GameDifficulty == EASY) {
    board->SourceRect.w = EASY_LEN;
    board->remain = EASY_REMAIN;
  }else if (GameCondition.GameDifficulty == NORMAL) {
    board->SourceRect.w = NORMAL_LEN;
    board->remain = NOR_REMAIN;
  }else if (GameCondition.GameDifficulty == HARD) {
    board->SourceRect.w = HARD_LEN;
    board->remain = HARD_REMAIN;
  }
  board->DestRect.w = board->SourceRect.w;
  if (GameCondition.GameState == ONE_PLAYER){
    board->DestRect.x = (WIN_WIDTH - board->SourceRect.w) / 2;
  }else if (GameCondition.GameState == TWO_PLAYER){
    board->DestRect.x = WIN_WIDTH * (color == RED ? 3 : 1) / 4 - board->SourceRect.w / 2;
  }
}

void BoardDestroy(Board* board){   //销毁Board对象
  memset(board, 0, sizeof(Board));
  free(board);
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
  printf("ReceiveMessage: %s\n", buf);
#endif
}

void SocketSend(SOCKET soc, const char* buf){
  //在buf前加上自身长度
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
    step++;
    if (step > SEND_STEP){
      errorf("SocketSend: send failed, over step\n");
      ClientQuit(SEND_FAILURE);
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
  }
  recordf("SocketSend: send success(len = %d)\n", len);
#ifdef DEBUG
  printf("SocketSend: send success(len = %d)\nSendMessage: %s\n", len, SendBuf);
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