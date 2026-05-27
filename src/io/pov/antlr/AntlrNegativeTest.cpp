#include <iostream>
#include <string>

#include "io/pov/antlr/AntlrParseDiagnostics.h"
#include "io/pov/ParseErrorReporter.h"

#ifdef POV_WITH_ANTLR_RUNTIME
#include "POVLexer.h"
#include "POVParser.h"
#include "antlr4-runtime.h"
#endif

int main()
{
#ifndef POV_WITH_ANTLR_RUNTIME
    std::cerr << "ANTLR runtime not enabled.\n";
    return 2;
#else
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

    auto expectParseFailureWithLocation = [](const std::string &text, const std::string &name) -> bool {
        antlr4::ANTLRInputStream stream(text);
        POVLexer lexer(&stream);
        antlr4::CommonTokenStream tokens(&lexer);
        POVParser parser(&tokens);
        FirstSyntaxErrorListener listener;
        parser.removeErrorListeners();
        parser.addErrorListener(&listener);
        parser.scene();

        if (!listener.hasError) {
            return false;
        }

        try {
            AntlrParseDiagnostics::raiseSyntaxError(name, listener.line, listener.column, listener.message);
        } catch (const ParseErrorReporter::ParseException &) {
            return true;
        }
        return false;
    };

    const std::string invalidDeclare = "#declare X = sphere { <1,2,3> }\n";
    const std::string invalidMissingBrace = "sphere { <1,2,3> 4\ncamera { location <0,0,-5> }\n";
    const std::string invalidBadVector = "sphere { <1,2> 4 }\n";
    const std::string invalidLightField = "light_source { <0,0,0> colour <1,1,1> point_at <0,0> }\n";
    const std::string invalidCsgNesting = "difference { union { sphere { <0,0,0> 1 } sphere { <1,0,0> 1 } }\n";
    const std::string invalidCsgBounded = "difference { bounded_by { sphere { <0,0,0> 1 } } }\n";
    const std::string invalidCsgNoShadow = "difference { no_shadow sphere { <0,0,0> 1 } }\n";

    if (!expectParseFailureWithLocation(invalidDeclare, "<invalid-declare>")) {
        std::cerr << "Expected failure: invalid #declare\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidMissingBrace, "<invalid-missing-brace>")) {
        std::cerr << "Expected failure: missing brace\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidBadVector, "<invalid-bad-vector>")) {
        std::cerr << "Expected failure: invalid vector\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidLightField, "<invalid-light>")) {
        std::cerr << "Expected failure: invalid light field\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidCsgNesting, "<invalid-csg>")) {
        std::cerr << "Expected failure: invalid csg nesting\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidCsgBounded, "<invalid-csg-bounded>")) {
        std::cerr << "Expected failure: invalid csg bounded_by\n";
        return 1;
    }
    if (!expectParseFailureWithLocation(invalidCsgNoShadow, "<invalid-csg-no-shadow>")) {
        std::cerr << "Expected failure: invalid csg no_shadow\n";
        return 1;
    }

    std::cout << "ANTLR negative test passed.\n";
    return 0;
#endif
}
