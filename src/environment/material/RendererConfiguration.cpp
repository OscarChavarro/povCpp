#include <cstring>

#include "environment/material/RendererConfiguration.h"

void
RenderingConfiguration::setInputFileName(const char* name)
{
    strncpy(inputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    inputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
RenderingConfiguration::setOutputFileName(const char* name)
{
    strncpy(outputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    outputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
RenderingConfiguration::setStatFileName(const char* name)
{
    strncpy(statFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    statFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
RenderingConfiguration::reset()
{
    options = 0;
    setQuality(9);
    inputFileName[0] = '\0';
    outputFileName[0] = '\0';
    statFileName[0] = '\0';
    outputFileInputStream = nullptr;
    fileBufferSize = 0;
    antialiasThreshold = 0.3;
    firstLine = 0;
    lastLine = -1;
    outputFormat = '\0';
    verboseFormat = '\0';
    tokenizerCaseSensitiveMode = 0;
    tokenizerMaxSymbols = 500;
    numberOfThreads = 1;
    std::strcpy(inputFileName, "object.dat");
}
