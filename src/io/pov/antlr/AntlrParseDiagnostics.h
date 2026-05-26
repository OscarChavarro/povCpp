#ifndef __POV_ANTLR_PARSE_DIAGNOSTICS_H__
#define __POV_ANTLR_PARSE_DIAGNOSTICS_H__

#include <string>

#include "io/pov/ParseErrorReporter.h"

class AntlrParseDiagnostics {
  public:
    static void raiseSyntaxError(
        const std::string &sourceFile, int line, int column, const std::string &message);
    static std::string formatLocation(const std::string &sourceFile, int line, int column);
};

#endif
