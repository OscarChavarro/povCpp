#ifndef __PARSER_CONTEXT_H__
#define __PARSER_CONTEXT_H__

#include "io/ReservedWord.h"
#include "io/TokenStruct.h"
#include "io/pov/ParseGlobals.h"

class RenderFrame;
class RGBAColorPaletteSpan;

class ParserContext {
  public:
    static ReservedWord *reservedWords();
    static TokenStruct &token();
    static int *termCounts();
    static RenderFrame *&parsingFrame();
    static RGBAColorPaletteSpan *&constructionMap();
    static Constant *constants();
    static int &numberOfConstants();
    static int &degenerateTriangles();
};

#endif
