#ifndef __FRAMECONFIG_H__
#define __FRAMECONFIG_H__
/****************************************************************************
 *                         frame.h
 *
 *  This header file is included by all C modules in POV-Ray. It defines all
 *  globally-accessible types and constants.
 *
 *****************************************************************************/

/* Generic header for all modules */
#include "common/Config.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "common/Color.h"

class FrameConfig {
  public:
    static inline void
    readEnvVarBefore()
    {
    }
    static inline void
    readEnvVarAfter(char *optionStringPtr)
    {
        (void)optionStringPtr;
    }
    static inline void
    configMath()
    {
    }
    static inline void
    startTime(time_t *tstart)
    {
        std::time(tstart);
    }
    static inline void
    stopTime(time_t *tstop)
    {
        std::time(tstop);
    }
    static inline void
    printOtherCredits()
    {
    }
    static inline void
    testAbort()
    {
    }
    static inline void
    finishPovray()
    {
    }
    static inline void
    cooperate()
    {
    }
    static inline double
    acosInline(double value)
    {
        return acos(value);
    }
    static inline double
    sqrtInline(double value)
    {
        return sqrt(value);
    }
    static inline double
    powInline(double base, double exp)
    {
        return pow(base, exp);
    }
    static inline double
    cosInline(double value)
    {
        return cos(value);
    }
    static inline double
    sinInline(double value)
    {
        return sin(value);
    }
    static inline long
    labsInline(long x)
    {
        return (x < 0) ? -x : x;
    }
    template <typename T>
    static inline T
    maxInline(T x, T y)
    {
        return (x < y) ? y : x;
    }
    static inline void
    printCreditsInline()
    {
    }
    static inline void
    printStatsInline()
    {
    }
    static inline void
    waitForKeypress()
    {
    }
};

// Deprecated: Use FrameConfig::readEnvVarBefore(), readEnvVarAfter(),
// configMath() instead
static inline void
readEnvVarBefore()
{
    FrameConfig::readEnvVarBefore();
}
static inline void
readEnvVarAfter(char *optionStringPtr)
{
    FrameConfig::readEnvVarAfter(optionStringPtr);
}
static inline void
configMath()
{
    FrameConfig::configMath();
}

static constexpr int FILE_NAME_LENGTH = 150;

static constexpr const char *DBL_FORMAT_STRING = "%lf";

static constexpr char DEFAULT_OUTPUT_FORMAT = 'd';

static constexpr const char *RED_RAW_FILE_EXTENSION = ".red";

static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";

static constexpr const char *BLUE_RAW_FILE_EXTENSION = ".blu";

static constexpr const char *FILENAME_SEPARATOR = "/";

/* 0==yes 1==no 2==opt */
static constexpr int CASE_SENSITIVE_DEFAULT = 0;

static constexpr const char *READ_FILE_STRING = "rb";

static constexpr const char *WRITE_FILE_STRING = "wb";

static constexpr const char *APPEND_FILE_STRING = "ab";

static constexpr char NORMAL = '0';

static constexpr char GREY = 'G';

// Deprecated: Use FrameConfig:: methods instead
static inline void
startTime(time_t *tstart)
{
    FrameConfig::startTime(tstart);
}
static inline void
stopTime(time_t *tstop)
{
    FrameConfig::stopTime(tstop);
}
static inline void
printOtherCredits()
{
    FrameConfig::printOtherCredits();
}
static inline void
testAbort()
{
    FrameConfig::testAbort();
}
static inline void
finishPovray()
{
    FrameConfig::finishPovray();
}
static inline void
cooperate()
{
    FrameConfig::cooperate();
}
static inline double
acosInline(double value)
{
    return FrameConfig::acosInline(value);
}
static inline double
sqrtInline(double value)
{
    return FrameConfig::sqrtInline(value);
}
static inline double
powInline(double base, double exp)
{
    return FrameConfig::powInline(base, exp);
}
static inline double
cosInline(double value)
{
    return FrameConfig::cosInline(value);
}
static inline double
sinInline(double value)
{
    return FrameConfig::sinInline(value);
}
inline long
labsInline(long x)
{
    return FrameConfig::labsInline(x);
}
template <typename T>
inline T
maxInline(T x, T y)
{
    return FrameConfig::maxInline(x, y);
}

static constexpr int ansiFunc = 1;

static constexpr int TRUE = 1;
static constexpr int FALSE = 0;

static inline void
printCreditsInline()
{
    FrameConfig::printCreditsInline();
}
static inline void
printStatsInline()
{
    FrameConfig::printStatsInline();
}

static constexpr int MAX_CONSTANTS = 1000;

static inline void
waitForKeypress()
{
    FrameConfig::waitForKeypress();
}

/* If compiler version is undefined, then make it 'u' for unknown */
static constexpr const char *COMPILER_VER = ".u";

/* These values determine the minumum and maximum distances
    that qualify as ray-object intersections */
static constexpr double Small_Tolerance = 0.001;
static constexpr double Max_Distance = 1.0e7;

static constexpr int MAX_ORDER = 7;

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

typedef double MATRIX[4][4];
typedef int CONSTANT;
typedef short WORD;

extern unsigned int Options;

#endif
