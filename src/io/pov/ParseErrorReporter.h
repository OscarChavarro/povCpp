#ifndef __PARSE_ERROR_REPORTER_H__
#define __PARSE_ERROR_REPORTER_H__

#include "io/Tokenizer.h"

class ParserContext;

class ParseErrorReporter {
  public:
    static void Error(const char *str);
    static void Error(const char *str, ParserContext &ctx);
    static void parseError(TOKEN tokenId);
    static void parseError(TOKEN tokenId, ParserContext &ctx);
    static void typeError();
    static void typeError(ParserContext &ctx);
    static void Undeclared();
    static void Undeclared(ParserContext &ctx);
    static char *getTokenString(TOKEN tokenId);
    static char *getTokenString(TOKEN tokenId, ParserContext &ctx);
};

#endif
