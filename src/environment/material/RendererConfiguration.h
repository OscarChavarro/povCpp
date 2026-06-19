#ifndef __RENDERING_CONFIGURATION__
#define __RENDERING_CONFIGURATION__

#include <cstring>

#include "environment/material/RenderOutput.h"

class RenderingConfiguration {
  public:
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

    unsigned int getOptions() const { return options; }
    void setOptions(unsigned int opts) { options = opts; }
    void setOptionFlags(unsigned int flags) { options |= flags; }
    void clearOptionFlags(unsigned int flags) { options &= ~flags; }
    bool hasOptionFlags(unsigned int flags) const { return (options & flags) != 0; }
    void setOptionEnabled(unsigned int flags, bool enabled) {
        if (enabled) {
            setOptionFlags(flags);
        } else {
            clearOptionFlags(flags);
        }
    }
    int getQuality() const { return quality; }
    void setQuality(int q) { quality = q; }
    const char* getInputFileName() const { return inputFileName; }
    char* getInputFileNameBuffer() { return inputFileName; }
    void setInputFileName(const char* name) {
        strncpy(inputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
        inputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
    }
    const char* getOutputFileName() const { return outputFileName; }
    char* getOutputFileNameBuffer() { return outputFileName; }
    void setOutputFileName(const char* name) {
        strncpy(outputFileName, name, RENDER_FILE_NAME_LENGTH - 1);
        outputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
    }
    const char* getStatFileName() const { return statFileName; }
    char* getStatFileNameBuffer() { return statFileName; }
    void setStatFileName(const char* name) {
        strncpy(statFileName, name, RENDER_FILE_NAME_LENGTH - 1);
        statFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
    }
    RenderOutput* getOutputFileInputStream() const { return outputFileInputStream; }
    void setOutputFileInputStream(RenderOutput* out) { outputFileInputStream = out; }
    int getFileBufferSize() const { return fileBufferSize; }
    void setFileBufferSize(int size) { fileBufferSize = size; }
    double getAntialiasThreshold() const { return antialiasThreshold; }
    void setAntialiasThreshold(double threshold) { antialiasThreshold = threshold; }
    int getFirstLine() const { return firstLine; }
    void setFirstLine(int line) { firstLine = line; }
    int getLastLine() const { return lastLine; }
    void setLastLine(int line) { lastLine = line; }
    char getDisplayFormat() const { return displayFormat; }
    void setDisplayFormat(char format) { displayFormat = format; }
    char getOutputFormat() const { return outputFormat; }
    void setOutputFormat(char format) { outputFormat = format; }
    char getVerboseFormat() const { return verboseFormat; }
    void setVerboseFormat(char format) { verboseFormat = format; }
    char getPaletteOption() const { return paletteOption; }
    void setPaletteOption(char option) { paletteOption = option; }
    char getColorBits() const { return colorBits; }
    void setColorBits(char bits) { colorBits = bits; }
    int getTokenizerCaseSensitiveMode() const { return tokenizerCaseSensitiveMode; }
    void setTokenizerCaseSensitiveMode(int mode) { tokenizerCaseSensitiveMode = mode; }
    int getTokenizerMaxSymbols() const { return tokenizerMaxSymbols; }
    void setTokenizerMaxSymbols(int maxSymbols) { tokenizerMaxSymbols = maxSymbols; }
    bool hasOutputFileName() const { return outputFileName[0] != '\0'; }

    RenderingConfiguration() { reset(); }
    void reset();

  private:
    unsigned int options;
    int quality;
    char inputFileName[RENDER_FILE_NAME_LENGTH];
    char outputFileName[RENDER_FILE_NAME_LENGTH];
    char statFileName[RENDER_FILE_NAME_LENGTH];
    RenderOutput *outputFileInputStream;
    int fileBufferSize;
    double antialiasThreshold;
    int firstLine;
    int lastLine;
    char displayFormat;
    char outputFormat;
    char verboseFormat;
    char paletteOption;
    char colorBits;
    int tokenizerCaseSensitiveMode;
    int tokenizerMaxSymbols;
};

#endif
