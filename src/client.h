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
#define NUMBER_SIZE 40              //数字大小
#define CFG_ITEM    3               //配置数量

#define SINGLE_MIN_X 255            //mainUI的单人按钮的x最小值
#define SINGLE_MAX_X 512            //mainUI的单人按钮的x最大值
#define SINGLE_MIN_Y 384            //mainUI的单人按钮的y最小值
#define SINGLE_MAX_Y 513            //mainUI的单人按钮的y最大值
#define DOUBLE_MIN_X SINGLE_MAX_X   //mainUI的多人按钮的x最小值
#define DOUBLE_MAX_X 770            //mainUI的多人按钮的x最大值
#define DOUBLE_MIN_Y SINGLE_MIN_Y   //mainUI的多人按钮的y最小值
#define DOUBLE_MAX_Y SINGLE_MAX_Y   //mainUI的多人按钮的y最大值
#define README_MIN_X DOUBLE_MAX_X   //mainUI的帮助按钮的x最小值
#define README_MAX_X 1025           //mainUI的单人按钮的x最大值
#define README_MIN_Y SINGLE_MIN_Y   //mainUI的帮助按钮的y最小值
#define README_MAX_Y SINGLE_MAX_Y   //mainUI的帮助按钮的y最大值
#define RETURN_MAX_X 144
#define RETURN_MAX_Y 72
#define USER_TIP_X   400
#define USER_TIP_Y   600
#define BOARD_INIT_Y 650
#define BOARD_INIT_H 20

#define EASY_LEN     200
#define NORMAL_LEN   150
#define HARD_LEN     100
#define EASY_REMAIN  10
#define NOR_REMAIN   8
#define HARD_REMAIN  5

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
#define ADDR_STEP     5
#define CONNECT_STEP  20
#define SEND_STEP     20
#define RECEIVE_STEP  20
#define CONNECT_DELAY 1000

#define HOST_TO_NET 1
#define NET_TO_HOST 2
#define LAN true
#define WAN false
#define RED true
#define BLUE false

#define ARG (*(int*)ThreadArgv)

//类型定义

typedef enum game_state{          //游戏状态
  MAIN = 0,
  ONE_PLAYER = 1,
  TWO_PLAYER = 2,
  ONE_DEATH = 3
}game_state;

typedef enum game_difficulty{     //游戏难度
  EASY = 1,
  NORMAL = 2,
  HARD = 3
}game_difficulty;

typedef struct game_condition{    //游戏信息
  game_state GameState;
  game_difficulty GameDifficulty;
  bool LocalNum;
  int LocalBallX;
  int LocalBallY;
  int LocalBoardX;
  bool NetNum;
  int NetBallX;
  int NetBallY;
  int NetBoardX;
}game_condition;

typedef struct Board{   //弹球
  bool life;
  int remain;
  SDL_Surface* sur;
  SDL_Texture* tex;
  SDL_Rect SourceRect;
  SDL_Rect DestRect;
}Board;

typedef struct Ball{    //挡板
  int x;
  int y;
  SDL_Surface* sur;
  SDL_Texture* tex;
}Ball;

//变量定义

static WSADATA data;
static SOCKET ServerSocket;
static struct sockaddr_in ServerAddr;
static char ServerIP[17] = "0.0.0.0";
static unsigned short ServerPort = 0;
static char Send[BUF_SIZE] = {0};
static char Receive[BUF_SIZE] = {0};
static game_condition GameCondition;

static pthread_t TransmissionThread;
static int ThreadArg = 0;
static pthread_mutex_t GameInitMutex;
static pthread_cond_t GameInitCond;
static pthread_mutex_t GameWaitMutex;
static pthread_cond_t GameWaitCond;

static Board* LocalBoard;
static Board* NetBoard;

static int record = 0;
static FILE *cfg;
static FILE *log_file;

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
static SDL_Color FontColor = {0, 0, 0, 255};

static TTF_Font *NumberFont = NULL;                //数字
static SDL_Surface *NumberFontSurface = NULL;
static SDL_Texture *NumberFontTexture = NULL;
static SDL_Rect NumberFontRect;
static SDL_Color NumberFontColor = {0x88, 0x0a, 0x39, 0xff};

static Mix_Music *bgm;                            //BGM
static const int BgmVolume[] = {100, 45, 60, 80};
static const char* BgmPath[] = {"msc/George Duke-It's On.mp3",
                                "msc/Tennis-I'm Callin'.mp3",
                                "msc/Nieve-WriteThisDown.mp3",
                                "msc/Swollen Members-Fuel Injected.mp3",
                                "msc/Lakey-Inspired-The-Process.mp3"};

//函数声明

void ClientCfgInit(void);
void ClientLibInit(void);
void ClientInitAddr(char *IP, u_short port, bool flag);
void ClientLoadResource(void);
void ClientEventLoop(void);
void ClientGameInit(void);
void ClientGameQuit(void);
void ClientQuit(int code);
void ClientConnect(SOCKET *server, SOCKADDR_IN* server_addr);
void* ClientTransmissionThread(void* ThreadArgv);
void ClientRender(void);
void ClientBGM(void);
void BoardCreate(Board* board, bool color);
void BoardDestroy(Board* board);
void SocketReceive(SOCKET soc, char* buf);
void SocketSend(SOCKET soc, const char* buf);
void ClientDrawText(char *text, int x, int y, bool pre);
void ClientDrawNumber(int num, int x, int y, bool pre);
void recordf(const char* format, ...);
void errorf(const char* format, ...);

#endif
