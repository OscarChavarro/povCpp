#ifndef __LOGGER_H__
#define __LOGGER_H__

class Logger {
  public:
    static void error(const char *format, ...);
    static void info(const char *format, ...);
};

#endif
