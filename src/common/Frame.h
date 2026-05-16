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
#include <cstdio>
#include <cstring>

#ifndef DBL
#define DBL double
#endif

#include "common/Colour.h"

#ifndef READ_ENV_VAR_BEFORE
#define READ_ENV_VAR_BEFORE
#endif
#ifndef READ_ENV_VAR_AFTER
#define READ_ENV_VAR_AFTER                                                     \
    if ((Option_String_Ptr = getenv("POVRAYOPT")) != NULL)                     \
        readOptions(Option_String_Ptr);
#endif

#ifndef CONFIG_MATH
#define CONFIG_MATH
#endif

#ifndef EPSILON
#define EPSILON 1.0e-10
#endif

static constexpr int FILE_NAME_LENGTH = 150;

#ifndef HUGE_VAL
#define HUGE_VAL 1.0e+17
#endif

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

#ifndef START_TIME
#define START_TIME time(&tstart);
#endif

#ifndef STOP_TIME
#define STOP_TIME time(&tstop);
#endif

#ifndef TIME_ELAPSED
#define TIME_ELAPSED difftime(tstop, tstart);
#endif

#ifndef STARTUP_POVRAY
#define STARTUP_POVRAY
#endif

#ifndef PRINT_OTHER_CREDITS
#define PRINT_OTHER_CREDITS
#endif

#ifndef TEST_ABORT
#define TEST_ABORT
#endif

#ifndef FINISH_POVRAY
#define FINISH_POVRAY
#endif

#ifndef COOPERATE
#define COOPERATE
#endif

#ifndef ACOS
#define ACOS acos
#endif

#ifndef SQRT
#define SQRT sqrt
#endif

#ifndef POW
#define POW pow
#endif

#ifndef COS
#define COS cos
#endif

#ifndef SIN
#define SIN sin
#endif

#ifndef labs
#define labs(x) (long)((x < 0) ? -x : x)
#endif

#ifndef max
#define max(x, y) ((x < y) ? y : x)
#endif

#ifndef STRLN
#define STRLN(x) x
#endif

#ifndef PARAMS
#define PARAMS(x) x
#endif

#ifndef ANSIFUNC
#define ANSIFUNC 1
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

static constexpr int TRUE = 1;
static constexpr int FALSE = 0;

#ifndef IFF_SWITCH_CAST
#define IFF_SWITCH_CAST (int)
#endif

#ifndef PRINT_CREDITS
#define PRINT_CREDITS printCredits();
#endif

#ifndef PRINT_STATS
#define PRINT_STATS printStats();
#endif

static constexpr int MAX_CONSTANTS = 1000;

#ifndef WAIT_FOR_KEYPRESS
#define WAIT_FOR_KEYPRESS
#endif

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
