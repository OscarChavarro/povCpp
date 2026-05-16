/****************************************************************************
 *                         gifdecod.c
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

/*
    This module was freely borrowed from FRACTINT, so here is their entire
    copyright to keep them happy:
*/

/* DECODER.C - An LZW decoder for GIF
 * Copyright (C) 1987, by Steven A. Bennett
 *
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * In accordance with the above, I want to credit Steve Wilhite who wrote
 * the code which this is heavily inspired by...
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 * Release Notes: This file contains a decoder routine for GIF images
 * which is similar, structurally, to the original routine by Steve Wilhite.
 * It is, however, somewhat noticably faster in most cases.
 *
 == This routine was modified for use in FRACTINT in two ways.
 ==
 == 1) The original #includes were folded into the routine strictly to hold
 ==     down the number of files we were dealing with.
 ==
 == 2) The 'stack', 'suffix', 'prefix', and 'buf' arrays were changed from
 ==     static and 'new'ed to external only so that the assembler
 ==     program could use the same array space for several independent
 ==     chunks of code.  Also, 'stack' was renamed to 'dstack' for TASM
 ==     compatibility.
 ==
 == 3) The 'outLine()' external function has been changed to reference
 ==     '*outln()' for flexibility (in particular, 3D transformations)
 ==
 == 4) A call to 'keypressed()' has been added after the 'outln()' calls
 ==     to check for the presenc of a key-press as a bail-out signal
 ==
 == (Bert Tyler and Timothy Wegner)
*/

/*
    This routine was modified for Persistence of Vision Raytracer in the
   following ways:

    1)  Removed calls to buzzer() and keypressed() to get rid of ASM files.

    2)  The dstack, suffix, and prefix arrays were made STATIC once again.

    3)  Added the usual ANSI function prototypes, etc. in the Persistence of
   Vision Raytracer headers.
*/

#include "io/GifDecod.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "io/Gif.h"

#define LOCAL static
#define IMPORT extern

#define FAST register

/* typedef short WORD; */
typedef unsigned short UWORD;
typedef char TEXT;
typedef unsigned char UTINY;
typedef long LONG;
typedef unsigned long ULONG;
typedef int INT;

/* Various error codes used by decoder
 * and my own routines...    It's okay
 * for you to define whatever you want,
 * as long as it's negative...  It will be
 * returned intact up the various subroutine
 * levels...
 */
static constexpr int OUT_OF_MEMORY = -10;
static constexpr int BAD_CODE_SIZE = -20;
static constexpr int READ_ERROR = -1;
static constexpr int WRITE_ERROR = -2;
static constexpr int OPEN_ERROR = -3;
static constexpr int CREATE_ERROR = -4;

/* IMPORT INT getByte()
 *
 *    - This external (machine specific) function is expected to return
 * either the next byte from the GIF file, or a negative number, as
 * defined in ERRS.H.
 */
IMPORT INT getByte();

/* IMPORT INT bad_code_count;
 *
 * This value is the only other global required by the using program, and
 * is incremented each time an out of range code is read by the decoder.
 * When this value is non-zero after a decode, your GIF file is probably
 * corrupt in some way...
 */
INT badCodeCount;

static constexpr int MAX_CODES = 4095;

/* Static variables */
LOCAL WORD currSize; /* The current code size */
LOCAL WORD clear;    /* Value for a clear code */
LOCAL WORD ending;   /* Value for a ending code */
LOCAL WORD newcodes; /* First available code */
LOCAL WORD topSlot;  /* Highest code for current size */
LOCAL WORD slot;     /* Last read code */

/* The following static variables are used
 * for seperating out codes
 */
LOCAL WORD navailBytes = 0; /* # bytes left in block */
LOCAL WORD nbitsLeft = 0;   /* # bits left in current byte */
LOCAL UTINY b1;             /* Current byte */
LOCAL UTINY byteBuff[257];  /* Current block */
LOCAL UTINY *pbytes;        /* Pointer to next byte in block */

LOCAL LONG codeMask[13] = {0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F,
    0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF};

/* This function initializes the decoder for reading a new image.
 */
WORD
initExp(int iSize)
{
    WORD size;
    size = (WORD)iSize;
    currSize = size + 1;
    topSlot = 1 << currSize;
    clear = 1 << size;
    ending = clear + 1;
    slot = newcodes = ending + 1;
    navailBytes = nbitsLeft = 0;
    return (0);
}

/* getNextCode()
 * - gets the next code from the GIF file.  Returns the code, or else
 * a negative number in case of file errors...
 */
WORD
getNextCode()
{
    WORD i;
    WORD x;
    ULONG ret;

    if (nbitsLeft == 0) {
        if (navailBytes <= 0) {

            /* Out of bytes in current block, so read next block
             */
            pbytes = byteBuff;
            if ((navailBytes = getByte()) < 0) {
                return (navailBytes);
            }
            if (navailBytes) {
                for (i = 0; i < navailBytes; ++i) {
                    if ((x = getByte()) < 0) {
                        return (x);
                    }
                    byteBuff[i] = (UTINY)x;
                }
            }
        }
        b1 = *pbytes++;
        nbitsLeft = 8;
        --navailBytes;
    }

    ret = b1 >> (8 - nbitsLeft);
    while (currSize > nbitsLeft) {
        if (navailBytes <= 0) {

            /* Out of bytes in current block, so read next block
             */
            pbytes = byteBuff;
            if ((navailBytes = getByte()) < 0) {
                return (navailBytes);
            }
            if (navailBytes) {
                for (i = 0; i < navailBytes; ++i) {
                    if ((x = getByte()) < 0) {
                        return (x);
                    }
                    byteBuff[i] = (UTINY)x;
                }
            }
        }
        b1 = *pbytes++;
        ret |= b1 << nbitsLeft;
        nbitsLeft += 8;
        --navailBytes;
    }
    nbitsLeft -= currSize;
    ret &= codeMask[currSize];
    return ((WORD)(ret));
}

/* The reason we have these seperated like this instead of using
 * a structure like the original Wilhite code did, is because this
 * stuff generally produces significantly faster code when compiled...
 * This code is full of similar speedups...  (For a good book on writing
 * C for speed or for space optomisation, see Efficient C by Tom Plum,
 * published by Plum-Hall Associates...)
 */

/*
I removed the LOCAL identifiers in the arrays below and replaced them
with 'extern's so as to declare (and re-use) the space elsewhere.
The arrays are actually declared in the assembler source.
                                                                     Bert Tyler
*/

LOCAL UTINY *dstack;       /* Stack for storing pixels */
LOCAL UTINY *suffix;       /* Suffix table */
LOCAL UWORD *prefix;       /* Prefix linked list */
extern UTINY *decoderline; /* decoded line goes here */

/* WORD decoder(linewidth)
 *     WORD linewidth;                    * Pixels per line of image *
 *
 * - This function decodes an LZW image, according to the method used
 * in the GIF spec.  Every *linewidth* "characters" (ie. pixels) decoded
 * will generate a call to outLine(), which is a user specific function
 * to display a line of pixels.  The function gets its codes from
 * getNextCode() which is responsible for reading blocks of data and
 * seperating them into the proper size codes.  Finally, getByte() is
 * the global routine to read the next byte from the GIF file.
 *
 * It is generally a good idea to have linewidth correspond to the actual
 * width of a line (as specified in the Image header) to make your own
 * code a bit simpler, but it isn't absolutely necessary.
 *
 * Returns: 0 if successful, else negative.  (See ERRS.H)
 *
 */

void
cleanupGifDecoder()
{
    delete dstack;
    delete suffix;
    delete prefix;
}

WORD
decoder(int iLinewidth)
{
    WORD linewidth;
    FAST UTINY *sp, *bufptr;
    UTINY *buf;
    FAST WORD code, fc, oc, bufcnt;
    WORD c;
    WORD size;
    WORD ret;

    linewidth = (WORD)iLinewidth;

    /* Initialize for decoding a new image...
     */
    if ((size = getByte()) < 0) {
        return (size);
    }
    if (size < 2 || 9 < size) {
        return (BAD_CODE_SIZE);
    }
    initExp((int)size); /* changed param to int */

    dstack = new UTINY[MAX_CODES + 1];
    suffix = new UTINY[MAX_CODES + 1];
    prefix = new UWORD[MAX_CODES + 1];

    /* Initialize in case they forgot to put in a clear code.
     * (This shouldn't happen, but we'll try and decode it anyway...)
     */
    oc = fc = 0;

    buf = decoderline;

    badCodeCount = 0;

    /* Set up the stack pointer and decode buffer pointer
     */
    sp = dstack;
    bufptr = buf;
    bufcnt = linewidth;

    /* This is the main loop.  For each code we get we pass through the
     * linked list of prefix codes, pushing the corresponding "character" for
     * each code onto the stack.  When the list reaches a single "character"
     * we push that on the stack too, and then start unstacking each
     * character for output in the correct order.  Special handling is
     * included for the clear code, and the whole thing ends when we get
     * an ending code.
     */
    while ((c = getNextCode()) != ending) {

        /* If we had a file error, return without completing the decode
         */
        if (c < 0) {
            cleanupGifDecoder();
            return (0);
        }

        /* If the code is a clear code, reinitialize all necessary items.
         */
        if (c == clear) {
            currSize = size + 1;
            slot = newcodes;
            topSlot = 1 << currSize;

            /* Continue reading codes until we get a non-clear code
             * (Another unlikely, but possible case...)
             */
            while ((c = getNextCode()) == clear) {
                ;
            }

            /* If we get an ending code immediately after a clear code
             * (Yet another unlikely case), then break out of the loop.
             */
            if (c == ending) {
                break;
            }

            /* Finally, if the code is beyond the range of already set codes,
             * (This one had better NOT happen...  I have no idea what will
             * result from this, but I doubt it will look good...) then set it
             * to color zero.
             */
            if (c >= slot) {
                c = 0;
            }

            oc = fc = c;

            /* And let us not forget to put the char into the buffer... And
             * if, on the off chance, we were exactly one pixel from the end
             * of the line, we have to send the buffer to the outLine()
             * routine...
             */
            *bufptr++ = (UTINY)c;
            if (--bufcnt == 0) {
                COOPERATE
                if ((ret = outLine(buf, linewidth)) < 0) {
                    cleanupGifDecoder();
                    return (ret);
                }

                bufptr = buf;
                bufcnt = linewidth;
            }
        } else {

            /* In this case, it's not a clear code or an ending code, so
             * it must be a code code...  So we can now decode the code into
             * a stack of character codes. (Clear as mud, right?)
             */
            code = c;

            /* Here we go again with one of those off chances...  If, on the
             * off chance, the code we got is beyond the range of those already
             * set up (Another thing which had better NOT happen...) we trick
             * the decoder into thinking it actually got the last code read.
             * (Hmmn... I'm not sure why this works...  But it does...)
             */
            if (code >= slot) {
                if (code > slot) {
                    ++badCodeCount;
                }
                code = oc;
                *sp++ = (UTINY)fc;
            }

            /* Here we scan back along the linked list of prefixes, pushing
             * helpless characters (ie. suffixes) onto the stack as we do so.
             */
            while (code >= newcodes) {
                *sp++ = suffix[code];
                code = prefix[code];
            }

            /* Push the last character on the stack, and set up the new
             * prefix and suffix, and if the required slot number is greater
             * than that allowed by the current bit size, increase the bit
             * size.  (NOTE - If we are all full, we *don't* save the new
             * suffix and prefix...  I'm not certain if this is correct...
             * it might be more proper to overwrite the last code...
             */
            *sp++ = (UTINY)code;
            if (slot < topSlot) {
                fc = code;
                suffix[slot] = (UTINY)fc;
                prefix[slot++] = oc;
                oc = c;
            }
            if (slot >= topSlot) {
                if (currSize < 12) {
                    topSlot <<= 1;
                    ++currSize;
                }
            }

            /* Now that we've pushed the decoded string (in reverse order)
             * onto the stack, lets pop it off and put it into our decode
             * buffer...  And when the decode buffer is full, write another
             * line...
             */
            while (sp > dstack) {
                *bufptr++ = *(--sp);
                if (--bufcnt == 0) {
                    COOPERATE
                    if ((ret = outLine(buf, linewidth)) < 0) {
                        cleanupGifDecoder();
                        return (ret);
                    }
                    bufptr = buf;
                    bufcnt = linewidth;
                }
            }
        }
    }
    ret = 0;
    if (bufcnt != linewidth) {
        ret = outLine(buf, (linewidth - bufcnt));
    }

    cleanupGifDecoder();
    return (ret);
}
