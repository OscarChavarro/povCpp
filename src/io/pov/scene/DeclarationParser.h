#ifndef __DECLARATION_PARSER_H__
#define __DECLARATION_PARSER_H__

#include "io/pov/context/ParserContext.h"

class DeclarationParser {
  public:
    static void parseDeclare();
    static void parseDeclare(ParserContext &ctx);
};

#endif
