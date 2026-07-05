#ifndef __PARSER_CONTEXT__
#define __PARSER_CONTEXT__

#include "io/context/RenderRuntimeState.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/SymbolTable.h"
#include "io/pov/context/TokenizerStream.h"
#include "io/pov/lexer/ITokenStream.h"
#include "io/pov/lexer/PovToken.h"
#include "io/pov/lexer/ReservedWord.h"

class PovRayMaterial;

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
    // PovRayRendererConfiguration so io/pov/context does not depend on environment.
    void setDiagnostics(bool verboseErrors, const char *statFileName) {
        mVerboseErrors = verboseErrors;
        mStatFileName = statFileName;
    }
    bool writesVerboseErrors() const { return mVerboseErrors; }
    const char *statFileName() const { return mStatFileName; }

    // Selects the CSG classification algorithm at parse time: false
    // (default) keeps point-membership classification
    // (ConstructiveSolidGeometryByMorganRules), true makes CsgParser build
    // ConstructiveSolidGeometryByRaySegment nodes instead ([ROTH1982]).
    // Decoupled from PovRayRendererConfiguration for the same reason as
    // setDiagnostics().
    void setCsgRoth(bool value) { mCsgRoth = value; }
    bool usesCsgRoth() const { return mCsgRoth; }

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
    bool mCsgRoth = false;
    const char *mStatFileName;
    double mAntialiasThreshold;
    RenderRuntimeState *mRuntimeState;
};

#endif
