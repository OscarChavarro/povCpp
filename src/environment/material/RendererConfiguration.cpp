#include <cstring>

#include "environment/material/RendererConfiguration.h"

RenderingConfiguration *RenderingConfiguration::sActive = nullptr;

void
RenderingConfiguration::reset()
{
    options = 0;
    quality = 9;
    inputFileName[0] = '\0';
    outputFileName[0] = '\0';
    statFileName[0] = '\0';
    outputFileInputStream = nullptr;
    fileBufferSize = 0;
    antialiasThreshold = 0.3;
    firstLine = 0;
    lastLine = -1;
    displayFormat = '\0';
    outputFormat = '\0';
    verboseFormat = '\0';
    paletteOption = '\0';
    colorBits = 8;
    std::strcpy(inputFileName, "object.dat");
}
