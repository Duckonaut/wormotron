#include "log.h"

#include <stdio.h>

FILE *g_log_file;

#ifndef __unix__
#include <windows.h>
#include <io.h>

HANDLE g_log_handle;

void log_init()
{
    g_log_handle = GetStdHandle(STD_ERROR_HANDLE);

    if (g_log_handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "GetStdHandle failed: %d\n", GetLastError());
        exit(1);
    }

#ifdef RELEASE
    g_log_file = fopen("log.txt", "w");
#endif
}

void log_close()
{
    if (g_log_file)
    {
        fclose(g_log_file);
    }
}

#else

void log_init()
{
#ifdef RELEASE
    g_log_file = fopen("log.txt", "w");
#endif
}

void log_close()
{
    if (g_log_file)
    {
        fclose(g_log_file);
    }
}

#endif

