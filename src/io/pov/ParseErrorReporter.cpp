#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "environment/material/RendererConfiguration.h"
#include "io/pov/Parse.h"



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

    for (i = 0; i < LAST_TOKEN; i++) {
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
    Logger::error( "Error in file %s line %d\n", ctx.token().Filename,
        ctx.token().tokenLineNo + 1);
    expected = ParseErrorReporter::getTokenString(tokenId, ctx);
    found = ParseErrorReporter::getTokenString(ctx.token().tokenId, ctx);
    Logger::error( "%s expected but %s found instead\n", expected, found);
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
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
    Logger::error( "Error in file %s line %d\n", ctx.token().Filename,
        ctx.token().tokenLineNo + 1);
    fprintf(stderr, "Identifier %s is the wrong type\n", ctx.token().Token_String);
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", ctx.token().Filename,
            ctx.token().tokenLineNo + 1);
        fprintf(statFile, "Identifier %s is the wrong type\n", ctx.token().Token_String);

        fclose(statFile);
    }

    exit(1);
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
    Logger::error( "Error in file %s line %d\n", ctx.token().Filename,
        ctx.token().tokenLineNo + 1);
    Logger::error( "Undeclared identifier %s\n", ctx.token().Token_String);
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", ctx.token().Filename,
            ctx.token().tokenLineNo + 1);
        fprintf(statFile, "Undeclared identifier %s\n", ctx.token().Token_String);
        fclose(statFile);
    }

    exit(1);
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
    Logger::error( "Error in file %s line %d\n", ctx.token().Filename,
        ctx.token().tokenLineNo + 1);
    Logger::error( "%s\n", str);

    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", ctx.token().Filename,
            ctx.token().tokenLineNo + 1);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    exit(1);
}
