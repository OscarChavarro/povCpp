#ifndef __PARSER_CONTEXT_H__
#define __PARSER_CONTEXT_H__

#include "io/ReservedWord.h"
#include "io/TokenStruct.h"
#include "io/pov/ITokenStream.h"
#include "io/pov/ParserConstants.h"
#include "io/pov/ParseGlobals.h"
#include "io/pov/SymbolTable.h"
#include "io/pov/TokenizerTokenStream.h"

class RenderFrame;
class RGBAColorPaletteSpan;

class ParserContext {
  public:
    ParserContext();

    ReservedWord *reservedWords();
    TokenStruct &token();
    int *termCounts();
    RenderFrame *&parsingFrame();
    RGBAColorPaletteSpan *&constructionMap();
    Constant *constants();
    int &numberOfConstants();
    SymbolTable &symbols();
    int &degenerateTriangles();
    ITokenStream &tokenStream();
    void setTokenStream(ITokenStream *tokenStream);

  private:
    static TokenizerTokenStream sDefaultTokenStream;
    static RenderFrame *sSharedParsingFramePtr;
    static RGBAColorPaletteSpan *sSharedConstructionMap;
    static SymbolTable sSharedSymbols;
    static int sSharedDegenerateTriangles;

    ITokenStream *mTokenStream;
    RenderFrame **mParsingFramePtr;
    RGBAColorPaletteSpan **mConstructionMap;
    SymbolTable *mSymbols;
    int *mDegenerateTriangles;
};

#endif
