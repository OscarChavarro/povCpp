#include "io/pov/antlr/AntlrSceneRuntimePipeline.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <limits.h>
#include <unistd.h>

#include "io/FileLocator.h"

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
#include "common/logger/Logger.h"

namespace {
bool startsWith(const std::string &text, const char *prefix)
{
    return text.compare(0, std::char_traits<char>::length(prefix), prefix) == 0;
}

std::string trimLeft(const std::string &line)
{
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) {
        ++i;
    }
    return line.substr(i);
}

std::string normalizePath(const std::string &path)
{
    std::string out = path;
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

std::string parentDir(const std::string &path)
{
    const std::string p = normalizePath(path);
    const size_t slash = p.find_last_of('/');
    if (slash == std::string::npos) {
        return ".";
    }
    if (slash == 0) {
        return "/";
    }
    return p.substr(0, slash);
}

std::string joinPath(const std::string &a, const std::string &b)
{
    if (b.empty()) {
        return normalizePath(a);
    }
    const std::string bn = normalizePath(b);
    if (!bn.empty() && (bn[0] == '/' || (bn.size() > 1 && bn[1] == ':'))) {
        return bn;
    }
    std::string an = normalizePath(a);
    if (an.empty()) {
        an = ".";
    }
    if (an.back() == '/') {
        return an + bn;
    }
    return an + "/" + bn;
}

std::string canonicalizePath(const std::string &path)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        return normalizePath(path);
    }

    std::string absPath = path;
    if (path.empty() || (path[0] != '/' && (path.size() <= 1 || path[1] != ':'))) {
        absPath = joinPath(cwd, path);
    }

    absPath = normalizePath(absPath);

    // Try to simplify the path by opening and getting the canonical name
    // For now, just return the normalized absolute path
    return absPath;
}

bool readFileText(const std::string &path, std::string &out)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.good()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

bool resolveIncludePath(const std::string &includeName, const std::string &includingFile,
    std::string &resolvedPath)
{
    std::string contentProbe;
    const std::string includingDir = parentDir(includingFile);

    const std::string fromLocal = joinPath(includingDir, includeName);
    if (readFileText(fromLocal, contentProbe)) {
        resolvedPath = fromLocal;
        return true;
    }

    // Legacy-style fallback: also try ../include, ../../include, etc.
    std::string dirCursor = includingDir;
    for (int depth = 0; depth < 8; ++depth) {
        const std::string candidate = joinPath(joinPath(dirCursor, "include"), includeName);
        if (readFileText(candidate, contentProbe)) {
            resolvedPath = candidate;
            return true;
        }
        const std::string parent = parentDir(dirCursor);
        if (parent == dirCursor) {
            break;
        }
        dirCursor = parent;
    }

    const java::ArrayList<java::String> &paths = FileLocator::searchPaths();
    for (long int i = 0; i < paths.size(); ++i) {
        const std::string candidate = joinPath(paths.get(i).toCString(), includeName);
        if (readFileText(candidate, contentProbe)) {
            resolvedPath = candidate;
            return true;
        }
    }
    return false;
}

bool extractQuotedString(const std::string &line, std::string &out)
{
    const size_t first = line.find('"');
    if (first == std::string::npos) {
        return false;
    }
    const size_t second = line.find('"', first + 1);
    if (second == std::string::npos || second <= first + 1) {
        return false;
    }
    out = line.substr(first + 1, second - first - 1);
    return true;
}

void expandIncludesRecursive(const std::string &path, std::set<std::string> &activeStack,
    std::string &out)
{
    const std::string normalized = normalizePath(path);

    if (activeStack.find(normalized) != activeStack.end()) {
        throw std::runtime_error("Cyclic #include detected while expanding ANTLR input: " + normalized);
    }
    activeStack.insert(normalized);

    std::string text;
    if (!readFileText(normalized, text)) {
        throw std::runtime_error("Failed to open input file for ANTLR pipeline: " + normalized);
    }

    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const std::string trimmed = trimLeft(line);
        if (startsWith(trimmed, "#include")) {
            std::string includeName;
            if (!extractQuotedString(trimmed, includeName)) {
                throw std::runtime_error("Malformed #include directive in: " + normalized);
            }

            std::string resolved;
            if (!resolveIncludePath(includeName, normalized, resolved)) {
                throw std::runtime_error("Failed to resolve #include '" + includeName + "' from: " + normalized);
            }

            expandIncludesRecursive(resolved, activeStack, out);
            out.push_back('\n');
            continue;
        }
        out += line;
        out.push_back('\n');
    }

    activeStack.erase(normalized);
}

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
AntlrSceneRuntimePipeline::parseAndApply(RenderFrame *framePtr, std::string &error)
{
#ifndef POV_WITH_ANTLR_RUNTIME
    (void)framePtr;
    error = "ANTLR runtime pipeline is not enabled in this build";
    return false;
#else
    const char *inputPath = RenderingConfiguration::global().inputFileName;
    Logger::info("[ANTLR-PIPELINE] parseAndApply called with inputPath: %s\n",
        (inputPath != nullptr ? inputPath : "null"));

    if (inputPath == nullptr || inputPath[0] == '\0') {
        error = "Missing input file name for ANTLR pipeline";
        Logger::error("[ANTLR-PIPELINE] %s\n", error.c_str());
        return false;
    }

    std::string canonPath = canonicalizePath(inputPath);
    Logger::info("[ANTLR-PIPELINE] Canonicalized path: %s\n", canonPath.c_str());

    std::string input;
    try {
        std::set<std::string> includeStack;
        Logger::info("[ANTLR-PIPELINE] Starting include expansion\n");
        expandIncludesRecursive(canonPath, includeStack, input);
        const char *dumpExpanded = std::getenv("POVCPP_ANTLR_DUMP_EXPANDED");
        if (dumpExpanded != nullptr && dumpExpanded[0] != '\0') {
            std::ofstream out(dumpExpanded, std::ios::out | std::ios::trunc);
            if (out.is_open()) {
                out << input;
            }
        }
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }

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
