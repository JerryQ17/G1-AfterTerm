/**
 * @file debug.h
 * @author JerryQ
 * @details 调试程序时，用来记录信息的函数和宏\n
 * 将第12行代码注释掉即可关闭调试功能
 * */
#ifndef MY_DEBUG_HEADER__NJU_SE_2022__
#define MY_DEBUG_HEADER__NJU_SE_2022__

#include <stdio.h>

#define MY_DEBUG_FLAG__NJU_SE_2022__

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif  //min

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

/**
 * @name RecordFlag
 * @details 控制recordf函数是否运行，值为0时不运行，值为非零值时运行
 * @sa recordf
 */

int RecordFlag = 0;

/**
 * @name LogFilePtr
 * @details 指向日志文件的文件指针
 * @sa recordf
 */

FILE *LogFilePtr = NULL;

/**
 * @name recordf
 * @category 函数
 * @param format 格式串
 * @param ... 可变数量的数据项
 * @return 和printf一样
 * @details 在日志文件(LogFilePtr)中记录信息
 */

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int recordf(const char *format, ...) {
  if (RecordFlag && LogFilePtr) {
    int retval;
    __builtin_va_list local_argv;
    __builtin_va_start(local_argv, format);
    retval = __mingw_vfprintf(LogFilePtr, format, local_argv);
    __builtin_va_end(local_argv);
    return retval;
  }
  return 0;
}

/**
 * @name errorf
 * @category 宏
 * @param format 格式串
 * @param argv 可变数量的数据项
 * @details 在日志和标准误差流中记录错误
 * @sa recordf
 */
#ifndef errorf
#define errorf(format, argv...)     \
do {                                \
  recordf(format, ##argv);          \
  fprintf(stderr, format, ##argv);  \
}while (0)
#endif //errorf

/**
 * @name debugf_b
 * @category 函数
 * @param format 格式串
 * @param ... 可变数量的数据项
 * @return 和printf一样
 * @details 简略的调试(brief)
 */

__mingw_ovr
__attribute__((__format__(printf, 1, 2))) __MINGW_ATTRIB_NONNULL(1)
int debugf_b(const char *format, ...) {
#ifdef MY_DEBUG_FLAG__NJU_SE_2022__
  int retval;
  __builtin_va_list local_argv;
  __builtin_va_start(local_argv, format);
  retval = __mingw_vfprintf(stdout, format, local_argv);
  __builtin_va_end(local_argv);
  return retval;
#else
  return 0;
#endif  //MY_DEBUG_FLAG__NJU_SE_2022__
}

/**
 * @name debugf_d
 * @category 函数
 * @param line  行数
 * @param func  函数名
 * @param file  文件名
 * @param format 格式串
 * @param ... 可变数量的数据项
 * @return 和printf一样
 * @details 详细的调试(detailed)，格式串末尾不需要回车，最后自动输出一个回车
 * @sa debugf
 */

__mingw_ovr
__attribute__((__format__(printf, 4, 5))) __MINGW_ATTRIB_NONNULL(4)
int debugf_d(const int line, const char *func, const char *file, const char *format, ...) {
#ifdef MY_DEBUG_FLAG__NJU_SE_2022__
  int retval;
  printf("%s Line %d Function %s:\n", file, line, func);
  __builtin_va_list local_argv;
  __builtin_va_start(local_argv, format);
  retval = __mingw_vfprintf(stdout, format, local_argv);
  __builtin_va_end(local_argv);
  printf("\n");
  return retval;
#else
  return 0;
#endif  //MY_DEBUG_FLAG__NJU_SE_2022__
}

/**
 * @name debugf
 * @category 宏
 * @param format 格式串
 * @param argv 可变数量的数据项
 * @details 使用debugf_d函数的简便方式
 * @example debugf("C4:%d", 7355608);
 * @sa debugf_d
 */

#ifndef debugf
#define debugf(format, argv...) debugf_d(__LINE__, __func__, __FILE__, format, ##argv)
#endif //debugf

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  //MY_DEBUG_HEADER__NJU_SE_2022__