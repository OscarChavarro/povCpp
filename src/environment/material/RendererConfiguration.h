#ifndef __RENDERING_CONFIGURATION__
#define __RENDERING_CONFIGURATION__

#include "environment/material/RenderOutput.h"

class RenderingConfiguration {
  public:
    static constexpr int RENDER_FILE_NAME_LENGTH = 150;
    static constexpr unsigned int DISPLAY = 1u;
    static constexpr unsigned int VERBOSE = 2u;
    static constexpr unsigned int DISK_WRITE = 4u;
    static constexpr unsigned int PROMPT_EXIT = 8u;
    static constexpr unsigned int ANTIALIAS = 16u;
    static constexpr unsigned int EXIT_ENABLE = 128u;
    static constexpr unsigned int CONTINUE_TRACE = 256u;
    static constexpr unsigned int VERBOSE_FILE = 512u;
    static constexpr char DEFAULT_OUTPUT_FORMAT = 'd';

    RenderingConfiguration();

    unsigned int getOptions() const;
    void setOptionFlags(unsigned int flags);
    void clearOptionFlags(unsigned int flags);
    bool hasOptionFlags(unsigned int flags) const;
    void setOptionEnabled(unsigned int flags, bool enabled);
    int getQuality() const;
    void setQuality(int q);
    const char* getInputFileName() const;
    void setInputFileName(const char* name);
    const char* getOutputFileName() const;
    char* getOutputFileNameBuffer();
    void setOutputFileName(const char* name);
    const char* getStatFileName() const;
    void setStatFileName(const char* name);
    RenderOutput* getOutputFileInputStream() const;
    void setOutputFileInputStream(RenderOutput* out);
    int getFileBufferSize() const;
    void setFileBufferSize(int size);
    double getAntialiasThreshold() const;
    void setAntialiasThreshold(double threshold);
    int getFirstLine() const;
    void setFirstLine(int line);
    int getLastLine() const;
    void setLastLine(int line);
    char getOutputFormat() const;
    void setOutputFormat(char format);
    char getVerboseFormat() const;
    void setVerboseFormat(char format);
    int getTokenizerCaseSensitiveMode() const;
    void setTokenizerCaseSensitiveMode(int mode);
    int getTokenizerMaxSymbols() const;
    void setTokenizerMaxSymbols(int maxSymbols);
    bool hasOutputFileName() const;
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
    char outputFormat;
    char verboseFormat;
    int tokenizerCaseSensitiveMode;
    int tokenizerMaxSymbols;
};

inline unsigned int
RenderingConfiguration::getOptions() const
{
    return options;
}

inline void
RenderingConfiguration::setOptionFlags(unsigned int flags)
{
    options |= flags;
}

inline void
RenderingConfiguration::clearOptionFlags(unsigned int flags)
{
    options &= ~flags;
}

inline bool
RenderingConfiguration::hasOptionFlags(unsigned int flags) const
{
    return (options & flags) != 0;
}

inline void
RenderingConfiguration::setOptionEnabled(unsigned int flags, bool enabled)
{
    if (enabled) {
        setOptionFlags(flags);
    } else {
        clearOptionFlags(flags);
    }
}

inline int
RenderingConfiguration::getQuality() const
{
    return quality;
}

inline void
RenderingConfiguration::setQuality(int q)
{
    quality = q;
}

inline const char*
RenderingConfiguration::getInputFileName() const
{
    return inputFileName;
}

inline const char*
RenderingConfiguration::getOutputFileName() const
{
    return outputFileName;
}

inline char*
RenderingConfiguration::getOutputFileNameBuffer()
{
    return outputFileName;
}

inline const char*
RenderingConfiguration::getStatFileName() const
{
    return statFileName;
}

inline RenderOutput*
RenderingConfiguration::getOutputFileInputStream() const
{
    return outputFileInputStream;
}

inline void
RenderingConfiguration::setOutputFileInputStream(RenderOutput* out)
{
    outputFileInputStream = out;
}

inline int
RenderingConfiguration::getFileBufferSize() const
{
    return fileBufferSize;
}

inline void
RenderingConfiguration::setFileBufferSize(int size)
{
    fileBufferSize = size;
}

inline double
RenderingConfiguration::getAntialiasThreshold() const
{
    return antialiasThreshold;
}

inline void
RenderingConfiguration::setAntialiasThreshold(double threshold)
{
    antialiasThreshold = threshold;
}

inline int
RenderingConfiguration::getFirstLine() const
{
    return firstLine;
}

inline void
RenderingConfiguration::setFirstLine(int line)
{
    firstLine = line;
}

inline int
RenderingConfiguration::getLastLine() const
{
    return lastLine;
}

inline void
RenderingConfiguration::setLastLine(int line)
{
    lastLine = line;
}

inline char
RenderingConfiguration::getOutputFormat() const
{
    return outputFormat;
}

inline void
RenderingConfiguration::setOutputFormat(char format)
{
    outputFormat = format;
}

inline char
RenderingConfiguration::getVerboseFormat() const
{
    return verboseFormat;
}

inline void
RenderingConfiguration::setVerboseFormat(char format)
{
    verboseFormat = format;
}

inline int
RenderingConfiguration::getTokenizerCaseSensitiveMode() const
{
    return tokenizerCaseSensitiveMode;
}

inline void
RenderingConfiguration::setTokenizerCaseSensitiveMode(int mode)
{
    tokenizerCaseSensitiveMode = mode;
}

inline int
RenderingConfiguration::getTokenizerMaxSymbols() const
{
    return tokenizerMaxSymbols;
}

inline void
RenderingConfiguration::setTokenizerMaxSymbols(int maxSymbols)
{
    tokenizerMaxSymbols = maxSymbols;
}

inline bool
RenderingConfiguration::hasOutputFileName() const
{
    return outputFileName[0] != '\0';
}

inline
RenderingConfiguration::RenderingConfiguration()
{
    reset();
}

#endif
