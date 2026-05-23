#include "common/logger/Logger.h"

#include <cstdarg>
#include <cstdio>

void
Logger::error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void
Logger::info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
