#include "io/pov/antlr/AntlrParseDiagnostics.h"

#include "common/logger/Logger.h"

std::string
AntlrParseDiagnostics::formatLocation(const std::string &sourceFile, int line, int column)
{
    const std::string file = sourceFile.empty() ? "<antlr>" : sourceFile;
    return file + ":" + std::to_string(line) + ":" + std::to_string(column);
}

void
AntlrParseDiagnostics::raiseSyntaxError(
    const std::string &sourceFile, int line, int column, const std::string &message)
{
    const std::string location = formatLocation(sourceFile, line, column);
    Logger::error("Error at %s\n", location.c_str());
    Logger::error("ANTLR syntax error: %s\n", message.c_str());

    const std::string merged = std::string("ANTLR syntax error at ") + location + ": " + message;
    throw ParseErrorReporter::ParseException(merged.c_str());
}
