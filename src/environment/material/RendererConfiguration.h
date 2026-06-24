#ifndef __RENDERING_CONFIGURATION__
#define __RENDERING_CONFIGURATION__

#include "environment/material/RenderOutput.h"

class RenderingConfiguration {
  private:
    static constexpr int RENDER_FILE_NAME_LENGTH = 150;

    unsigned int options;
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
    // Feature flags behind the +qN quality preset (see
    // doc/vitralNormalizationAnalysis.md §7). setQuality() sets these in bulk;
    // the direct setters below let callers select an arbitrary subset.
    static constexpr unsigned int WITH_SURFACE_LIGHTING = 4096u;
    static constexpr unsigned int WITH_SHADOWS = 8192u;
    static constexpr unsigned int WITH_TEXTURES = 16384u;
    static constexpr unsigned int WITH_FILTERED_SHADOWS = 32768u;
    static constexpr unsigned int WITH_REFRACTION = 65536u;
    static constexpr unsigned int WITH_BUMP_MAPPING = 131072u;
    static constexpr unsigned int WITH_REFLECTION = 262144u;
    static constexpr char DEFAULT_OUTPUT_FORMAT = 'd';

    // Same vocabulary and integer values as VITRAL's ShadingType enum /
    // RendererConfiguration::SHADING_TYPE_* (see
    // doc/vitralNormalizationAnalysis.md §7.3). povCpp's shading
    // pipeline only ever implements two of these five points - a fixed
    // ambient+Lambert+Blinn-Phong+Phong-highlight model when surface
    // lighting is on, and no lighting at all when it is off (the q0-1
    // preview) - so getShadingType() is a *derived* read-only view of
    // withSurfaceLighting(), not separate state: there is no shader in this
    // codebase that implements FLAT, GOURAUD or COOK_TERRANCE, so exposing a
    // setter for them would claim support that does not exist. The point of
    // this enum is solely to let povCpp's q0-1 "no lighting" preview and
    // VITRAL's SHADING_NOLIGHT be recognised as the same concept.
    static constexpr int SHADING_TYPE_NOLIGHT = 0;
    static constexpr int SHADING_TYPE_FLAT = 1;
    static constexpr int SHADING_TYPE_GOURAUD = 2;
    static constexpr int SHADING_TYPE_PHONG = 3;
    static constexpr int SHADING_TYPE_COOK_TERRANCE = 4;

    RenderingConfiguration();

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
    int getShadingType() const;
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
    int getNumberOfThreads() const;
    void setNumberOfThreads(int n);
    bool hasOutputFileName() const;
    void reset();
};

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

inline void
RenderingConfiguration::setQuality(int q)
{
    // Preset over the feature flags below (doc/vitralNormalizationAnalysis.md
    // §7.2): each of the five bands {0,1} {2,3} {4,5} {6,7} {8,9} reproduces
    // today's images bit-for-bit. The flags are the only stored state; the
    // quality integer is not retained (nothing reads it back - the shaders
    // query the predicates, see §7 of doc/vitralNormalizationAnalysis.md).
    setSurfaceLightingEnabled(q > 1);
    setShadowsEnabled(q > 3);
    setTexturesEnabled(q > 5);
    setFilteredShadowsEnabled(q > 5);
    setRefractionEnabled(q > 5);
    setBumpMappingEnabled(q >= 8);
    setReflectionEnabled(q >= 8);
}

inline bool
RenderingConfiguration::withSurfaceLighting() const
{
    return hasOptionFlags(WITH_SURFACE_LIGHTING);
}

inline void
RenderingConfiguration::setSurfaceLightingEnabled(bool enabled)
{
    setOptionEnabled(WITH_SURFACE_LIGHTING, enabled);
}

inline bool
RenderingConfiguration::withShadows() const
{
    return hasOptionFlags(WITH_SHADOWS);
}

inline void
RenderingConfiguration::setShadowsEnabled(bool enabled)
{
    setOptionEnabled(WITH_SHADOWS, enabled);
}

inline bool
RenderingConfiguration::withTextures() const
{
    return hasOptionFlags(WITH_TEXTURES);
}

inline void
RenderingConfiguration::setTexturesEnabled(bool enabled)
{
    setOptionEnabled(WITH_TEXTURES, enabled);
}

inline bool
RenderingConfiguration::withFilteredShadows() const
{
    return hasOptionFlags(WITH_FILTERED_SHADOWS);
}

inline void
RenderingConfiguration::setFilteredShadowsEnabled(bool enabled)
{
    setOptionEnabled(WITH_FILTERED_SHADOWS, enabled);
}

inline bool
RenderingConfiguration::withRefraction() const
{
    return hasOptionFlags(WITH_REFRACTION);
}

inline void
RenderingConfiguration::setRefractionEnabled(bool enabled)
{
    setOptionEnabled(WITH_REFRACTION, enabled);
}

inline bool
RenderingConfiguration::withBumpMapping() const
{
    return hasOptionFlags(WITH_BUMP_MAPPING);
}

inline void
RenderingConfiguration::setBumpMappingEnabled(bool enabled)
{
    setOptionEnabled(WITH_BUMP_MAPPING, enabled);
}

inline bool
RenderingConfiguration::withReflection() const
{
    return hasOptionFlags(WITH_REFLECTION);
}

inline void
RenderingConfiguration::setReflectionEnabled(bool enabled)
{
    setOptionEnabled(WITH_REFLECTION, enabled);
}

inline int
RenderingConfiguration::getShadingType() const
{
    return withSurfaceLighting() ? SHADING_TYPE_PHONG : SHADING_TYPE_NOLIGHT;
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

inline int
RenderingConfiguration::getNumberOfThreads() const
{
    return numberOfThreads;
}

inline void
RenderingConfiguration::setNumberOfThreads(int n)
{
    numberOfThreads = n;
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
