/*! \brief Primitive log functionality
    \file logger.h
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-23
    
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
 */
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define logprintf(msg, ...)                                                        \
    printf("\033[37;1;1m>> %s():%d (%s)\033[0m \n", __func__, __LINE__, __FILE__); \
    printf(msg, ##__VA_ARGS__);

/*! \brief Several typical log level constants */
enum
{
    LOGLEVEL_VERBOSE = 100,
    LOGLEVEL_DISPLAY = 90,
    LOGLEVEL_INFO = 80,
    LOGLEVEL_WARNING = 50,
    LOGLEVEL_ERROR = 25,
    LOGLEVEL_CRITICAL = 5
};

extern unsigned g_logLv;

#define lvlog(lv, msg, ...)           \
    if (lv <= g_logLv)                \
    {                                 \
        logprintf(msg, ##__VA_ARGS__) \
    }

/*! \brief String hash function using djb2 algorithm
    \param str Inpust string.
    \return 
 */
static unsigned long
hash_djb2(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

typedef intptr_t UCommandLineDescriptor;

/*! \brief Parse command line arguments.
    \param argc Number of arguments.
    \param argv Argument values
    \param bRemoveOpts Set true to remove optional arguments from given list.
 */
void ParseCommandLineArgs(UCommandLineDescriptor *p, int *argc, char ***argv, bool bRemoveOpts);

/*! \brief Release commandline data */
void FreeCommandLineArgs(UCommandLineDescriptor *p);