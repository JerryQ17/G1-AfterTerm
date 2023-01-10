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

//宏定义

#define DEBUG

#define IP_FAILURE      1
#define SOCKET_FAILURE  2
#define BIND_FAILURE    3
#define RECEIVE_FAILURE 4
#define SEND_FAILURE    5

#define HOST_TO_NET 1
#define NET_TO_HOST 2

#define CFG_ITEM 1
#define CFG_PATH "cfg/cfg.txt"
#define LOG_PATH "cfg/slog.txt"

#define MES_MAX_LEN 4
#define BUF_SIZE 1000
#define SEND_STEP 20
#define RECEIVE_STEP 20


//变量定义

static WSADATA data;
static char ServerIP[17] = {0};
static int SocketNumber = 0;

static int record = 0;
static FILE *cfg;
static FILE *log_file;

//函数声明

void ServerInit(void);
void ServerIP_LAN(char *ip);
void ServerQuit(int code);
void SocketCreate(SOCKET* soc, struct sockaddr_in *addr);
void SocketListen(SOCKET soc, int backlog);
void SocketAccept(const SOCKET* ser, SOCKET* cli, struct sockaddr_in* cli_addr);
void SocketReceive(SOCKET soc, char* buf);
void SocketSend(SOCKET soc, char* buf);
void DataResolve(char* buf, int flag);
void recordf(const char* format, ...);
void errorf(const char* format, ...);
#endif
