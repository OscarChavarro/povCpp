#include "io/pov/ParserContext.h"
#include "common/logger/Logger.h"
#include "environment/material/RendererConfiguration.h"
#include "io/pov/ParseErrorReporter.h"

namespace {
void reportLocation(ParserContext &ctx)
{
    const char *file = (ctx.token().Filename != nullptr) ? ctx.token().Filename : "<unknown>";
    const int line = ctx.token().tokenLineNo + 1;
    const int column = (ctx.token().tokenColumnNo > 0) ? ctx.token().tokenColumnNo : 1;
    Logger::error("Error at %s:%d:%d\n", file, line, column);
}

void writeVerboseStatLine(FILE *statFile, ParserContext &ctx)
{
    const char *file = (ctx.token().Filename != nullptr) ? ctx.token().Filename : "<unknown>";
    const int line = ctx.token().tokenLineNo + 1;
    const int column = (ctx.token().tokenColumnNo > 0) ? ctx.token().tokenColumnNo : 1;
    fprintf(statFile, "Error at %s:%d:%d\n", file, line, column);
}
}

char *
ParseErrorReporter::getTokenString(TOKEN tokenId)
{
    ParserContext ctx;
    return ParseErrorReporter::getTokenString(tokenId, ctx);
}

char *
ParseErrorReporter::getTokenString(TOKEN tokenId, ParserContext &ctx)
{
    int i;

    for (i = 0; i < Tokenizer::LAST_TOKEN; i++) {
        if (ctx.reservedWords()[i].tokenNumber == tokenId) {
            return (char *)(ctx.reservedWords()[i].Token_Name);
        }
    }
    return (char *)("");
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
    char *expected;
    char *found;
    FILE *statFile;
    reportLocation(ctx);
    expected = ParseErrorReporter::getTokenString(tokenId, ctx);
    found = ParseErrorReporter::getTokenString(ctx.token().tokenId, ctx);
    Logger::error( "%s expected but %s found instead\n", expected, found);
    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(
            statFile, "%s expected but %s found instead\n", expected, found);
        fclose(statFile);
    }

    throw ParseException("Parse error");
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

    throw ParseException("Type error");
}

void
ParseErrorReporter::Undeclared()
{
    ParserContext ctx;
    ParseErrorReporter::Undeclared(ctx);
}

void
ParseErrorReporter::Undeclared(ParserContext &ctx)
{
    FILE *statFile;
    reportLocation(ctx);
    Logger::error( "Undeclared identifier %s\n", ctx.token().Token_String);
    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(statFile, "Undeclared identifier %s\n", ctx.token().Token_String);
        fclose(statFile);
    }

    throw ParseException("Undeclared identifier");
}

void
ParseErrorReporter::Error(const char *str)
{
    ParserContext ctx;
    ParseErrorReporter::Error(str, ctx);
}

void
ParseErrorReporter::Error(const char *str, ParserContext &ctx)
{
    FILE *statFile;
    reportLocation(ctx);
    Logger::error( "%s\n", str);

    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        statFile = fopen(RenderingConfiguration::global().statFileName, "w+t");
        writeVerboseStatLine(statFile, ctx);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    throw ParseException(str);
}
