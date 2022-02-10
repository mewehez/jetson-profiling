#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <cstdio>
#include "config.h"

#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_WHITE     "\033[1;37m"
#define COLOR_RED       "\033[0;31m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_NONE      "\033[0m"

#define WARNING     COLOR_YELLOW "[Warn] " COLOR_NONE
#define ERROR       COLOR_RED "[Error] " COLOR_NONE
#define INFO        COLOR_BLUE "[Info] " COLOR_NONE
#define DEBUG       COLOR_WHITE "[Debug] " COLOR_NONE

#ifdef LOG_VALUES
    #define log(format, args...) printf(format, ##args)
#else
    #define log(format, args...)
#endif

#ifdef __cplusplus
}
#endif

#endif