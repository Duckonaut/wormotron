#pragma once
#include <stdio.h>

extern FILE* g_log_file;

#ifndef __unix__
#include <Windows.h>
extern HANDLE g_log_handle;
#endif

void log_init(void);
void log_close(void);

// if shipping, strip all logging
#ifndef SHIPPING

// if in release, log to pre-opened file descriptor
#ifdef RELEASE
#define LOG_FILE g_log_file
#else
#define LOG_FILE stderr
#endif

#define __LOG_ERROR_STR "ERROR"
#define __LOG_WARNING_STR "WARNING"
#define __LOG_INFO_STR "INFO"
#define __LOG_DEBUG_STR "DEBUG"

#ifdef RELEASE
#define LOG_ERROR(...) fprintf(LOG_FILE, __LOG_ERROR_STR ": " __VA_ARGS__)
#define LOG_WARNING(...) fprintf(LOG_FILE, __LOG_WARNING_STR ": " __VA_ARGS__)
#define LOG_INFO(...) fprintf(LOG_FILE, __LOG_INFO_STR ": " __VA_ARGS__)
#define LOG_DEBUG(...)
#else
#ifdef __unix__
#define __LOG_NC "\033[0m"
#define __LOG_ERROR "\033[31m"
#define __LOG_WARNING "\033[33m"
#define __LOG_INFO "\033[32m"
#define __LOG_DEBUG "\033[34m"

#define LOG_ERROR(...) fprintf(LOG_FILE, __LOG_ERROR __LOG_ERROR_STR __LOG_NC ": " __VA_ARGS__)
#define LOG_WARNING(...)                                                                       \
    fprintf(LOG_FILE, __LOG_WARNING __LOG_WARNING_STR __LOG_NC ": " __VA_ARGS__)
#define LOG_INFO(...) fprintf(LOG_FILE, __LOG_INFO __LOG_INFO_STR __LOG_NC ": " __VA_ARGS__)
#define LOG_DEBUG(...) fprintf(LOG_FILE, __LOG_DEBUG __LOG_DEBUG_STR __LOG_NC ": " __VA_ARGS__)

#else // __unix__
#define __LOG_ERROR 0x0C
#define __LOG_WARNING 0x0E
#define __LOG_INFO 0x0A
#define __LOG_DEBUG 0x09

#define LOG_ERROR(...)                                                                         \
    {                                                                                          \
        SetConsoleTextAttribute(g_log_handle, __LOG_ERROR);                                    \
        fprintf(LOG_FILE, __LOG_ERROR_STR ": ");                                               \
        SetConsoleTextAttribute(g_log_handle, 0x07);                                           \
        fprintf(LOG_FILE, __VA_ARGS__);                                                        \
    }
#define LOG_WARNING(...)                                                                       \
    {                                                                                          \
        SetConsoleTextAttribute(g_log_handle, __LOG_WARNING);                                  \
        fprintf(LOG_FILE, __LOG_WARNING_STR ": ");                                             \
        SetConsoleTextAttribute(g_log_handle, 0x07);                                           \
        fprintf(LOG_FILE, __VA_ARGS__);                                                        \
    }
#define LOG_INFO(...)                                                                          \
    {                                                                                          \
        SetConsoleTextAttribute(g_log_handle, __LOG_INFO);                                     \
        fprintf(LOG_FILE, __LOG_INFO_STR ": ");                                                \
        SetConsoleTextAttribute(g_log_handle, 0x07);                                           \
        fprintf(LOG_FILE, __VA_ARGS__);                                                        \
    }
#define LOG_DEBUG(...)                                                                         \
    {                                                                                          \
        SetConsoleTextAttribute(g_log_handle, __LOG_DEBUG);                                    \
        fprintf(LOG_FILE, __LOG_DEBUG_STR ": ");                                               \
        SetConsoleTextAttribute(g_log_handle, 0x07);                                           \
        fprintf(LOG_FILE, __VA_ARGS__);                                                        \
    }
#endif // __unix__
#endif // RELEASE

#else // SHIPPING

#define LOG_ERROR(...)
#define LOG_WARNING(...)
#define LOG_INFO(...)
#define LOG_DEBUG(...)

#endif

