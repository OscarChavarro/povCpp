#ifndef __PARSE_ERROR_REPORTER__
#define __PARSE_ERROR_REPORTER__

#include "io/pov/context/ParserContext.h"

class ParseErrorReporter {
  public:
    static void reportError(const char *str);
    static void reportError(const char *str, ParserContext &ctx);
    static void parseError(TOKEN tokenId);
    static void parseError(TOKEN tokenId, ParserContext &ctx);
    static void typeError();
    static void typeError(ParserContext &ctx);
    static void reportUndeclared();
    static void reportUndeclared(ParserContext &ctx);
    static const char *getTokenString(TOKEN tokenId);
    static const char *getTokenString(TOKEN tokenId, ParserContext &ctx);

  private:
    static void reportLocation(ParserContext &ctx);
    static void writeVerboseStatLine(FILE *statFile, ParserContext &ctx);
};

#endif
