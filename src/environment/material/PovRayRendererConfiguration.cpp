#include <cstring>

#include "environment/material/PovRayRendererConfiguration.h"

void
PovRayRendererConfiguration::setInputFileName(const char* name)
{
    strncpy(inputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    inputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
PovRayRendererConfiguration::setOutputFileName(const char* name)
{
    strncpy(outputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    outputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
PovRayRendererConfiguration::setStatFileName(const char* name)
{
    strncpy(statFileName, name, RENDER_FILE_NAME_LENGTH - 1);
    statFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
}

void
PovRayRendererConfiguration::reset()
{
    options = 0x00;
    setOptionFlags(WITH_SURFACE_LIGHTING | WITH_SHADOWS | WITH_TEXTURES |
        WITH_FILTERED_SHADOWS | WITH_REFRACTION | WITH_BUMP_MAPPING |
        WITH_REFLECTION);
    RendererConfiguration::setShadingType(SHADING_TYPE_PHONG);
    inputFileName[0] = '\0';
    outputFileName[0] = '\0';
    statFileName[0] = '\0';
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
