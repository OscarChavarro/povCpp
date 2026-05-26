#include "io/pov/antlr/AntlrSceneRuntimePipeline.h"

#include <fstream>
#include <sstream>

#include "environment/material/RendererConfiguration.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/antlr/AntlrParseDiagnostics.h"
#include "io/pov/antlr/AntlrParsedSceneProgram.h"
#include "io/pov/antlr/AntlrSceneIr.h"
#include "io/pov/antlr/AntlrSceneLowering.h"

#ifdef POV_WITH_ANTLR_RUNTIME
#include "POVLexer.h"
#include "POVParser.h"
#include "antlr4-runtime.h"
#include "io/pov/antlr/AntlrParseTreeToIrMapper.h"

namespace {
class FirstSyntaxErrorListener : public antlr4::BaseErrorListener {
  public:
    bool hasError = false;
    int line = 1;
    int column = 1;
    std::string message;

    void syntaxError(antlr4::Recognizer * /*recognizer*/, antlr4::Token * /*offendingSymbol*/,
        size_t lineArg, size_t charPositionInLine, const std::string &msg,
        std::exception_ptr /*e*/) override
    {
        if (hasError) {
            return;
        }
        hasError = true;
        line = (int)lineArg;
        column = (int)charPositionInLine + 1;
        message = msg;
    }
};
}
#endif

bool
AntlrSceneRuntimePipeline::parseAndApply(RenderFrame *framePtr, ParserContext & /*ctx*/, std::string &error)
{
#ifndef POV_WITH_ANTLR_RUNTIME
    (void)framePtr;
    error = "ANTLR runtime pipeline is not enabled in this build";
    return false;
#else
    const char *inputPath = RenderingConfiguration::global().inputFileName;
    if (inputPath == nullptr || inputPath[0] == '\0') {
        error = "Missing input file name for ANTLR pipeline";
        return false;
    }

    std::ifstream in(inputPath, std::ios::binary);
    if (!in.good()) {
        error = std::string("Failed to open input file for ANTLR pipeline: ") + inputPath;
        return false;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string input = buffer.str();

    antlr4::ANTLRInputStream stream(input);
    POVLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    POVParser parser(&tokens);

    FirstSyntaxErrorListener listener;
    parser.removeErrorListeners();
    parser.addErrorListener(&listener);

    POVParser::SceneContext *sceneCtx = parser.scene();
    if (listener.hasError || sceneCtx == nullptr) {
        const std::string msg = listener.hasError ? listener.message : "null scene context";
        const int line = listener.hasError ? listener.line : 1;
        const int column = listener.hasError ? listener.column : 1;
        AntlrParseDiagnostics::raiseSyntaxError(inputPath, line, column, msg);
    }

    AntlrParsedSceneProgram *parsed = AntlrParseTreeToIrMapper::mapScene(sceneCtx);
    if (parsed == nullptr || parsed->program == nullptr) {
        if (parsed != nullptr) {
            delete parsed;
        }
        error = "ANTLR mapper returned null IR program";
        return false;
    }

    try {
        AntlrSceneLowering::applyProgram(*parsed->program, framePtr);
    } catch (...) {
        AntlrSceneIrNodes::destroyProgram(parsed->program);
        parsed->program = nullptr;
        delete parsed;
        throw;
    }

    AntlrSceneIrNodes::destroyProgram(parsed->program);
    parsed->program = nullptr;
    delete parsed;
    return true;
#endif
}
