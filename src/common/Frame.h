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
#include <math.h>
#include <stdio.h>
#include <string.h>

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
        read_options(Option_String_Ptr);
#endif

#ifndef CONFIG_MATH
#define CONFIG_MATH
#endif

#ifndef EPSILON
#define EPSILON 1.0e-10
#endif

#ifndef FILE_NAME_LENGTH
#define FILE_NAME_LENGTH 150
#endif

#ifndef HUGE_VAL
#define HUGE_VAL 1.0e+17
#endif

#ifndef DBL_FORMAT_STRING
#define DBL_FORMAT_STRING "%lf"
#endif

#ifndef DEFAULT_OUTPUT_FORMAT
#define DEFAULT_OUTPUT_FORMAT 'd'
#endif

#ifndef RED_RAW_FILE_EXTENSION
#define RED_RAW_FILE_EXTENSION ".red"
#endif

#ifndef GREEN_RAW_FILE_EXTENSION
#define GREEN_RAW_FILE_EXTENSION ".grn"
#endif

#ifndef BLUE_RAW_FILE_EXTENSION
#define BLUE_RAW_FILE_EXTENSION ".blu"
#endif

#ifndef FILENAME_SEPARATOR
#define FILENAME_SEPARATOR "/"
#endif

/* 0==yes 1==no 2==opt */
#ifndef CASE_SENSITIVE_DEFAULT
#define CASE_SENSITIVE_DEFAULT 0
#endif

#ifndef READ_FILE_STRING
#define READ_FILE_STRING "rb"
#endif

#ifndef WRITE_FILE_STRING
#define WRITE_FILE_STRING "wb"
#endif

#ifndef APPEND_FILE_STRING
#define APPEND_FILE_STRING "ab"
#endif

#ifndef NORMAL
#define NORMAL '0'
#endif

#ifndef GREY
#define GREY 'G'
#endif

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

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef IFF_SWITCH_CAST
#define IFF_SWITCH_CAST (int)
#endif

#ifndef PRINT_CREDITS
#define PRINT_CREDITS print_credits();
#endif

#ifndef PRINT_STATS
#define PRINT_STATS print_stats();
#endif

#ifndef MAX_CONSTANTS
#define MAX_CONSTANTS 1000
#endif

#ifndef WAIT_FOR_KEYPRESS
#define WAIT_FOR_KEYPRESS
#endif

/* If compiler version is undefined, then make it 'u' for unknown */
#ifndef COMPILER_VER
#define COMPILER_VER ".u"
#endif

/* These values determine the minumum and maximum distances
    that qualify as ray-object intersections */
#define Small_Tolerance 0.001
#define Max_Distance 1.0e7

#define MAX_ORDER 7

#define DISPLAY 1
#define VERBOSE 2
#define DISKWRITE 4
#define PROMPTEXIT 8
#define ANTIALIAS 16
#define DEBUGGING 32
#define RGBSEPARATE 64
#define EXITENABLE 128
#define CONTINUE_TRACE 256
#define VERBOSE_FILE 512

typedef DBL MATRIX[4][4];
typedef int CONSTANT;
typedef short WORD;

extern unsigned int Options;

#endif
