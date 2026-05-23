#ifndef __RENDERING_CONFIGURATION_H__
#define __RENDERING_CONFIGURATION_H__

#include "common/LegacyBoolean.h"

class ImageFileHandle;
static constexpr int RENDER_FILE_NAME_LENGTH = 150;

static constexpr unsigned int DISPLAY = 1u;
static constexpr unsigned int VERBOSE = 2u;
static constexpr unsigned int DISKWRITE = 4u;
static constexpr unsigned int PROMPTEXIT = 8u;
static constexpr unsigned int ANTIALIAS = 16u;
static constexpr unsigned int DEBUGGING = 32u;
static constexpr unsigned int RGBSEPARATE = 64u;
static constexpr unsigned int EXITENABLE = 128u;
static constexpr unsigned int CONTINUE_TRACE = 256u;
static constexpr unsigned int VERBOSE_FILE = 512u;

static constexpr char DEFAULT_OUTPUT_FORMAT = 'd';

class RenderingConfiguration {
  public:
    unsigned int options;
    int quality;
    char inputFileName[RENDER_FILE_NAME_LENGTH];
    char outputFileName[RENDER_FILE_NAME_LENGTH];
    char statFileName[RENDER_FILE_NAME_LENGTH];
    ImageFileHandle *outputFileInputStream;
    int fileBufferSize;
    double antialiasThreshold;
    int firstLine;
    int lastLine;
    char displayFormat;
    char outputFormat;
    char verboseFormat;
    char paletteOption;
    char colorBits;

    void reset();
};

extern RenderingConfiguration globalRenderingConfiguration;

#endif
