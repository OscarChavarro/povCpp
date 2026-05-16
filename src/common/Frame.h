#ifndef __FRAME_H__
#define __FRAME_H__
/****************************************************************************
 *                         frame.h
 *
 *  This header file is included by all C modules in POV-Ray. It defines all
 *  globally-accessible types and constants.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

/* Generic header for all modules */
#include "common/Config.h"
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstring>

#include "common/Colour.h"

static inline void readEnvVarBefore() {}

static inline void readEnvVarAfter(char *optionStringPtr)
{
    (void)optionStringPtr;
}

static inline void configMath() {}

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

static inline void startTime(time_t *tstart) { std::time(tstart); }

static inline void stopTime(time_t *tstop) { std::time(tstop); }
static inline void printOtherCredits() {}

static inline void testAbort() {}

static inline void finishPovray() {}

static inline void cooperate() {}

static inline DBL acosInline(DBL value) { return acos(value); }

static inline DBL sqrtInline(DBL value) { return sqrt(value); }

static inline DBL powInline(DBL base, DBL exp) { return pow(base, exp); }

static inline DBL cosInline(DBL value) { return cos(value); }

static inline DBL sinInline(DBL value) { return sin(value); }

inline long labsInline(long x)
{
    return (x < 0) ? -x : x;
}

template <typename T>
inline T maxInline(T x, T y)
{
    return (x < y) ? y : x;
}

static constexpr int ansiFunc = 1;

static constexpr int TRUE = 1;
static constexpr int FALSE = 0;

static inline void printCreditsInline() {}

static inline void printStatsInline() {}

static constexpr int MAX_CONSTANTS = 1000;

static inline void waitForKeypress() {}

/* If compiler version is undefined, then make it 'u' for unknown */
static constexpr const char *COMPILER_VER = ".u";

/* These values determine the minumum and maximum distances
    that qualify as ray-object intersections */
static constexpr DBL Small_Tolerance = 0.001;
static constexpr DBL Max_Distance = 1.0e7;

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

typedef DBL MATRIX[4][4];
typedef int CONSTANT;
typedef short WORD;

extern unsigned int Options;

#endif
