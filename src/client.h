#ifndef CLIENT_H
#define CLIENT_H

//包含库

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <winsock2.h>

#include "debug.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

//宏定义

#define WIN_TITLE   "Breakout Client"         //窗口标题
#define WIN_WIDTH   1280                      //窗口宽度
#define WIN_HEIGHT  720                       //窗口高度
#define FONT_SIZE   60                        //字体大小
#define CFG_ITEM    4                         //配置数量
#define BUF_SIZE    2000                      //缓冲大小
#define FRAME_RATE  60
#define REACT_BONUS 1                         //反应奖励

#define SINGLE_MIN_X  255                     //mainUI的单人按钮的x最小值
#define SINGLE_MAX_X  512                     //mainUI的单人按钮的x最大值
#define SINGLE_MIN_Y  384                     //mainUI的单人按钮的y最小值
#define SINGLE_MAX_Y  513                     //mainUI的单人按钮的y最大值
#define DOUBLE_MIN_X  SINGLE_MAX_X            //mainUI的多人按钮的x最小值
#define DOUBLE_MAX_X  770                     //mainUI的多人按钮的x最大值
#define DOUBLE_MIN_Y  SINGLE_MIN_Y            //mainUI的多人按钮的y最小值
#define DOUBLE_MAX_Y  SINGLE_MAX_Y            //mainUI的多人按钮的y最大值
#define README_MIN_X  DOUBLE_MAX_X            //mainUI的帮助按钮的x最小值
#define README_MAX_X  1025                    //mainUI的单人按钮的x最大值
#define README_MIN_Y  SINGLE_MIN_Y            //mainUI的帮助按钮的y最小值
#define README_MAX_Y  SINGLE_MAX_Y            //mainUI的帮助按钮的y最大值
#define RETURN_MAX_X  144                     //gameUI的返回按钮的x最大值
#define RETURN_MAX_Y  72                      //gameUI的返回按钮的y最大值
#define USER_TIP_X    400                     //用户提示的x最小值
#define USER_TIP_Y    600                     //用户提示的y最小值
#define SCORE_X       1190                    //分数的x最小值
#define SCORE_Y       10                      //分数的y最小值
#define WL_X          525                     //游戏胜利或失败提示的x最小值
#define WL_Y          330                     //游戏胜利或失败提示的y最小值
#define BOARD_MIN_Y   600                     //弹板的y最小值
#define BOARD_INIT_Y  650                     //弹板的y初始值

#define CFG_PATH        "cfg/cfg.txt"         //配置路径
#define LOG_PATH        "cfg/clog.txt"        //日志路径
#define FONT_PATH       "font/calibri.ttf"    //字体路径
#define MAIN_UI_PATH    "img/mainUI.png"      //主界面路径
#define GAME_UI_PATH    "img/gameUI.png"      //游戏界面路径
#define RED_BOARD_PATH  "img/RedBoard.png"    //红色弹板路径
#define BLUE_BOARD_PATH "img/BlueBoard.png"   //蓝色弹板路径

#define IP_STEP       5                       //输入IP最大次数
#define CONNECT_STEP  20                      //尝试连接服务端最大次数

#define CONNECT_DELAY 1000                    //连接服务端失败的显示时间
#define WL_DELAY      2000                    //输赢的显示时间
#define OFFLINE_DELAY 2000                    //另一名玩家下线的显示时间

#define LAN true                              //局域网模式
//#define WAN false
//#define BLUE true
#define RED false                             //红色弹板
#define CLIENT_TO_SERVER true                 //客户端发送服务端
#define SERVER_TO_CLIENT false                //服务端发送客户端

#define IP_FAILURE              1
#define SOCKET_FAILURE          2
#define CONNECT_FAILURE         3
#define SDL_INIT_ERROR          4
#define WINDOW_CREATE_ERROR     5
#define RENDERER_CREATE_ERROR   6
#define TTF_INIT_ERROR          7
#define TTF_OPEN_FONT_ERROR     8
#define MIX_OPEN_AUDIO_ERROR    9
#define NUM_ERROR               10

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
}game_condition;

typedef enum Element{             //元素种类
  FIRE = 0,
  WATER = 1,
  ICE = 2,
  THUNDER = 3,
  EMPTY = 4
}Element;

typedef struct Board{             //挡板
  int life;
  uint8_t alpha;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_Rect SourceRect;
  SDL_Rect DestRect;
}Board;

typedef enum BallDir{             //弹球方向
  LEFT = -1,
  VERTICAL = 0,
  RIGHT = 1
}BallDir;

typedef struct Ball{              //弹球
  int score;
  int hit;
  bool SetOff;
  BallDir dir;
  double k;
  Board* board;
  Element element;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_FRect DestRect;
}Ball;

typedef struct Brick{             //砖块
  int life;
  Element element;
  uint8_t alpha;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_Rect DestRect;
}Brick;

//变量定义

static WSADATA          data;
static SOCKET           ServerSocket;
static SOCKADDR_IN      ServerAddr;
static char             ServerIP[17]      = "0.0.0.0";
static u_short          ServerPort        = 0;
static int              mod               = 0;
static char             Send[BUF_SIZE]    = {0};
static char             Receive[BUF_SIZE] = {0};
static game_condition   GameCondition;
static volatile bool    GameChangeFlag    = false;

static const int BoardLenVec[]        = {200, 150, 100};
static const int BoardLifeVec[]       = {10, 8, 5};
static const int BoardMoveSpeedVec[]  = {10, 8, 5};

static const double BallInitialK = 100;
static const double BallMoveSpeedVec[]  = {0.5, 1.0, 1.5};
static const char*  BallPathVec[]       = { "img/FireBall.png",
                                            "img/WaterBall.png",
                                            "img/IceBall.png",
                                            "img/ThunderBall.png",
                                            "img/EmptyBall.png"};

static Brick*       BrickArr        = NULL;
static bool         BrickPre        = false;
static const int    BrickLifeVec[]  = {1, 2, 3};
static const int    BrickNum[]      = {30, 60, 90};
static const char*  BrickPathVec[]  = { "img/FireBrick.png",
                                        "img/WaterBrick.png",
                                        "img/IceBrick.png",
                                        "img/ThunderBrick.png",
                                        "img/EmptyBrick.png"};

static pthread_t        TransmissionThread;
static int              ThreadArg = 0;
static pthread_mutex_t  GameInitMutex, GameWaitMutex, GameChangeMutex, BoardMoveMutex, NetQuitMutex, GameQuitMutex;
static pthread_cond_t   GameInitCond, GameWaitCond, GameChangeCond, BoardMoveCond;

static Board* LocalBoard;
static Board* NetBoard;
static Ball*  LocalBall;
static Ball*  NetBall;

static SDL_Window*    Window    = NULL;               //窗口
static SDL_Renderer*  Renderer  = NULL;               //渲染器

static SDL_Surface* MainSurface = NULL;               //mainUI
static SDL_Texture* MainTexture = NULL;
static SDL_Rect     MainRect;

static SDL_Surface* GameSurface = NULL;               //gameUI
static SDL_Texture* GameTexture = NULL;
static SDL_Rect     GameRect;

static TTF_Font*    Font        = NULL;               //字体
static SDL_Surface* FontSurface = NULL;
static SDL_Texture* FontTexture = NULL;
static SDL_Rect     FontRect;
static SDL_Color    FontColor   = {0x88, 0x0a, 0x39, 0xff};

static Mix_Music*   bgm;                              //BGM
static Mix_Chunk*   sound;                            //音效
static const int    BgmVolumeVec[]  = {100, 45, 60, 80, 70};
static const char*  BgmPathVec[]    = { "msc/George Duke-It's On.mp3",
                                        "msc/Tennis-I'm Callin'.mp3",
                                        "msc/Nieve-WriteThisDown.mp3",
                                        "msc/Swollen Members-Fuel Injected.mp3",
                                        "msc/Lakey-Inspired-The-Process.mp3"};
static const char*  SoundPathVec[]  = { "msc/thunder.mp3",
                                        "msc/bomb.mp3",
                                        "msc/death.mp3"};

//函数声明

void  ClientCfgInit(void);
void  ClientLibInit(void);
void  ClientIPInit(const char *IP, u_short port);
void  ClientLoadResource(void);
void  ClientEventLoop(void);
void  ClientGameInit(void);
void  ClientGameChange(void);
void  ClientGameQuit(void);
void  ClientQuit(int code);
void  ClientConnect(SOCKET *server, SOCKADDR_IN* server_addr);
void* ClientTransmissionThread(void* ThreadArgv);
void  ClientDataResolve(char* buf, bool flag);
void  ClientRender(void);
void  ClientPlayBGM(void);
void  ClientPlaySound(int num);
void  ClientDrawText(const char* text, int x, int y, bool pre);

void  BoardCreate(Board* board, bool color);
void  BoardMove(Board* board, Ball* ball, SDL_KeyCode operation);
void  BoardDestroy(Board* board);

void  BallCreate(Ball* ball, Board* board);
void  BallMove(Ball* ball);
void  BallHit(Ball* ball, Brick* brick, const char* mode);
void  BallDestroy(Ball* ball);

void  BrickCreate(Brick* brick, int x, int y, Element color);
void  BrickArrCreate(Brick *arr);
void  BrickArrDestroy(Brick *arr);
void  BrickDestroy(Brick* brick);

void  ElementReact(Ball* ball, Brick* brick);

void  SocketReceive(SOCKET soc, char* buf);
void  SocketSend(SOCKET soc, const char* buf);

#endif  //CLIENT_H