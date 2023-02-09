#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define DEBUG

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif  //min

extern int    record;
extern FILE*  LogFile;

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int recordf(const char* format, ...){   //向日志文件中记录信息
  if (record) {
    int retval;
    __builtin_va_list local_argv; __builtin_va_start(local_argv, format);
    retval = __mingw_vfprintf(LogFile, format, local_argv);
    __builtin_va_end(local_argv);
    return retval;
  }
}

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int errorf(const char* format, ...){    //在日志和标准误差流中记录错误
  int retval_e, retval_r;
  __builtin_va_list local_argv_e; __builtin_va_start(local_argv_e, format);
  retval_e = __mingw_vfprintf(stderr, format, local_argv_e);
  __builtin_va_end(local_argv_e);
  if (record) {
    __builtin_va_list local_argv_r; __builtin_va_start(local_argv_r, format);
    retval_r = __mingw_vfprintf(LogFile, format, local_argv_r);
    __builtin_va_end(local_argv_r);
    return min(retval_e, retval_r);
  }
  return retval_e;
}

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int Debug(const char* format, ...){     //调试用
#ifdef DEBUG
  int retval;
  __builtin_va_list local_argv; __builtin_va_start(local_argv, format);
  __mingw_vfprintf(stdout, format, local_argv);
  __builtin_va_end(local_argv);
  return retval;
#endif  //DEBUG
}

#endif  //DEBUG_H
