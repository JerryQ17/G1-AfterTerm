#ifndef CLIENT_H
#define CLIENT_H

//包含库

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <winsock2.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

//宏定义

#define DEBUG

#define TITLE       "冬津羽戏客户端"   //窗口标题
#define WIN_WIDTH   1280            //窗口宽度
#define WIN_HEIGHT  720             //窗口高度
#define FONT_SIZE   60              //字体大小
#define CFG_ITEM    3               //配置数量

#define SINGLE_MIN_X  255            //mainUI的单人按钮的x最小值
#define SINGLE_MAX_X  512            //mainUI的单人按钮的x最大值
#define SINGLE_MIN_Y  384            //mainUI的单人按钮的y最小值
#define SINGLE_MAX_Y  513            //mainUI的单人按钮的y最大值
#define DOUBLE_MIN_X  SINGLE_MAX_X   //mainUI的多人按钮的x最小值
#define DOUBLE_MAX_X  770            //mainUI的多人按钮的x最大值
#define DOUBLE_MIN_Y  SINGLE_MIN_Y   //mainUI的多人按钮的y最小值
#define DOUBLE_MAX_Y  SINGLE_MAX_Y   //mainUI的多人按钮的y最大值
#define README_MIN_X  DOUBLE_MAX_X   //mainUI的帮助按钮的x最小值
#define README_MAX_X  1025           //mainUI的单人按钮的x最大值
#define README_MIN_Y  SINGLE_MIN_Y   //mainUI的帮助按钮的y最小值
#define README_MAX_Y  SINGLE_MAX_Y   //mainUI的帮助按钮的y最大值
#define RETURN_MAX_X  144
#define RETURN_MAX_Y  72
#define USER_TIP_X    400
#define USER_TIP_Y    600
#define SCORE_X       1190
#define SCORE_Y       10
#define WL_X          525
#define WL_Y          330
#define WL_DELAY      2000
#define BOARD_MIN_Y   600
#define BOARD_INIT_Y  650

#define IP_FAILURE              1
#define SOCKET_FAILURE          2
#define CONNECT_FAILURE         3
#define RECEIVE_FAILURE         4
#define SEND_FAILURE            5
#define SDL_INIT_ERROR          6
#define WINDOW_CREATE_ERROR     7
#define RENDERER_CREATE_ERROR   8
#define TTF_INIT_ERROR          9
#define TTF_OPEN_FONT_ERROR     10
#define MIX_OPEN_AUDIO_ERROR    11

#define CFG_PATH            "cfg/cfg.txt"
#define LOG_PATH            "cfg/clog.txt"
#define FONT_PATH           "font/calibri.ttf"
#define MAIN_UI_PATH        "img/mainUI.png"
#define GAME_UI_PATH        "img/gameUI.png"
#define RED_BOARD_PATH      "img/RedBoard.png"
#define BLUE_BOARD_PATH     "img/BlueBoard.png"

#define MES_MAX_LEN   4
#define BUF_SIZE      1000
#define IP_STEP       5
#define CONNECT_STEP  20
#define SEND_STEP     20
#define RECEIVE_STEP  20
#define CONNECT_DELAY 1000

#define HOST_TO_NET true
#define NET_TO_HOST false
#define LAN true
//#define WAN false
//#define BLUE true
#define RED false

#define ARG         (*(int*)ThreadArgv)
#define state       GameCondition.GameState
#define difficulty  GameCondition.GameDifficulty

#define ObjectMinX(Object) ((Object)->DestRect.x)
#define ObjectMaxX(Object) (ObjectMinX(Object) + (Object)->DestRect.w)
#define ObjectMinY(Object) ((Object)->DestRect.y)
#define ObjectMaxY(Object) (ObjectMinY(Object) + (Object)->DestRect.h)
#define ObjectMidX(Object) ((ObjectMinX(Object) + ObjectMaxX(Object)) / 2.0)
#define ObjectMidY(Object) ((ObjectMinY(Object) + ObjectMaxY(Object)) / 2.0)

//类型定义

typedef enum game_state{          //游戏状态
  MAIN = 0,
  ONE_PLAYER = 1,
  TWO_PLAYER = 2,
  LOCAL_DEATH = 3,
  NET_DEATH = 4
}game_state;

typedef enum game_difficulty{     //游戏难度
  EASY = 0,
  NORMAL = 1,
  HARD = 2
}game_difficulty;

typedef struct game_condition{    //游戏信息
  game_state GameState;
  game_difficulty GameDifficulty;
  bool LocalNum;
  float LocalBallX;
  float LocalBallY;
  int LocalBoardX;
  int LocalBoardY;
  bool NetNum;
  float NetBallX;
  float NetBallY;
  int NetBoardX;
  int NetBoardY;
}game_condition;

typedef enum Element{             //元素种类
  FIRE = 0,
  WATER = 1,
  ICE = 2,
  THUNDER = 3,
  EMPTY = 4
}Element;

typedef struct Board{   //挡板
  int life;
  Element element;
  uint8_t alpha;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_Rect SourceRect;
  SDL_Rect DestRect;
}Board;

typedef enum BallDir{
  LEFT = -1,
  VERTICAL = 0,
  RIGHT = 1
}BallDir;

typedef struct Ball{    //弹球
  int score;
  bool SetOff;
  BallDir dir;
  double k;
  Board* board;
  Element element;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_FRect DestRect;
}Ball;

typedef struct Brick{   //砖块
  int life;
  Element element;
  uint8_t alpha;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_Rect DestRect;
}Brick;

//变量定义

static WSADATA data;
static SOCKET ServerSocket;
static SOCKADDR_IN ServerAddr;
static char ServerIP[17] = "0.0.0.0";
static u_short ServerPort = 0;
static char Send[BUF_SIZE] = {0};
static char Receive[BUF_SIZE] = {0};
static game_condition GameCondition;

static const int BoardLenVec[] = {200, 150, 100};
static const int BoardLifeVec[] = {10, 8, 5};
static const int BoardMoveSpeedVec[] = {10, 8, 5};

static const double BallInitialK = 100;
static const double BallMoveSpeedVec[] = {0.5, 1.0, 1.5};
static const char* BallPathVec[] = {"img/FireBall.png",
                                    "img/WaterBall.png",
                                    "img/IceBall.png",
                                    "img/ThunderBall.png",
                                    "img/EmptyBall.png"};

static Brick* BrickArr = NULL;
static bool BrickPre = false;
static const int BrickLifeVec[] = {1, 2, 3};
static const int BrickNum[] = {30, 60, 90};
static const char* BrickPathVec[] = { "img/FireBrick.png",
                                      "img/WaterBrick.png",
                                      "img/IceBrick.png",
                                      "img/ThunderBrick.png",
                                      "img/EmptyBrick.png"};

static pthread_t TransmissionThread;
static int ThreadArg = 0;
static pthread_mutex_t GameInitMutex, GameWaitMutex, BoardMoveMutex;
static pthread_cond_t GameInitCond, GameWaitCond, BoardMoveCond;

static Board *LocalBoard, *NetBoard;
static Ball *LocalBall, *NetBall;

static int record = 0;
static FILE *cfg;
static FILE *LogFile;

static SDL_Window *Window = NULL;                  //窗口
static SDL_Renderer *Renderer = NULL;              //渲染器

static SDL_Surface *MainSurface = NULL;            //mainUI
static SDL_Texture *MainTexture = NULL;
static SDL_Rect MainRect;

static SDL_Surface *GameSurface = NULL;            //gameUI
static SDL_Texture *GameTexture = NULL;
static SDL_Rect GameRect;

static TTF_Font *Font = NULL;                      //字体
static SDL_Surface *FontSurface = NULL;
static SDL_Texture *FontTexture = NULL;
static SDL_Rect FontRect;
static SDL_Color FontColor = {0x88, 0x0a, 0x39, 0xff};

static Mix_Music *bgm;                            //BGM
static const int BgmVolumeVec[] = {100, 45, 60, 80};
static const char* BgmPathVec[] = {"msc/George Duke-It's On.mp3",
                                    "msc/Tennis-I'm Callin'.mp3",
                                    "msc/Nieve-WriteThisDown.mp3",
                                    "msc/Swollen Members-Fuel Injected.mp3",
                                    "msc/Lakey-Inspired-The-Process.mp3"};

//函数声明

void ClientCfgInit(void);
void ClientLibInit(void);
void ClientIPInit(char *IP, u_short port, bool flag);
void ClientLoadResource(void);
void ClientEventLoop(void);
void ClientGameInit(void);
void ClientGameChange(void);
void ClientGameQuit(void);
void ClientQuit(int code);
void ClientConnect(SOCKET *server, SOCKADDR_IN* server_addr);
void* ClientTransmissionThread(void* ThreadArgv);
void ClientDataResolve(char* buf, int flag);
void ClientRender(void);
void ClientPlayBGM(void);
void ClientDrawText(const char* text, int x, int y, bool pre);

void BoardCreate(Board* board, bool color);
void BoardMove(Board* board, Ball* ball, SDL_KeyCode operation);
void BoardDestroy(Board* board);

void BallCreate(Ball* ball, Board* board);
void BallMove(Ball* ball);
void BallHit(Ball* ball, Brick* brick, const char* mode);
void BallDestroy(Ball* ball);

void BrickCreate(Brick* brick, int x, int y, Element color);
void BrickArrCreate(Brick *arr);
void BrickArrDestroy(Brick *arr);
void BrickDestroy(Brick* brick);

void SocketReceive(SOCKET soc, char* buf);
void SocketSend(SOCKET soc, const char* buf);

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int recordf(const char* format, ...){    //向日志文件中记录信息
  if (record){
    int retval;
    __builtin_va_list local_argv; __builtin_va_start(local_argv, format);
    retval = __mingw_vfprintf(LogFile, format, local_argv);
    __builtin_va_end(local_argv );
    return retval;
  }
}

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int errorf(const char* format, ...){   //在日志和标准误差流中记录错误
  int retval_r, retval_e;
  if (record) {
    __builtin_va_list local_argv; __builtin_va_start(local_argv, format);
    retval_r = __mingw_vfprintf(LogFile, format, local_argv);
    __builtin_va_end(local_argv );
  }
  __builtin_va_list local_argv; __builtin_va_start(local_argv, format);
  retval_e = __mingw_vfprintf(stderr, format, local_argv );
  __builtin_va_end( local_argv );
  return min(retval_e, retval_r);
}

#endif
