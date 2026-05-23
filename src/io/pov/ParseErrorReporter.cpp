#include "common/LegacyBoolean.h"
#include "environment/material/RendererConfiguration.h"
#include "io/pov/Parse.h"

extern ReservedWord globalReservedWords[];
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;

char *
ParseErrorReporter::getTokenString(TOKEN tokenId)
{
    int i;

    for (i = 0; i < LAST_TOKEN; i++) {
        if (globalReservedWords[i].Token_Number == tokenId) {
            return (char *)(globalReservedWords[i].Token_Name);
        }
    }
    return (char *)("");
}

void
ParseErrorReporter::parseError(TOKEN tokenId)
{
    char *expected;
    char *found;
    FILE *statFile;
    Logger::error( "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    expected = ParseErrorReporter::getTokenString(tokenId);
    found = ParseErrorReporter::getTokenString(globalToken.Token_Id);
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
    FILE *statFile;
    Logger::error( "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    fprintf(
        stderr, "Identifier %s is the wrong type\n", globalToken.Token_String);
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "Identifier %s is the wrong type\n",
            globalToken.Token_String);

        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::Undeclared()
{
    FILE *statFile;
    Logger::error( "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    Logger::error( "Undeclared identifier %s\n", globalToken.Token_String);
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(
            statFile, "Undeclared identifier %s\n", globalToken.Token_String);
        fclose(statFile);
    }

    exit(1);
}

void
ParseErrorReporter::Error(const char *str)
{
    FILE *statFile;
    Logger::error( "Error in file %s line %d\n", globalToken.Filename,
        globalToken.Token_Line_No + 1);
    Logger::error( "%s\n", str);

    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Error in file %s line %d\n", globalToken.Filename,
            globalToken.Token_Line_No + 1);
        fprintf(statFile, "%s\n", str);
        fclose(statFile);
    }
    exit(1);
}
