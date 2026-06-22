#ifndef __PARSER_CONTEXT__
#define __PARSER_CONTEXT__

#include "common/RenderRuntimeState.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/SymbolTable.h"
#include "io/pov/context/TokenizerStream.h"
#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/lexer/PovToken.h"
#include "io/pov/lexer/ReservedWord.h"

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

    PovRayMaterial *getDefaultTexture() const { return mDefaultTexture; }
    void setDefaultTexture(PovRayMaterial *texture) { mDefaultTexture = texture; }

    // Diagnostics needed by the error reporter, decoupled from the render-time
    // RenderingConfiguration so io/pov/context does not depend on environment.
    void setDiagnostics(bool verboseErrors, const char *statFileName) {
        mVerboseErrors = verboseErrors;
        mStatFileName = statFileName;
    }
    bool writesVerboseErrors() const { return mVerboseErrors; }
    const char *statFileName() const { return mStatFileName; }

    double getAntialiasThreshold() const { return mAntialiasThreshold; }
    void setAntialiasThreshold(double threshold) { mAntialiasThreshold = threshold; }

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
    PovRayMaterial *mDefaultTexture;
    bool mVerboseErrors;
    const char *mStatFileName;
    double mAntialiasThreshold;
    RenderRuntimeState *mRuntimeState;
};

#endif
