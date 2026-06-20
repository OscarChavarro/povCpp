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
class RenderRuntimeState;

class ParserContext {
  public:
    ParserContext();

    const ReservedWord *reservedWords();
    PovToken &token();
    Constant *constants();
    SymbolTable &symbols();
    int findConstant();
    int &degenerateTriangles();
    ITokenStream &tokenStream();
    void resetTokenStreamHistory();
    Tokenizer &tokenizer() { return mTokenizer; }

    PovrayMaterial *getDefaultTexture() const { return mDefaultTexture; }
    void setDefaultTexture(PovrayMaterial *texture) { mDefaultTexture = texture; }

    const RenderingConfiguration *getReportingConfig() const { return mReportingConfig; }
    void setReportingConfig(const RenderingConfiguration *config) { mReportingConfig = config; }
    bool writesVerboseErrors() const;
    const char *statFileName() const;
    RenderRuntimeState *getRuntimeState() const { return mRuntimeState; }
    void setRuntimeState(RenderRuntimeState *runtimeState) { mRuntimeState = runtimeState; }

  private:
    Tokenizer mTokenizer;
    TokenizerStream mDefaultTokenStream;
    SymbolTable mSharedSymbols;
    int mSharedDegenerateTriangles;
    ITokenStream *mTokenStream;
    SymbolTable * const mSymbols;
    int * const mDegenerateTriangles;
    PovrayMaterial *mDefaultTexture;
    const RenderingConfiguration *mReportingConfig;
    RenderRuntimeState *mRuntimeState;
};

#endif
