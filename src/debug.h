/**
 * @file debug.h
 * @author JerryQ
 * @details 调试程序时，用来记录信息的函数和宏（还有一些杂项）\n
 * 将第12行代码注释掉即可关闭调试功能
 * */
#ifndef MY_DEBUG_HEADER__NJU_SE_2022__
#define MY_DEBUG_HEADER__NJU_SE_2022__

#include <stdio.h>
#include <time.h>

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
#define errorf(format, argv...)     \
do {                                \
  recordf(format, ##argv);          \
  fprintf(stderr, format, ##argv);  \
}while (0)

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

#define debugf(format, argv...) debugf_d(__LINE__, __func__, __FILE__, format, ##argv)

/**
 * @name formatTime
 * @category 函数
 * @param buf 指向目标数组的指针，用来复制产生的字符串
 * @param BufSize 被复制到buf的最大字符数
 * @param format 格式串，如果为NULL，则默认为"%Y-%m-%d %H:%M:%S"形式
 * @return 如果产生的字符串小于BufSize个字符（包括空结束字符），则会返回复制到buf中的字符总数（不包括空结束字符），否则返回零
 * @details
 * 说明符  替换为                 样例\n
 * %a   缩写的星期几名称            Sun\n
 * %A   完整的星期几名称            Sunday\n
 * %b   缩写的月份名称             Mar\n
 * %B	完整的月份名称             March\n
 * %c	日期和时间表示法            Sun Aug 19 02:56:02 2012\n
 * %d	一月中的第几天（01-31）      19\n
 * %H	24 小时格式的小时（00-23）   14\n
 * %I	12 小时格式的小时（01-12）   05\n
 * %j	一年中的第几天（001-366）    231\n
 * %m	十进制数表示的月份（01-12）    08\n
 * %M	分（00-59）                55\n
 * %p	AM 或 PM 名称              PM\n
 * %S	秒（00-61）                02\n
 * %U	一年中的第几周，以第一个星期日作为第一周的第一天（00-53）	33\n
 * %w	十进制数表示的星期几，星期日表示为 0（0-6）	4\n
 * %W	一年中的第几周，以第一个星期一作为第一周的第一天（00-53）	34\n
 * %x	日期表示法                   08/19/12\n
 * %X	时间表示法                   02:50:06\n
 * %y	年份，最后两个数字（00-99）    01\n
 * %Y	年份                      2012\n
 * %Z	时区的名称或缩写            CDT\n
 * %%	一个%符号                   %
 * @sa FormatTime
 */

__mingw_ovr
__attribute__((__format__(strftime, 3, 0))) __MINGW_ATTRIB_NONNULL(1)
size_t formatTime(char* buf, size_t BufSize, const char* format){
  time_t current = time(NULL);
  return strftime(buf, BufSize, format ? format : "%Y-%m-%d %H:%M:%S", localtime(&current));
}

/**
 * @name FormatTime
 * @category 宏
 * @param buf 指向目标数组的指针，用来复制产生的字符串
 * @details 使用formatTime函数的简便方式
 * @sa formatTime
 */

#define FormatTime(buf) formatTime(buf, sizeof(buf), NULL)

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  //MY_DEBUG_HEADER__NJU_SE_2022__