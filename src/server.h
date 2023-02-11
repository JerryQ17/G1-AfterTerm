#ifndef SERVER_H
#define SERVER_H

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

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

//宏定义

#define BUF_SIZE 1000

#define IP_FAILURE      1
#define SOCKET_FAILURE  2
#define BIND_FAILURE    3
#define NUM_ERROR       10

#define CLIENT_TO_SERVER true
#define SERVER_TO_CLIENT false

#define CFG_ITEM 1
#define CFG_PATH "cfg/cfg.txt"
#define LOG_PATH "cfg/slog.txt"

#define ARG (*(int*)ThreadArgv)

//变量定义

static WSADATA          data;
static char             ServerIP[17] = {0};
static SOCKET           ServerSocket;
static SOCKET           ClientSocket[2];
static SOCKADDR_IN      ServerAddr;
static SOCKADDR_IN      ClientAddr[2];
static int              ClientNumber = 0;
static int              mod = 0;
static int              difficulty = 0;
static int              BoardX[2] = {0};
static int              BoardY[2] = {0};
static int              BoardLife[2] = {0};
static int              BrickLife[90] = {0};
static const int        BrickNum[] = {30, 60, 90};
static char             BrickOrder[BUF_SIZE] = {0};
static float            BallX[2] = {0};
static float            BallY[2] = {0};
static volatile bool    PlayerQuit = false;

static pthread_t        ClientThread[2];
static int              ThreadArg[2] = {0, 1};
static pthread_mutex_t  GameInitMutex;
static pthread_mutex_t  TransmissionMutex[2];
static pthread_cond_t   GameInitCond;
static pthread_cond_t   TransmissionCond[2];

//函数声明

void  ServerInit(void);
void  ServerIP_LAN(char *ip);
void  ServerQuit(int code);
_Noreturn
void* ServerTransmissionThread(void* ThreadArgv);
void  ServerDataResolve(char* buf, int ThreadNum, bool flag);

void  BrickArrCreate(char* ret, int diff);

void  SocketCreate(SOCKET* soc, struct sockaddr_in *addr);
void  SocketListen(SOCKET soc, int backlog);
void  SocketAccept(const SOCKET* ser, SOCKET* cli, struct sockaddr_in* cli_addr);
void  SocketReceive(SOCKET soc, char* buf);
void  SocketSend(SOCKET soc, const char* buf);

#endif  //CLIENT_H