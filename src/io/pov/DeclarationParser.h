#ifndef __DECLARATION_PARSER_H__
#define __DECLARATION_PARSER_H__

class ParserContext;

class DeclarationParser {
  public:
    static void parseDeclare();
    static void parseDeclare(ParserContext &ctx);
};

#endif
