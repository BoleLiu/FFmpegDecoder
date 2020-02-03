#ifndef _SYS_LOG_
#define _SYS_LOG_

#include <stdio.h>

#ifdef __ANDROID_API__
#include <android/log.h>
#endif

#define SYS_LOG_LEVEL_CLOSE 0
#define SYS_LOG_LEVEL_ERROR 1
#define SYS_LOG_LEVEL_WARNING 2
#define SYS_LOG_LEVEL_INFO  3
#define SYS_LOG_LEVEL_DEBUG 4
#define SYS_LOG_LEVEL_VERBOSE 5

#ifndef SYS_LOG_TAG
#define SYS_LOG_TAG "PLDroidShortVideo"
#endif

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#endif

#ifdef __ANDROID_API__
#define LOG_VERBOSE ANDROID_LOG_VERBOSE
#define LOG_DEBUG   ANDROID_LOG_DEBUG
#define LOG_INFO    ANDROID_LOG_INFO
#define LOG_WARNING ANDROID_LOG_WARN
#define LOG_ERROR   ANDROID_LOG_ERROR
#define PRINT(level,format,...) __android_log_print(level,SYS_LOG_TAG,format,##__VA_ARGS__);
#else
#define LOG_VERBOSE "V"
#define LOG_DEBUG   "D"
#define LOG_INFO    "I"
#define LOG_WARNING "W"
#define LOG_ERROR   "E"
#define PRINT(level,format,...) printf("%s %s: " format,level,SYS_LOG_TAG,##__VA_ARGS__);
#endif

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_VERBOSE
#define LOGV(format,...) PRINT(LOG_VERBOSE,format,##__VA_ARGS__)
#else
#define LOGV(format,...)
#endif

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
#define LOGD(format,...) PRINT(LOG_DEBUG,format,##__VA_ARGS__)
#else
#define LOGD(format,...)
#endif

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_INFO
#define LOGI(format,...) PRINT(LOG_INFO,format,##__VA_ARGS__)
#else
#define LOGI(format,...)
#endif

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING
#define LOGW(format,...) PRINT(LOG_WARNING,format,##__VA_ARGS__)
#else
#define LOGW(format,...)
#endif

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_ERROR
#define LOGE(format,...) PRINT(LOG_ERROR,format,##__VA_ARGS__)
#else
#define LOGE(format,...)
#endif

#endif
