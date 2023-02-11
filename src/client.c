#include "client.h"

__attribute__((unused))
int SDL_main(int argc, char* argv[]){
  //客户端初始化
  ClientCfgInit();
  if (argc == 3) {
    ClientIPInit(argv[1], (u_short) strtol(argv[2], NULL, 10), LAN);
  }
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
  FILE* cfg = fopen(CFG_PATH, "r");
  if (cfg != NULL) {
    //读取设置失败 不影响游戏运行 只需要报告错误
    if (fscanf(cfg, "RecordFlag=%d\nmod=%d\nServerIP=%15s\nServerPort=%hd",
                &RecordFlag, &mod, ServerIP, &ServerPort) < CFG_ITEM){
      fprintf(stderr, "ClientCfgInit: Error occurred when loading configs, ");
      if (feof(cfg)) fprintf(stderr, "EOF\n");
      else if (ferror(cfg))  fprintf(stderr, "Read Error\n");
      else fprintf(stderr, "Match Error\n");
    }
    fclose(cfg);
  }else errorf("ClientCfgInit: Fail to find cfg.txt\n");
  if (RecordFlag){    //根据设置文件 以及日志文件是否能正常写入 来决定是否记录信息
    LogFilePtr = fopen(LOG_PATH, "a+");
    if (LogFilePtr == NULL) {   //日志文件不能正常写入 不记录信息
      RecordFlag = 0;
      errorf("Failed to open cfg/slog.txt\n");
    }else{
      time_t cur_time = time(NULL);
      recordf("ClientCfgInit: Program start at %sRecordFlag = %d\tmod = %d\n",
              ctime(&cur_time), RecordFlag, mod);
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
  Window = SDL_CreateWindow(WIN_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
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
    debugf_b("Valid Initial Network Config, IP = %s, Port = %hd\n", ServerIP, ServerPort);
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
  while (true) {
    if (state != MAIN && state != ONE_PLAYER) pthread_mutex_lock(&NetQuitMutex);
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
          debugf_b("ClientEventLoop: Mouse button up (%d, %d)\n", event.button.x, event.button.y);
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
          break;
        default:break;
      }
    }
    if (state != MAIN) ClientGameChange();
    ClientRender();
    if (state != MAIN && state != ONE_PLAYER) pthread_mutex_unlock(&NetQuitMutex);
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
    pthread_mutex_init(&GameChangeMutex, NULL);
    pthread_cond_init(&GameChangeCond, NULL);
    pthread_mutex_init(&NetQuitMutex, NULL);
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
    if (state == ONE_PLAYER) {        //单人模式直接返回mainUI
      SDL_Delay(WL_DELAY);
      ClientGameQuit();
    }else if (state == LOCAL_DEATH){  //多人模式

    }else if (state == NET_DEATH){    //都

    }else{
      state = LOCAL_DEATH;
    }
    return;
  }
  //锁定GameChangeMutex
  if (state != MAIN && state != ONE_PLAYER) {
    pthread_mutex_lock(&GameChangeMutex);
    GameChangeFlag = true;
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
  //解锁GameChangeMutex
  if (state != MAIN && state != ONE_PLAYER) {
    GameChangeFlag = false;
    pthread_mutex_unlock(&GameChangeMutex);
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
    pthread_mutex_destroy(&GameChangeMutex);
    pthread_cond_destroy(&GameChangeCond);
    pthread_mutex_destroy(&NetQuitMutex);
  }
  BrickArrDestroy(BrickArr);
  BrickPre = false;
  BallDestroy(LocalBall);
  BoardDestroy(LocalBoard);
  memset(&GameCondition, 0, sizeof(GameCondition));
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
  memset(server_addr, 0, sizeof(SOCKADDR_IN));
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
  debugf_b("ClientConnect: connect success return %d code %d\n", cv, WSAGetLastError());
}

void* ClientTransmissionThread(void* ThreadArgv){
  ClientConnect(&ServerSocket, &ServerAddr);    //连接服务器
  while (true) {    //传输的循环
    if (state == MAIN){
      pthread_exit(NULL);
      break;        //细节break，虽然没用，但是可以少一个Endless Loop的Warning
    }
    switch (ARG) {
      case 0:   //从服务器获得自己的编号
        pthread_mutex_lock(&GameInitMutex);
        SocketSend(ServerSocket, "ConnectRequest");
        SocketReceive(ServerSocket, Receive);
        GameCondition.LocalNum = !strcmp(Receive, "1");
        GameCondition.NetNum = !GameCondition.LocalNum;
        pthread_cond_signal(&GameInitCond);
        pthread_mutex_unlock(&GameInitMutex);
        ARG++;
        debugf_b("LocalNum: %d\n", GameCondition.LocalNum);
        break;
      case 1:   //根据服务端的数据对砖块初始化
        sprintf(Send, "BrickOrder%d", difficulty);
        SocketSend(ServerSocket, Send);
        SocketReceive(ServerSocket, Receive);
        char *brk_ptr = Receive;
        for (int i = 0; i < BrickNum[difficulty]; i++) {
          int x = strtol(brk_ptr, &brk_ptr, 10);
          int y = strtol(brk_ptr, &brk_ptr, 10);
          Element color = strtol(brk_ptr, &brk_ptr, 10);
          BrickCreate(&BrickArr[i], x, y, color);
        }
        BrickPre = true;
        ARG++;
        break;
      case 2:   //等待另一个客户端准备好
        pthread_mutex_lock(&GameWaitMutex);
        SocketSend(ServerSocket, "ClientReady");
        SocketReceive(ServerSocket, Receive);
        pthread_cond_signal(&GameWaitCond);
        pthread_mutex_unlock(&GameWaitMutex);
        ARG++;
        break;
      case 3:   //游戏中传输数据
        pthread_mutex_lock(&BoardMoveMutex);
        pthread_cond_wait(&BoardMoveCond, &BoardMoveMutex);
        ClientDataResolve(Send, CLIENT_TO_SERVER);
        SocketSend(ServerSocket, Send);
        SocketReceive(ServerSocket, Receive);
        //对方异常退出
        if (!strcmp(Receive, "quit")){
          pthread_mutex_lock(&NetQuitMutex);
          ClientDrawText("Player Disconnect", USER_TIP_X, USER_TIP_Y, true);
          BallDestroy(NetBall);
          pthread_mutex_destroy(&BoardMoveMutex);
          pthread_cond_destroy(&BoardMoveCond);
          BrickArrDestroy(BrickArr);
          BrickPre = false;
          BallDestroy(LocalBall);
          BoardDestroy(LocalBoard);
          ThreadArg = 0;
          state = MAIN;
          SDL_Delay(OFFLINE_DELAY);
          ClientRender();
          ClientPlayBGM();
          pthread_mutex_unlock(&NetQuitMutex);
          pthread_mutex_destroy(&NetQuitMutex);
          pthread_exit(NULL);
        }
        ClientDataResolve(Receive, SERVER_TO_CLIENT);
        pthread_mutex_unlock(&BoardMoveMutex);
        break;
      default:break;
    }
    memset(Send, 0, BUF_SIZE);
    memset(Receive, 0, BUF_SIZE);
  }
}

void ClientDataResolve(char* buf, bool flag){    //客户端数据解析
  //锁定GameChangeMutex
  pthread_mutex_lock(&GameChangeMutex);
  //数据解析
  if (flag == CLIENT_TO_SERVER){        //客户端数据转换为服务端数据
    //改变游戏难度
    if (GameChangeFlag){
      sprintf(buf, "BrickOrder%d", difficulty);
      return;
    }
    //正常发送数据
    sprintf(buf, "%d %d %d %d %f %f ", GameCondition.LocalNum, LocalBoard->life,
            GameCondition.LocalBoardX, GameCondition.LocalBoardY, GameCondition.LocalBallX, GameCondition.LocalBallY);
    for (int i = 0; i < BrickNum[difficulty]; i++){
      char temp[3] = {(char)(BrickArr[i].life + 48), ' '};
      strcat(buf, temp);
    }
  }else if (flag == SERVER_TO_CLIENT){  //服务端数据转换为客户端数据
    char *ptr = buf;
    if (strtol(ptr, &ptr, 10) == GameCondition.NetNum) {
      NetBoard->life = strtol(ptr, &ptr, 10);
      NetBoard->DestRect.x = strtol(ptr, &ptr, 10);
      NetBoard->DestRect.y = strtol(ptr, &ptr, 10);
      NetBall->DestRect.x = strtof(ptr, &ptr);
      NetBall->DestRect.y = strtof(ptr, &ptr);
      for (int i = 0; i < BrickNum[difficulty]; i++){
        BrickArr[i].life = strtol(ptr, &ptr, 10);
      }
    }else{
      errorf("Wrong NetNum\n");
      //TODO
    }
  }
  //解锁GameChangeMutex
  GameChangeFlag = false;
  pthread_mutex_unlock(&GameChangeMutex);
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
  } else {              //渲染gameUI
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
  //SDL_FreeSurface(FontSurface);
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
  ball->hit = 0;
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
  for (int i = 0; i < BrickNum[difficulty]; i++) {
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
      }else if (ObjectMidX(ball) >= ObjectMaxX(&BrickArr[i]) - 1 &&
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
    ElementReact(ball, brick);
    ball->hit++;
    ball->k = -ball->k;
    ball->score++;
    ball->element = brick->element;
    ball->sur = IMG_Load(BallPathVec[ball->element]);
    brick->life--;
    brick->alpha = (uint8_t)((double)(brick->life) / BrickLifeVec[difficulty] * UINT8_MAX);
  }else if (!strcmp(mode, "bl")){                     //砖块左侧
    ElementReact(ball, brick);
    ball->hit++;
    ball->dir = LEFT;
    ball->score++;
    ball->k = -ball->k;
    brick->life--;
    brick->alpha = (uint8_t)((double)(brick->life) / BrickLifeVec[difficulty] * UINT8_MAX);
  }else if (!strcmp(mode, "br")){                       //砖块右侧
    ElementReact(ball, brick);
    ball->hit++;
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
    ball->hit = 0;
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
  //brick->tex = SDL_CreateTextureFromSurface(Renderer, brick->sur);
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

void ElementReact(Ball* ball, Brick* brick) {
  //检查反应条件
  if (!mod && brick->life == 0 || ball->board->life == 0) return;
  debugf("ball->element = %d", ball->element);
  //进行反应
  const Element BallElement = ball->element;
  const Element BrickElement = brick->element;
  debugf("BallElement = %d\tBrickElement = %d", BallElement, BrickElement);
  if ((BallElement == FIRE && BrickElement == THUNDER) || (BallElement == THUNDER && BrickElement == FIRE)) {
    /*火元素和雷元素相遇会爆炸，对半径一定范围内的砖块全部造成一次攻击*/
    ball->element = EMPTY;
    ball->score += (brick->life + REACT_BONUS);
    brick->life = 0;
    for (int i = 0; i < BrickNum[difficulty]; i++) {
      if (BrickArr[i].life &&
      abs(ObjectMinX(&BrickArr[i]) - ObjectMinX(brick)) <= brick->DestRect.w &&
      abs(ObjectMinY(&BrickArr[i]) - ObjectMinY(brick)) <= brick->DestRect.h){
        ball->score += (BrickArr[i].life + REACT_BONUS);
        BrickArr[i].life--;
      }
    }
  }else if ((BallElement == WATER || BallElement == ICE) && BrickElement == THUNDER) {
    /*水元素小球或冰元素小球撞击雷元素砖块会引发链式反应，导致相邻的水元素砖块全部受到一次攻击*/
    for (int i = 0; i < BrickNum[difficulty]; i++) {
      if (BrickArr[i].life && (BrickArr[i].element == WATER || BrickArr[i].element == ICE) &&
          abs(ObjectMinX(&BrickArr[i]) - ObjectMinX(brick)) <= brick->DestRect.w &&
          abs(ObjectMinY(&BrickArr[i]) - ObjectMinY(brick)) <= brick->DestRect.h){
        ball->score += (BrickArr[i].life + REACT_BONUS);
        BrickArr[i].life--;
      }
    }
  }else if (BallElement == EMPTY && BrickElement != EMPTY) {
    /*无元素小球撞击相应元素砖块会附着上相应元素*/
    ball->element = brick->element;
    ball->sur = IMG_Load(BallPathVec[ball->element]);
  }
}

void SocketReceive(SOCKET soc, char* buf){    //从soc接受buf
  recv(soc, buf, BUF_SIZE, 0);
  recordf("SocketReceive: soc = %llu\nReceiveMessage(%llu):\n%s\n", soc, strlen(buf), buf);
  debugf_b("ReceiveMessage(%llu):\n%s\n", strlen(buf), buf);
}

void SocketSend(SOCKET soc, const char* buf){   //发送buf给soc
  send(soc, buf, (int)(strlen(buf)) + 1, 0);
  recordf("SocketSend: soc = %llu\nSendMessage(%llu):\n%s\n", soc, strlen(buf), buf);
  debugf_b("SendMessage(%llu):\n%s\n", strlen(buf), buf);
}