#ifndef SERVER_H
#define SERVER_H

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
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

//宏定义

#define DEBUG

#define IP_FAILURE      1
#define SOCKET_FAILURE  2
#define BIND_FAILURE    3
#define RECEIVE_FAILURE 4
#define SEND_FAILURE    5

#define HOST_TO_NET true
#define NET_TO_HOST false

#define CFG_ITEM 1
#define CFG_PATH "cfg/cfg.txt"
#define LOG_PATH "cfg/slog.txt"

#define MES_MAX_LEN 4
#define BUF_SIZE 1000
#define SEND_STEP 20
#define RECEIVE_STEP 20

#define ARG (*(int*)ThreadArgv)

//变量定义

static WSADATA data;
static char ServerIP[17] = {0};
static int ClientNumber = 0;
static SOCKET ServerSocket, ClientSocket[2];
static struct sockaddr_in ServerAddr, ClientAddr[2];
static float BallX[2] = {0}, BallY[2] = {0};
static int BoardX[2] = {0}, BoardY[2] = {0};

static pthread_t ClientThread[2];
static int ThreadArg[2] = {0, 1};
static pthread_mutex_t GameInitMutex, TransmissionMutex[2];
static pthread_cond_t GameInitCond, TransmissionCond[2];

static int record = 0;
static FILE *cfg;
static FILE *LogFile;

static const int BrickNum[] = {30, 60, 90};
static char BrickOrder[BUF_SIZE] = {0};

//函数声明

void ServerInit(void);
void ServerIP_LAN(char *ip);
void ServerQuit(int code);
void* ServerTransmissionThread(void* ThreadArgv);
char* ServerDataResolve(char* buf, int ThreadNum, bool flag);

void BrickArrCreate(char* ret, int diff);

void SocketCreate(SOCKET* soc, struct sockaddr_in *addr);
void SocketListen(SOCKET soc, int backlog);
void SocketAccept(const SOCKET* ser, SOCKET* cli, struct sockaddr_in* cli_addr);
void SocketReceive(SOCKET soc, char* buf);
void SocketSend(SOCKET soc, const char* buf);

static inline void recordf(const char* format, ...)__attribute__((__format__(printf, 1, 2)));
static inline void errorf(const char* format, ...)__attribute__((__format__(printf, 1, 2)));

#endif
