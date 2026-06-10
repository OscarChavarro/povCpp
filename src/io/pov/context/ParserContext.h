#ifndef __PARSER_CONTEXT_H__
#define __PARSER_CONTEXT_H__

#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/lexer/TokenStruct.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/SymbolTable.h"
#include "io/pov/context/TokenizerStream.h"

class RenderFrame;

class ParserContext {
  public:
    ParserContext();

    ReservedWord *reservedWords();
    TokenStruct &token();
    RenderFrame *&parsingFrame();
    Constant *constants();
    int &numberOfConstants();
    SymbolTable &symbols();
    int findConstant();
    int &degenerateTriangles();
    ITokenStream &tokenStream();
    void setTokenStream(ITokenStream *tokenStream);
    void resetTokenStreamHistory();
    static void forceTokenStream(ITokenStream *tokenStream);
    static void clearForcedTokenStream();

  private:
    static TokenizerStream sDefaultTokenStream;
    static RenderFrame *sSharedParsingFramePtr;
    static SymbolTable sSharedSymbols;
    static int sSharedDegenerateTriangles;
    static ITokenStream *sForcedTokenStream;

    ITokenStream *mTokenStream;
    RenderFrame **mParsingFramePtr;
    SymbolTable *mSymbols;
    int *mDegenerateTriangles;
};

#endif
