#ifndef __PARSER_CONTEXT__
#define __PARSER_CONTEXT__

#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/lexer/PovToken.h"
#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/SymbolTable.h"
#include "io/pov/context/TokenizerStream.h"
#include "environment/scene/Scene.h"

class ParserContext {
  public:
    ParserContext();

    const ReservedWord *reservedWords();
    PovToken &token();
    Scene *&parsingFrame();
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
    static Scene *sSharedParsingFramePtr;
    static SymbolTable sSharedSymbols;
    static int sSharedDegenerateTriangles;
    static ITokenStream *sForcedTokenStream;

    ITokenStream *mTokenStream;
    Scene ** const mParsingFramePtr;
    SymbolTable * const mSymbols;
    int * const mDegenerateTriangles;
};

#endif
