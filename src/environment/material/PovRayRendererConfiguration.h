#ifndef __POV_RAY_RENDERER_CONFIGURATION__
#define __POV_RAY_RENDERER_CONFIGURATION__

#include "vsdk/toolkit/environment/material/RendererConfiguration.h"

class PovRayRendererConfiguration : public RendererConfiguration {
  private:
    static constexpr int RENDER_FILE_NAME_LENGTH = 150;

    unsigned int options;
    char inputFileName[RENDER_FILE_NAME_LENGTH];
    char outputFileName[RENDER_FILE_NAME_LENGTH];
    char statFileName[RENDER_FILE_NAME_LENGTH];
    int fileBufferSize;
    double antialiasThreshold;
    int firstLine;
    int lastLine;
    char outputFormat;
    char verboseFormat;
    int tokenizerCaseSensitiveMode;
    int tokenizerMaxSymbols;
    int numberOfThreads;

  public:
    static constexpr unsigned int DISPLAY = 1u;
    static constexpr unsigned int VERBOSE = 2u;
    static constexpr unsigned int DISK_WRITE = 4u;
    static constexpr unsigned int PROMPT_EXIT = 8u;
    static constexpr unsigned int ANTIALIAS = 16u;
    static constexpr unsigned int EXIT_ENABLE = 128u;
    static constexpr unsigned int CONTINUE_TRACE = 256u;
    static constexpr unsigned int VERBOSE_FILE = 512u;
    static constexpr unsigned int PARALLEL = 1024u;
    static constexpr unsigned int CSG_ROTH = 2048u;
    static constexpr unsigned int WITH_SURFACE_LIGHTING = 4096u;
    static constexpr unsigned int WITH_SHADOWS = 8192u;
    static constexpr unsigned int WITH_TEXTURES = 16384u;
    static constexpr unsigned int WITH_FILTERED_SHADOWS = 32768u;
    static constexpr unsigned int WITH_REFRACTION = 65536u;
    static constexpr unsigned int WITH_BUMP_MAPPING = 131072u;
    static constexpr unsigned int WITH_REFLECTION = 262144u;
    static constexpr char DEFAULT_OUTPUT_FORMAT = 'd';

    PovRayRendererConfiguration();

    void setOptionFlags(unsigned int flags);
    void clearOptionFlags(unsigned int flags);
    bool hasOptionFlags(unsigned int flags) const;
    void setOptionEnabled(unsigned int flags, bool enabled);
    void setQuality(int q);
    bool withSurfaceLighting() const;
    void setSurfaceLightingEnabled(bool enabled);
    bool withShadows() const;
    void setShadowsEnabled(bool enabled);
    bool withTextures() const;
    void setTexturesEnabled(bool enabled);
    bool withFilteredShadows() const;
    void setFilteredShadowsEnabled(bool enabled);
    bool withRefraction() const;
    void setRefractionEnabled(bool enabled);
    bool withBumpMapping() const;
    void setBumpMappingEnabled(bool enabled);
    bool withReflection() const;
    void setReflectionEnabled(bool enabled);
    const char* getInputFileName() const;
    void setInputFileName(const char* name);
    const char* getOutputFileName() const;
    char* getOutputFileNameBuffer();
    void setOutputFileName(const char* name);
    const char* getStatFileName() const;
    void setStatFileName(const char* name);
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
    int getNumberOfThreads() const;
    void setNumberOfThreads(int n);
    bool hasOutputFileName() const;
    void reset();
};

inline void
PovRayRendererConfiguration::setOptionFlags(unsigned int flags)
{
    options |= flags;
}

inline void
PovRayRendererConfiguration::clearOptionFlags(unsigned int flags)
{
    options &= ~flags;
}

inline bool
PovRayRendererConfiguration::hasOptionFlags(unsigned int flags) const
{
    return (options & flags) != 0;
}

inline void
PovRayRendererConfiguration::setOptionEnabled(unsigned int flags, bool enabled)
{
    if (enabled) {
        setOptionFlags(flags);
    } else {
        clearOptionFlags(flags);
    }
}

inline void
PovRayRendererConfiguration::setQuality(int q)
{
    setSurfaceLightingEnabled(q > 1);
    setShadowsEnabled(q > 3);
    setTexturesEnabled(q > 5);
    setFilteredShadowsEnabled(q > 5);
    setRefractionEnabled(q > 5);
    setBumpMappingEnabled(q >= 8);
    setReflectionEnabled(q >= 8);
}

inline bool
PovRayRendererConfiguration::withSurfaceLighting() const
{
    return hasOptionFlags(WITH_SURFACE_LIGHTING);
}

inline void
PovRayRendererConfiguration::setSurfaceLightingEnabled(bool enabled)
{
    setOptionEnabled(WITH_SURFACE_LIGHTING, enabled);
    RendererConfiguration::setShadingType(enabled ? SHADING_TYPE_PHONG : SHADING_TYPE_NOLIGHT);
}

inline bool
PovRayRendererConfiguration::withShadows() const
{
    return hasOptionFlags(WITH_SHADOWS);
}

inline void
PovRayRendererConfiguration::setShadowsEnabled(bool enabled)
{
    setOptionEnabled(WITH_SHADOWS, enabled);
}

inline bool
PovRayRendererConfiguration::withTextures() const
{
    return hasOptionFlags(WITH_TEXTURES);
}

inline void
PovRayRendererConfiguration::setTexturesEnabled(bool enabled)
{
    setOptionEnabled(WITH_TEXTURES, enabled);
}

inline bool
PovRayRendererConfiguration::withFilteredShadows() const
{
    return hasOptionFlags(WITH_FILTERED_SHADOWS);
}

inline void
PovRayRendererConfiguration::setFilteredShadowsEnabled(bool enabled)
{
    setOptionEnabled(WITH_FILTERED_SHADOWS, enabled);
}

inline bool
PovRayRendererConfiguration::withRefraction() const
{
    return hasOptionFlags(WITH_REFRACTION);
}

inline void
PovRayRendererConfiguration::setRefractionEnabled(bool enabled)
{
    setOptionEnabled(WITH_REFRACTION, enabled);
}

inline bool
PovRayRendererConfiguration::withBumpMapping() const
{
    return hasOptionFlags(WITH_BUMP_MAPPING);
}

inline void
PovRayRendererConfiguration::setBumpMappingEnabled(bool enabled)
{
    setOptionEnabled(WITH_BUMP_MAPPING, enabled);
}

inline bool
PovRayRendererConfiguration::withReflection() const
{
    return hasOptionFlags(WITH_REFLECTION);
}

inline void
PovRayRendererConfiguration::setReflectionEnabled(bool enabled)
{
    setOptionEnabled(WITH_REFLECTION, enabled);
}

inline const char*
PovRayRendererConfiguration::getInputFileName() const
{
    return inputFileName;
}

inline const char*
PovRayRendererConfiguration::getOutputFileName() const
{
    return outputFileName;
}

inline char*
PovRayRendererConfiguration::getOutputFileNameBuffer()
{
    return outputFileName;
}

inline const char*
PovRayRendererConfiguration::getStatFileName() const
{
    return statFileName;
}

inline int
PovRayRendererConfiguration::getFileBufferSize() const
{
    return fileBufferSize;
}

inline void
PovRayRendererConfiguration::setFileBufferSize(int size)
{
    fileBufferSize = size;
}

inline double
PovRayRendererConfiguration::getAntialiasThreshold() const
{
    return antialiasThreshold;
}

inline void
PovRayRendererConfiguration::setAntialiasThreshold(double threshold)
{
    antialiasThreshold = threshold;
}

inline int
PovRayRendererConfiguration::getFirstLine() const
{
    return firstLine;
}

inline void
PovRayRendererConfiguration::setFirstLine(int line)
{
    firstLine = line;
}

inline int
PovRayRendererConfiguration::getLastLine() const
{
    return lastLine;
}

inline void
PovRayRendererConfiguration::setLastLine(int line)
{
    lastLine = line;
}

inline char
PovRayRendererConfiguration::getOutputFormat() const
{
    return outputFormat;
}

inline void
PovRayRendererConfiguration::setOutputFormat(char format)
{
    outputFormat = format;
}

inline char
PovRayRendererConfiguration::getVerboseFormat() const
{
    return verboseFormat;
}

inline void
PovRayRendererConfiguration::setVerboseFormat(char format)
{
    verboseFormat = format;
}

inline int
PovRayRendererConfiguration::getTokenizerCaseSensitiveMode() const
{
    return tokenizerCaseSensitiveMode;
}

inline void
PovRayRendererConfiguration::setTokenizerCaseSensitiveMode(int mode)
{
    tokenizerCaseSensitiveMode = mode;
}

inline int
PovRayRendererConfiguration::getTokenizerMaxSymbols() const
{
    return tokenizerMaxSymbols;
}

inline void
PovRayRendererConfiguration::setTokenizerMaxSymbols(int maxSymbols)
{
    tokenizerMaxSymbols = maxSymbols;
}

inline int
PovRayRendererConfiguration::getNumberOfThreads() const
{
    return numberOfThreads;
}

inline void
PovRayRendererConfiguration::setNumberOfThreads(int n)
{
    numberOfThreads = n;
}

inline bool
PovRayRendererConfiguration::hasOutputFileName() const
{
    return outputFileName[0] != '\0';
}

inline
PovRayRendererConfiguration::PovRayRendererConfiguration()
{
    reset();
}

#endif
