#ifndef __PARSE_ERROR_REPORTER_H__
#define __PARSE_ERROR_REPORTER_H__

#include "io/Tokenize.h"

class ParseErrorReporter {
  public:
    static void Error(const char *str);
    static void parseError(TOKEN tokenId);
    static void typeError();
    static void Undeclared();
    static char *getTokenString(TOKEN tokenId);
};

#endif
