#ifndef __PARSER_CONTEXT__
#define __PARSER_CONTEXT__

#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/lexer/PovToken.h"
#include "io/pov/lexer/ReservedWord.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/SymbolTable.h"
#include "io/pov/context/TokenizerStream.h"

class PovrayMaterial;
class RenderingConfiguration;

class ParserContext {
  public:
    ParserContext();

    const ReservedWord *reservedWords();
    PovToken &token();
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

    PovrayMaterial *getDefaultTexture() const { return mDefaultTexture; }
    void setDefaultTexture(PovrayMaterial *texture) { mDefaultTexture = texture; }

    const RenderingConfiguration *getReportingConfig() const { return mReportingConfig; }
    void setReportingConfig(const RenderingConfiguration *config) { mReportingConfig = config; }

  private:
    static TokenizerStream sDefaultTokenStream;
    static SymbolTable sSharedSymbols;
    static int sSharedDegenerateTriangles;
    static ITokenStream *sForcedTokenStream;

    ITokenStream *mTokenStream;
    SymbolTable * const mSymbols;
    int * const mDegenerateTriangles;
    PovrayMaterial *mDefaultTexture;
    const RenderingConfiguration *mReportingConfig;
};

#endif
