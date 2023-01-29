#include "client.h"

int SDL_main(int argc, char* argv[]){
  //客户端初始化
  ClientCfgInit();
  if (argc == 3) ClientIPInit(argv[1], (u_short) strtol(argv[2], NULL, 10), LAN);
  else {
    errorf("Invalid argv\n");
    ClientIPInit(NULL, 0, LAN);
  }
  ClientLibInit();
  ClientLoadResource();
  ClientPlayBGM();
  ClientEventLoop();
  ClientQuit(EXIT_SUCCESS);
}

void ClientCfgInit(void){
  //读取设置文件
  cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    //读取设置失败 不影响游戏运行 只需要报告错误
    if (fscanf(cfg, "record=%d\nServerIP=%15s\nServerPort=%hd", &record, ServerIP, &ServerPort) < CFG_ITEM){
      fprintf(stderr, "ClientCfgInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ClientCfgInit: Fail to find cfg.txt\n");
  if (record){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    LogFile = fopen(LOG_PATH, "a+");
    if (LogFile == NULL) {   //日志文件不能正常写入 不记录信息
      record = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ClientCfgInit: Program start at %srecord = %d\n", ctime(&cur_time), record);
    }
  }
  memset(&GameCondition, 0, sizeof(GameCondition));
  state = MAIN;
}

void ClientLibInit(void){
  //客户端网络初始化
  int wv = WSAStartup(MAKEWORD(2, 2), &data);
  if (wv){
    errorf("ClientLibInit: WSAStartup Failed, code %d\n", wv);
    time_t cur_time = time(NULL);
    recordf("ClientLibInit: Program quit with code %d at %s\n", wv, ctime(&cur_time));
    exit(wv);
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
  if (Font == NULL) {   //字体打开失败
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

void ClientIPInit(char *IP, u_short port, bool flag){      //获取服务器的IP地址以及端口,flag控制局域网还是广域网
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
    if (step > IP_STEP){
      errorf("ClientIPInit: IP initialization failed, over step\n");
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
  GameSurface = IMG_Load(GAME_UI_PATH);
  GameRect = (SDL_Rect){0, 0, WIN_WIDTH, WIN_HEIGHT};
  recordf("LoadPicture: Complete!\n");
}

void ClientEventLoop(void){
  SDL_Event event;
  while (true){
    //小球移动
    if (state != MAIN && state != LOCAL_DEATH) BallMove(LocalBall);
    if (state == TWO_PLAYER || state == LOCAL_DEATH) BallMove(NetBall);
    //事件处理
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:                                                //关闭窗口
          recordf("ClientEventLoop: Quit by SDL_QUIT\n");
          if (state != MAIN) ClientGameQuit();
          return;
        case SDL_KEYDOWN: //按下键盘
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:                                         //Esc，退出
              if (state == MAIN) {
                recordf("ClientEventLoop: Quit by Esc\n");
                ClientQuit(EXIT_SUCCESS);
              } else {
                recordf("ClientEventLoop: Game Quit by Esc\n");
                ClientGameQuit();
              }
              break;
            case SDLK_UP: case SDLK_w:
            case SDLK_DOWN: case SDLK_s:
            case SDLK_LEFT: case SDLK_a:
            case SDLK_RIGHT: case SDLK_d:                             //移动挡板
              if (state != MAIN) {
                BoardMove(LocalBoard, LocalBall, event.key.keysym.sym);
              }
              break;
            case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE:    //小球出发
              LocalBall->SetOff = true;
              break;
            default: break;
          }
          break;
        case SDL_MOUSEBUTTONUP:
          recordf("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
          if (state == MAIN) {    //主界面
            if (event.button.x > SINGLE_MIN_X && event.button.x < SINGLE_MAX_X
                && event.button.y > SINGLE_MIN_Y && event.button.y < SINGLE_MAX_Y) {  //单人
              recordf("ClientEventLoop: Game start, one player\n");
              state = ONE_PLAYER;
              ClientGameInit();
            } else if (event.button.x > DOUBLE_MIN_X && event.button.x < DOUBLE_MAX_X //多人
                && event.button.y > DOUBLE_MIN_Y && event.button.y < DOUBLE_MAX_Y) {
              recordf("ClientEventLoop: Game start, two player\n");
              state = TWO_PLAYER;
              ClientGameInit();
            } else if (event.button.x > README_MIN_X && event.button.x < README_MAX_X
                && event.button.y > README_MIN_Y && event.button.y < README_MAX_Y) { //帮助
              recordf("ClientEventLoop: Press README button\n");
              int sv = system("start README.md");
              if (sv) {
                ClientRender();
                ClientDrawText("Failed to get help:(", USER_TIP_X, USER_TIP_Y, true);
                errorf("ClientEventLoop: Failed to open readme, return %d\n", sv);
              }
            }
          } else {     //游戏界面
            if (event.button.x > 0 && event.button.x < RETURN_MAX_X && event.button.y > 0
                && event.button.y < RETURN_MAX_Y) {  //返回主界面
              recordf("ClientEventLoop: Return mainUI\n");
              ClientGameQuit();
            }else if (!LocalBall->SetOff){
              //处理k和dir
              if (event.button.x != ObjectMidX(LocalBall)) {
                LocalBall->k = -(event.button.y - ObjectMidY(LocalBall)) / (event.button.x - ObjectMidX(LocalBall));
                LocalBall->dir = (LocalBall->k >= 0 ? RIGHT : LEFT);
              }else{
                LocalBall->k = BallInitialK;
                LocalBall->dir = VERTICAL;
              }
            }
          }
#ifdef DEBUG
          printf("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
#endif
          break;
        default:break;
      }
    }
    if (state != MAIN) ClientGameChange();
    ClientRender();
  }
}

void ClientGameInit(void){
  difficulty = EASY;
  //为弹板申请空间，并将NetBoard生命值设为0，使得单人游戏时不会渲染NetBoard，NetBall
  LocalBoard = calloc(1, sizeof(Board));
  NetBoard = calloc(1, sizeof(Board));
  NetBoard->life = 0;
  //为小球申请空间
  LocalBall = calloc(1, sizeof(Ball));
  NetBall = calloc(1, sizeof(Ball));
  //为砖块数组申请空间
  BrickArr = calloc(BrickNum[difficulty], sizeof(Brick));
  //判断单人游戏还是多人游戏，创建对象（创建线程），渲染gameUI
  if (state == ONE_PLAYER){
    BoardCreate(LocalBoard, RED);
    BallCreate(LocalBall, LocalBoard);
    BrickArrCreate(BrickArr);
    ClientRender();
  }else {
    //创建传输线程，互斥锁和条件变量
    pthread_mutex_init(&GameInitMutex, NULL);
    pthread_cond_init(&GameInitCond, NULL);
    pthread_create(&TransmissionThread, NULL, ClientTransmissionThread, &ThreadArg);
    //等待服务器告知自己的编号，并创建本地挡板
    pthread_mutex_lock(&GameInitMutex);
    pthread_cond_wait(&GameInitCond, &GameInitMutex);
    BoardCreate(LocalBoard, GameCondition.LocalNum);
    BallCreate(LocalBall, LocalBoard);
    pthread_mutex_unlock(&GameInitMutex);
    ClientRender();
    pthread_mutex_destroy(&GameInitMutex);
    pthread_cond_destroy(&GameInitCond);
    //第一个客户端等待第二个客户端上线
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
    BallCreate(NetBall, NetBoard);
    ClientRender();
    pthread_mutex_init(&BoardMoveMutex, NULL);
    pthread_cond_init(&BoardMoveCond, NULL);
  }
  ClientPlayBGM();
}

void ClientGameChange(void){    //一个难度的砖块结束后，改变游戏状态
  //检查条件是否满足
  if (LocalBoard->life) {   //生命值不为0
    for (int i = 0; i < BrickNum[difficulty]; i++) {
      if (BrickArr[i].life) return;
    }
  }else{    //生命值为0，游戏结束
    ClientDrawText("You Lose!", WL_X, WL_Y, true);
    SDL_Delay(WL_DELAY);
    ClientGameQuit();
    return;
  }
  //改变难度，判断游戏是否结束
  difficulty++;
  if (difficulty > HARD) {
    ClientDrawText("You Win!", WL_X, WL_Y, true);
    SDL_Delay(WL_DELAY);
    ClientGameQuit();
    return;
  }
  //摧毁对象
  BrickArrDestroy(BrickArr);
  BallDestroy(LocalBall);
  BoardDestroy(LocalBoard);
  BoardDestroy(NetBoard);
  if (state != ONE_PLAYER){
    BallDestroy(NetBall);
  }
  //重新为对象申请空间
  LocalBoard = calloc(1, sizeof(Board));
  NetBoard = calloc(1, sizeof(Board));
  NetBoard->life = 0;
  LocalBall = calloc(1, sizeof(Ball));
  BrickArr = calloc(BrickNum[difficulty], sizeof(Brick));
  //重新创建对象
  BoardCreate(LocalBoard, RED);
  BallCreate(LocalBall, LocalBoard);
  if (state == ONE_PLAYER){
    BrickArrCreate(BrickArr);
  }else {
    NetBall = calloc(1, sizeof(Ball));
    //TODO
  }
  LocalBall->SetOff = false;
}

void ClientGameQuit(void){
  if (state != ONE_PLAYER) {
    pthread_cancel(TransmissionThread);
    pthread_join(TransmissionThread, NULL);
    SocketSend(ServerSocket, "quit");
    BallDestroy(NetBall);
    BoardDestroy(NetBoard);
    pthread_mutex_destroy(&BoardMoveMutex);
    pthread_cond_destroy(&BoardMoveCond);
  }
  BrickArrDestroy(BrickArr);
  BrickPre = false;
  BallDestroy(LocalBall);
  BoardDestroy(LocalBoard);
  state = MAIN;
  ClientRender();
  ClientPlayBGM();
}

void ClientQuit(int code){
  Mix_HaltMusic();
  //纹理和表面
  SDL_DestroyTexture(MainTexture);
  SDL_FreeSurface(MainSurface);
  SDL_DestroyTexture(GameTexture);
  SDL_FreeSurface(GameSurface);
  //TTF
  TTF_CloseFont(Font);
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
    if (state == MAIN){
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
      //SocketReceive(ServerSocket, Receive);
      recv(ServerSocket, Receive, BUF_SIZE, 0);
#ifdef DEBUG
      puts(Receive);
#endif
      //根据服务端的数据来对砖块初始化
      char *brk_ptr = strchr(Receive, ':') + 1;
      for (int i = 0; i < BrickNum[difficulty]; i++) {
        int x = strtol(brk_ptr, &brk_ptr, 10);
        int y = strtol(brk_ptr, &brk_ptr, 10);
        Element color = strtol(brk_ptr, &brk_ptr, 10);
        BrickCreate(&BrickArr[i], x, y, color);
      }
#ifdef DEBUG
      puts(Receive);
#endif
      BrickPre = true;
      pthread_cond_signal(&GameWaitCond);
      pthread_mutex_unlock(&GameWaitMutex);
      ARG++;
    }else if (ARG == 2) {
      pthread_mutex_lock(&BoardMoveMutex);
      pthread_cond_wait(&BoardMoveCond, &BoardMoveMutex);
      ClientDataResolve(Send, HOST_TO_NET);
      SocketSend(ServerSocket, Send);
      SocketReceive(ServerSocket, Receive);
      ClientDataResolve(Receive, NET_TO_HOST);
      pthread_mutex_unlock(&BoardMoveMutex);
    }
    if (!strcmp("quit", Send)) ClientQuit(EXIT_SUCCESS);
    memset(Send, 0, BUF_SIZE);
    memset(Receive, 0, BUF_SIZE);
  }
}

void ClientDataResolve(char* buf, int flag){    //客户端数据解析
  if (flag == HOST_TO_NET){
    sprintf(buf, "Client%d,LocalBall(%f,%f),LocalBoard(%d,%d)", GameCondition.LocalNum,
            GameCondition.LocalBallX, GameCondition.LocalBallY, GameCondition.LocalBoardX, GameCondition.LocalBoardY);
  }else if (flag == NET_TO_HOST){
    sscanf(buf, "Client%*d,NetBall(%f,%f),NetBoard(%d,%d)",
            &GameCondition.NetBallX, &GameCondition.NetBallY, &GameCondition.NetBoardX, &GameCondition.NetBoardY);
    NetBoard->DestRect.x = GameCondition.NetBoardX;
    NetBoard->DestRect.y = GameCondition.NetBoardY;
    NetBall->DestRect.x = GameCondition.NetBallX;
    NetBall->DestRect.y = GameCondition.NetBallY;
  }
}

void ClientRender(void){    //客户端UI渲染
  //清空渲染器
  SDL_RenderClear(Renderer);
  //根据游戏状态来区分渲染内容
  if (state == MAIN) {  //渲染mainUI
    MainTexture = SDL_CreateTextureFromSurface(Renderer, MainSurface);
    SDL_RenderCopy(Renderer, MainTexture, NULL, &MainRect);
    SDL_RenderPresent(Renderer);
    SDL_DestroyTexture(MainTexture);
  } else {    //渲染gameUI
    GameTexture = SDL_CreateTextureFromSurface(Renderer, GameSurface);
    SDL_RenderCopy(Renderer, GameTexture, NULL, &GameRect);
    if (BrickPre) {   //如果砖块已经初始化，就渲染砖块
      for (int i = 0; i < BrickNum[difficulty]; i++) {
        if (BrickArr[i].life) {
          BrickArr[i].tex = SDL_CreateTextureFromSurface(Renderer, BrickArr[i].sur);
          SDL_SetTextureBlendMode(BrickArr[i].tex, SDL_BLENDMODE_BLEND);
          SDL_SetTextureAlphaMod(BrickArr[i].tex, BrickArr[i].alpha);
          SDL_RenderCopy(Renderer, BrickArr[i].tex, NULL, &BrickArr[i].DestRect);
          SDL_DestroyTexture(BrickArr[i].tex);
        }
      }
    }
    if (LocalBoard->life) {
      LocalBoard->tex = SDL_CreateTextureFromSurface(Renderer, LocalBoard->sur);
      SDL_SetTextureBlendMode(LocalBoard->tex, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(LocalBoard->tex, LocalBoard->alpha);
      SDL_RenderCopy(Renderer, LocalBoard->tex, &LocalBoard->SourceRect, &LocalBoard->DestRect);
      SDL_DestroyTexture(LocalBoard->tex);
      LocalBall->tex = SDL_CreateTextureFromSurface(Renderer, LocalBall->sur);
      SDL_RenderCopyF(Renderer, LocalBall->tex, NULL, &LocalBall->DestRect);
      SDL_DestroyTexture(LocalBall->tex);
    }
    if (state != ONE_PLAYER && state != NET_DEATH && NetBoard->life) {
      NetBoard->tex = SDL_CreateTextureFromSurface(Renderer, NetBoard->sur);
      SDL_SetTextureBlendMode(NetBoard->tex, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(NetBoard->tex, NetBoard->alpha);
      SDL_RenderCopy(Renderer, NetBoard->tex, &NetBoard->SourceRect, &NetBoard->DestRect);
      SDL_DestroyTexture(NetBoard->tex);
      NetBall->tex = SDL_CreateTextureFromSurface(Renderer, NetBall->sur);
      SDL_RenderCopyF(Renderer, NetBall->tex, NULL, &NetBall->DestRect);
      SDL_DestroyTexture(NetBall->tex);
    }
    char ScoreText[4] = {0};
    sprintf(ScoreText, "%d", LocalBall->score);
    ClientDrawText(ScoreText, SCORE_X, SCORE_Y, false);
    SDL_RenderPresent(Renderer);
    SDL_DestroyTexture(GameTexture);
  }
}

void ClientPlayBGM(void){
  Mix_HaltMusic();
  bgm = Mix_LoadMUS(BgmPathVec[state]);
  Mix_VolumeMusic(BgmVolumeVec[state]);
  Mix_PlayMusic(bgm, -1);
}

void ClientDrawText(const char *text, int x, int y, bool pre){   //根据参数渲染文本
  FontSurface = TTF_RenderUTF8_Blended(Font, text, FontColor);
  FontTexture = SDL_CreateTextureFromSurface(Renderer, FontSurface);
  FontRect = (SDL_Rect){x, y, FontSurface->w, FontSurface->h};
  SDL_RenderCopy(Renderer, FontTexture, NULL, &FontRect);
  if (pre) SDL_RenderPresent(Renderer);
  //末处理
  SDL_FreeSurface(FontSurface);
  SDL_DestroyTexture(FontTexture);
}

void BoardCreate(Board* const board, const bool color){   //创建Board对象
  board->life = BoardLifeVec[difficulty];
  board->alpha = UINT8_MAX;
  board->sur = IMG_Load(color == RED ? RED_BOARD_PATH : BLUE_BOARD_PATH);
  board->tex = SDL_CreateTextureFromSurface(Renderer, board->sur);
  board->SourceRect.w = BoardLenVec[difficulty];
  board->SourceRect.h = board->sur->h;
  board->DestRect.y = BOARD_INIT_Y;
  board->DestRect.w = board->SourceRect.w;
  board->DestRect.h = board->sur->h;
  if (state == ONE_PLAYER){
    board->DestRect.x = (WIN_WIDTH - board->SourceRect.w) / 2;
  }else if (state == TWO_PLAYER){
    board->DestRect.x = WIN_WIDTH * (color == RED ? 3 : 1) / 4 - board->SourceRect.w / 2;
  }
}

void BoardMove(Board* board, Ball* const ball, SDL_KeyCode const operation){   //弹板移动
  if (state != MAIN && state != LOCAL_DEATH){
    if (state != ONE_PLAYER){
      pthread_mutex_lock(&BoardMoveMutex);
    }
    switch (operation) {
      case SDLK_w: case SDLK_UP:    //上移
        if (board->DestRect.y > BOARD_MIN_Y) board->DestRect.y--;
        break;
      case SDLK_s: case SDLK_DOWN:  //下移
        if (board->DestRect.y + board->DestRect.h < WIN_HEIGHT) board->DestRect.y++;
        break;
      case SDLK_a: case SDLK_LEFT:  //左移
        if (board->DestRect.x >= BoardMoveSpeedVec[difficulty]){
          board->DestRect.x -= BoardMoveSpeedVec[difficulty];
        }else {
          board->DestRect.x = 0;
        }
        break;
      case SDLK_d: case SDLK_RIGHT: //右移
        if (board->DestRect.x + board->DestRect.w + BoardMoveSpeedVec[difficulty] < WIN_WIDTH){
          board->DestRect.x += BoardMoveSpeedVec[difficulty];
        }else if (board->DestRect.x + board->DestRect.w < WIN_WIDTH){
          board->DestRect.x = WIN_WIDTH - board->DestRect.w;
        }
        break;
      default:break;
    }
    if (!ball->SetOff) {
      ball->DestRect.x = (float)board->DestRect.x + (float)(board->DestRect.w - ball->sur->w) / 2;
      ball->DestRect.y = (float)board->DestRect.y - (float)ball->sur->h;
    }
    if (state != ONE_PLAYER){
      pthread_cond_signal(&BoardMoveCond);
      pthread_mutex_unlock(&BoardMoveMutex);
    }
    GameCondition.LocalBoardX = board->DestRect.x;
    GameCondition.LocalBoardY = board->DestRect.y;
    GameCondition.LocalBallX = ball->DestRect.x;
    GameCondition.LocalBallY = ball->DestRect.y;
  }
}

void BoardDestroy(Board* const board){   //销毁Board对象
  memset(board, 0, sizeof(Board));
  free(board);
}

void BallCreate(Ball* ball, Board* const board){   //创建Ball对象
  ball->score = 0;
  ball->SetOff = false;
  ball->dir = VERTICAL;
  ball->k = BallInitialK;
  ball->board = board;
  ball->element = EMPTY;
  ball->sur = IMG_Load(BallPathVec[EMPTY]);
  ball->tex = SDL_CreateTextureFromSurface(Renderer, ball->sur);
  ball->DestRect.x = (float)board->DestRect.x + (float)(board->DestRect.w - ball->sur->w) / 2;
  ball->DestRect.y = (float)board->DestRect.y - (float)ball->sur->h;
  ball->DestRect.w = (float)ball->sur->w;
  ball->DestRect.h = (float)ball->sur->h;
}

void BallMove(Ball* const ball){            //小球移动
  //检查是否满足移动条件
  if (!ball->board->life || !ball->SetOff) {              //如果生命值为0或者没有出发，就不能移动
    return;
  }
  //与砖块碰撞
  bool flag = true;
  for (int i = 0; i < BrickNum[difficulty]; i++){
    if (BrickArr[i].life) {
      if (ObjectMidY(ball) >= ObjectMinY(&BrickArr[i]) - 1 &&
          ObjectMidY(ball) <= ObjectMinY(&BrickArr[i]) + 1 &&
          ObjectMidX(ball) >= ObjectMinX(&BrickArr[i]) &&
          ObjectMidX(ball) <= ObjectMaxX(&BrickArr[i])) {         //上方
        BallHit(ball, &BrickArr[i], "bu");
        flag = false;
        break;
      }else if (ObjectMidY(ball) >= ObjectMaxY(&BrickArr[i]) - 1 &&
          ObjectMidY(ball) <= ObjectMaxY(&BrickArr[i]) + 1 &&
          ObjectMidX(ball) >= ObjectMinX(&BrickArr[i]) &&
          ObjectMidX(ball) <= ObjectMaxX(&BrickArr[i])){        //下方
        BallHit(ball, &BrickArr[i], "bd");
        flag = false;
        break;
      }else if (ObjectMidX(ball) >= ObjectMinX(&BrickArr[i]) - 1 &&
          ObjectMidX(ball) <= ObjectMinX(&BrickArr[i]) + 1 &&
          ObjectMidY(ball) >= ObjectMinY(&BrickArr[i]) &&
          ObjectMidY(ball) <= ObjectMaxY(&BrickArr[i])) {        //左侧
        BallHit(ball, &BrickArr[i], "bl");
        flag = false;
        break;
      } else if (ObjectMidX(ball) >= ObjectMaxX(&BrickArr[i]) - 1 &&
          ObjectMidX(ball) <= ObjectMaxX(&BrickArr[i]) + 1 &&
          ObjectMidY(ball) >= ObjectMinY(&BrickArr[i]) &&
          ObjectMidY(ball) <= ObjectMaxY(&BrickArr[i])) {        //右侧
        BallHit(ball, &BrickArr[i], "br");
        flag = false;
        break;
      }
    }
  }
  //碰撞检查
  if (flag) {
    if (ObjectMaxY(ball) >= WIN_HEIGHT) {                      //如果小球碰到屏幕底端，生命值-1，并重置小球和弹板的位置
      BallHit(ball, NULL, "sd");
    } else if (ObjectMinX(ball) <= 0) {                         //如果小球碰到屏幕左端，反弹
      BallHit(ball, NULL, "sl");
    } else if (ObjectMaxX(ball) >= WIN_WIDTH) {                 //如果小球碰到屏幕右端，反弹
      BallHit(ball, NULL, "sr");
    } else if (ObjectMinY(ball) <= 0 ||
    (ObjectMaxY(ball) >= (float)ball->board->DestRect.y &&
    ObjectMinX(ball) >= (float)ObjectMinX(ball->board) &&
    ObjectMaxX(ball) <= ObjectMaxX((ball->board)))) {
      BallHit(ball, NULL, "su");                    //如果小球碰到屏幕顶端或自己的挡板顶端，反弹
    }
  }
  //根据方向和斜率移动
  /*特殊情况*/
  if (ball->dir == VERTICAL) {                            //方向为Vertical时，单独处理
    ball->DestRect.y += (float)((ball->k > 0 ? -1 : 1) * BallMoveSpeedVec[difficulty]);
    return;
  }
  /*一般情况*/
  double dx, dy;
  dx = sqrt(BallMoveSpeedVec[difficulty] / (pow(ball->k, 2) + 1));
  dy = dx * ball->k;
  ball->DestRect.x += (float)(ball->dir * dx);
  ball->DestRect.y += (float)(ball->dir * dy);
}

void BallHit(Ball* ball, Brick* brick, const char* mode){   //小球与对象的碰撞
  if (!strcmp(mode, "bu") || !strcmp(mode, "bd")){    //砖块上方或下方
    ball->k = -ball->k;
    ball->score++;
    brick->life--;
    brick->alpha = (uint8_t)((double)(brick->life) / BrickLifeVec[difficulty] * UINT8_MAX);\
  }else if (!strcmp(mode, "bl")){                     //砖块左侧
    ball->dir = LEFT;
    ball->score++;
    ball->k = -ball->k;
    brick->life--;
    brick->alpha = (uint8_t)((double)(brick->life) / BrickLifeVec[difficulty] * UINT8_MAX);
  }else if (!strcmp(mode, "br")){                       //砖块右侧
    ball->dir = RIGHT;
    ball->score++;
    ball->k = -ball->k;
    brick->life--;
    brick->alpha = (uint8_t)((double)(brick->life) / BrickLifeVec[difficulty] * UINT8_MAX);
  }else if (!strcmp(mode, "su")){                       //屏幕上方
    ball->k = -ball->k;
  }else if (!strcmp(mode, "sd")){                       //屏幕下方
    //先修改挡板的属性
    if (state != ONE_PLAYER) {
      pthread_mutex_lock(&BoardMoveMutex);
    }
    ball->board->life--;
    ball->board->alpha = (uint8_t)((double)(ball->board->life) / BoardLifeVec[difficulty] * UINT8_MAX);
    ball->board->DestRect.x = state == ONE_PLAYER ?
                              (WIN_WIDTH - ball->board->SourceRect.w) / 2 :
                              WIN_WIDTH * (GameCondition.LocalNum == RED ? 1 : 3) / 4 - ball->board->SourceRect.w / 2;
    ball->board->DestRect.y = BOARD_INIT_Y;
    if (state != ONE_PLAYER) {
      pthread_mutex_unlock(&BoardMoveMutex);
    }
    //再修改小球的属性
    ball->SetOff = false;
    ball->dir = VERTICAL;
    ball->k = BallInitialK;
    ball->DestRect.x = (float)ball->board->DestRect.x + (float)(ball->board->DestRect.w - ball->sur->w) / 2;
    ball->DestRect.y = (float)ball->board->DestRect.y - (float)ball->sur->h;
  }else if (!strcmp(mode, "sl")){                       //屏幕左侧
    ball->dir = RIGHT;
    ball->k = -ball->k;
  }else if (!strcmp(mode, "sr")){                       //屏幕右侧
    ball->dir = LEFT;
    ball->k = -ball->k;
  }else{
    errorf("BallHit: Incompatible Mode\n");
  }
}

void BallDestroy(Ball* const ball){   //销毁Ball对象
  memset(ball, 0, sizeof(Ball));
  free(ball);
}

void BrickCreate(Brick* const brick, const int x, const int y, const Element color){
  brick->life = BrickLifeVec[difficulty];
  brick->element = color;
  brick->alpha = UINT8_MAX;
  brick->sur = IMG_Load(BrickPathVec[color]);
  printf("\n");
  puts(SDL_GetError());
  brick->tex = SDL_CreateTextureFromSurface(Renderer, brick->sur);
  brick->DestRect.x = x;
  brick->DestRect.y = y;
  brick->DestRect.w = brick->sur->w;
  brick->DestRect.h = brick->sur->h;
}

void BrickArrCreate(Brick* const arr){
  bool choice[16][12] = {0};
  for (int i = 0; i < BrickNum[difficulty]; i++) {
    int x = rand() % 15, y = rand() % 12, c = rand() % 5;
    while (choice[x][y]) {
      x = rand() % 15;
      y = rand() % 12;
    }
    choice[x][y] = true;
    BrickCreate(&arr[i], (x + 2) * 64, (y + 1) * 36, c);
  }
  BrickPre = true;
}

void BrickArrDestroy(Brick* const arr){
  for (int i = 0; i < BrickNum[difficulty]; i++){
    BrickDestroy(&arr[i]);
  }
  free(arr);
}

void BrickDestroy(Brick* const brick){
  memset(brick, 0, sizeof(Brick));
}

void SocketReceive(SOCKET soc, char* buf){
  char temp[BUF_SIZE] = {0};
  int r, len = -1, len_temp, rf = 0, step = 0;
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