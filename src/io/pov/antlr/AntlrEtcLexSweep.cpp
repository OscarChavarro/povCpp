#include <dirent.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef POV_WITH_ANTLR_RUNTIME
#include "POVLexer.h"
#include "antlr4-runtime.h"
#endif

namespace {
bool hasSuffix(const std::string &name, const std::string &suffix)
{
    return name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isDirectory(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

void collectPovFiles(const std::string &root, std::vector<std::string> &out)
{
    DIR *dir = opendir(root.c_str());
    if (dir == nullptr) {
        return;
    }
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        const std::string path = root + "/" + name;
        if (isDirectory(path)) {
            collectPovFiles(path, out);
        } else if (hasSuffix(name, ".pov")) {
            out.push_back(path);
        }
    }
    closedir(dir);
}

#ifdef POV_WITH_ANTLR_RUNTIME
class FirstLexerErrorListener : public antlr4::BaseErrorListener {
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

enum BraceContext {
    BRACE_OTHER = 0,
    BRACE_CSG = 1
};

bool lexFile(const std::string &path, int &boundedInCsgCount, int &clippedInCsgCount)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.good()) {
        std::cerr << "Cannot open file: " << path << "\n";
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();

    antlr4::ANTLRInputStream stream(text);
    POVLexer lexer(&stream);
    FirstLexerErrorListener listener;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&listener);

    std::vector<std::unique_ptr<antlr4::Token>> ownedTokens;
    std::vector<int> tokenTypes;
    while (true) {
        std::unique_ptr<antlr4::Token> tk(lexer.nextToken());
        if (!tk) {
            break;
        }
        const int type = tk->getType();
        tokenTypes.push_back(type);
        ownedTokens.push_back(std::move(tk));
        if (type == antlr4::Token::EOF) {
            break;
        }
    }

    if (listener.hasError) {
        std::cerr << path << ":" << listener.line << ":" << listener.column
                  << " lexer error: " << listener.message << "\n";
        return false;
    }

    std::vector<int> braceStack;
    int pendingCsgOpen = 0;
    for (size_t i = 0; i < tokenTypes.size(); ++i) {
        const int t = tokenTypes[i];
        if (t == POVLexer::UNION || t == POVLexer::INTERSECTION || t == POVLexer::DIFFERENCE) {
            pendingCsgOpen = 1;
            continue;
        }
        if (t == POVLexer::LEFT_CURLY) {
            if (pendingCsgOpen) {
                braceStack.push_back(BRACE_CSG);
            } else {
                braceStack.push_back(BRACE_OTHER);
            }
            pendingCsgOpen = 0;
            continue;
        }
        if (t == POVLexer::RIGHT_CURLY) {
            if (!braceStack.empty()) {
                braceStack.pop_back();
            }
            pendingCsgOpen = 0;
            continue;
        }
        if (t == POVLexer::BOUNDED_BY) {
            if (!braceStack.empty() && braceStack.back() == BRACE_CSG) {
                ++boundedInCsgCount;
            }
            pendingCsgOpen = 0;
            continue;
        }
        if (t == POVLexer::CLIPPED_BY) {
            if (!braceStack.empty() && braceStack.back() == BRACE_CSG) {
                ++clippedInCsgCount;
            }
            pendingCsgOpen = 0;
            continue;
        }
        if (t != antlr4::Token::EOF) {
            pendingCsgOpen = 0;
        }
    }

    return true;
}
#endif
}

int main()
{
#ifndef POV_WITH_ANTLR_RUNTIME
    std::cerr << "ANTLR runtime not enabled.\n";
    return 2;
#else
    std::vector<std::string> povFiles;
    collectPovFiles("etc", povFiles);
    if (povFiles.empty()) {
        std::cerr << "No .pov files found under ./etc\n";
        return 1;
    }

    int failed = 0;
    int boundedInCsgTotal = 0;
    int clippedInCsgTotal = 0;
    for (size_t i = 0; i < povFiles.size(); ++i) {
        int boundedInCsg = 0;
        int clippedInCsg = 0;
        const bool ok = lexFile(povFiles[i], boundedInCsg, clippedInCsg);
        if (!ok) {
            ++failed;
            continue;
        }
        if (boundedInCsg > 0 || clippedInCsg > 0) {
            std::cout << "CSG bounded/clipped candidate in " << povFiles[i]
                      << " bounded_by=" << boundedInCsg
                      << " clipped_by=" << clippedInCsg << "\n";
        }
        boundedInCsgTotal += boundedInCsg;
        clippedInCsgTotal += clippedInCsg;
    }

    std::cout << "ANTLR lex sweep: files=" << povFiles.size()
              << " lexer_failures=" << failed
              << " csg_bounded_by_hits=" << boundedInCsgTotal
              << " csg_clipped_by_hits=" << clippedInCsgTotal << "\n";

    return failed == 0 ? 0 : 1;
#endif
}
