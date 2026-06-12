#include <cstdio>
#include <cstdlib>
#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/RendererConfiguration.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"

void
ParseErrorReporter::reportLocation(ParserContext &ctx)
{
    const char *file = (ctx.token().Filename != nullptr) ? ctx.token().Filename : "<unknown>";
    const int line = ctx.token().tokenLineNo + 1;
    const int column = (ctx.token().tokenColumnNo > 0) ? ctx.token().tokenColumnNo : 1;
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "Error at %s:%d:%d\n", file, line, column);
        Logger::reportMessage("ParseErrorReporter", Logger::ERROR, "", _logMsg);
    }
}

void
ParseErrorReporter::writeVerboseStatLine(FILE *statFile, ParserContext &ctx)
{
    const char *file = (ctx.token().Filename != nullptr) ? ctx.token().Filename : "<unknown>";
    const int line = ctx.token().tokenLineNo + 1;
    const int column = (ctx.token().tokenColumnNo > 0) ? ctx.token().tokenColumnNo : 1;
    fprintf(statFile, "Error at %s:%d:%d\n", file, line, column);
}

const char *
ParseErrorReporter::getTokenString(TOKEN tokenId)
{
    ParserContext ctx;
    return ParseErrorReporter::getTokenString(tokenId, ctx);
}

const char *
ParseErrorReporter::getTokenString(TOKEN tokenId, ParserContext &ctx)
{
    int i;

    for (i = 0; i < Tokenizer::LAST_TOKEN; i++) {
        if (ctx.reservedWords()[i].tokenNumber == tokenId) {
            return ctx.reservedWords()[i].Token_Name;
        }
    }
    return "";
}

void
ParseErrorReporter::parseError(TOKEN tokenId)
{
    ParserContext ctx;
    ParseErrorReporter::parseError(tokenId, ctx);
}

void
ParseErrorReporter::parseError(TOKEN tokenId, ParserContext &ctx)
{
    const char *expected;
    const char *found;
    FILE *statFile;
    reportLocation(ctx);
    expected = ParseErrorReporter::getTokenString(tokenId, ctx);
    found = ParseErrorReporter::getTokenString(ctx.token().tokenId, ctx);
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "%s expected but %s found instead\n", expected, found);
        Logger::reportMessage("ParseErrorReporter", Logger::ERROR, "", _logMsg);
    }
    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(
            statFile, "%s expected but %s found instead\n", expected, found);
        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::typeError()
{
    ParserContext ctx;
    ParseErrorReporter::typeError(ctx);
}

void
ParseErrorReporter::typeError(ParserContext &ctx)
{
    FILE *statFile;
    reportLocation(ctx);
    fprintf(stderr, "Identifier %s is the wrong type\n", ctx.token().Token_String);
    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(statFile, "Identifier %s is the wrong type\n", ctx.token().Token_String);

        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::reportUndeclared()
{
    ParserContext ctx;
    ParseErrorReporter::reportUndeclared(ctx);
}

void
ParseErrorReporter::reportUndeclared(ParserContext &ctx)
{
    FILE *statFile;
    reportLocation(ctx);
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "Undeclared identifier %s\n", ctx.token().Token_String);
        Logger::reportMessage("ParseErrorReporter", Logger::ERROR, "", _logMsg);
    }
    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(statFile, "Undeclared identifier %s\n", ctx.token().Token_String);
        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::reportError(const char *str)
{
    ParserContext ctx;
    ParseErrorReporter::reportError(str, ctx);
}

void
ParseErrorReporter::reportError(const char *str, ParserContext &ctx)
{
    FILE *statFile;
    reportLocation(ctx);
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "%s\n", str);
        Logger::reportMessage("ParseErrorReporter", Logger::ERROR, "", _logMsg);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    exit(1);
}
