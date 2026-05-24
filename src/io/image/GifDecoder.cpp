#include "io/image/GifDecoder.h"
#include "common/LegacyBoolean.h"
#include "io/image/GifFormat.h"

typedef unsigned short UWORD;
typedef char TEXT;
typedef unsigned char UTINY;
typedef long LONG;
typedef unsigned long ULONG;
typedef int INT;

unsigned char *GifDecoder::decoderline = nullptr;

static constexpr int BAD_CODE_SIZE = -20;

INT badCodeCount;

static constexpr int MAX_CODES = 4095;

static WORD currSize; // The current code size
static WORD clear;    // Value for a clear code
static WORD ending;   // Value for a ending code
static WORD newcodes; // First available code
static WORD topSlot;  // Highest code for current size
static WORD slot;     // Last read code

static WORD navailBytes = 0; // # bytes left in block
static WORD nbitsLeft = 0;   // # bits left in current byte
static UTINY b1;             // Current byte
static UTINY byteBuff[257];  // Current block
static UTINY *pbytes;        // Pointer to next byte in block

static LONG codeMask[13] = {0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F,
    0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF};

// This function initializes the decoder for reading a new image.
WORD
GifDecoder::initExp(int iSize)
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

// GifDecoder::getNextCode()
// - gets the next code from the GIF file.  Returns the code, or else
// a negative number in case of file errors...
WORD
GifDecoder::getNextCode()
{
    WORD i;
    WORD x;
    ULONG ret;

    if (nbitsLeft == 0) {
        if (navailBytes <= 0) {

            // Out of bytes in current block, so read next block
            pbytes = byteBuff;
            if ((navailBytes = GifFormat::getByte()) < 0) {
                return (navailBytes);
            }
            if (navailBytes) {
                for (i = 0; i < navailBytes; ++i) {
                    if ((x = GifFormat::getByte()) < 0) {
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

            // Out of bytes in current block, so read next block
            pbytes = byteBuff;
            if ((navailBytes = GifFormat::getByte()) < 0) {
                return (navailBytes);
            }
            if (navailBytes) {
                for (i = 0; i < navailBytes; ++i) {
                    if ((x = GifFormat::getByte()) < 0) {
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

// The reason we have these separated like this instead of using
// a structure like the original Wilhite code did, is because this
// stuff generally produces significantly faster code when compiled...
// This code is full of similar speedups...  (For a good book on writing
// C for speed or for space optimization, see Efficient C by Tom Plum,
// published by Plum-Hall Associates...)
//
// Decoder work buffers are held as file statics and class static members.

static UTINY *dstack; // Stack for storing pixels
static UTINY *suffix; // Suffix table
static UWORD *prefix; // Prefix linked list

// WORD decoder(linewidth)
//     WORD linewidth;                    * Pixels per line of image *
//
// - This function decodes an LZW image, according to the method used
// in the GIF spec.  Every *linewidth* "characters" (ie. pixels) decoded
// will generate a call to GifFormat::outLine(), which is a user specific
// function to display a line of pixels.  The function gets its codes from
// GifDecoder::getNextCode() which is responsible for reading blocks of data and
// separating them into the proper size codes.  Finally, GifFormat::getByte() is
// the global routine to read the next byte from the GIF file.
//
// It is generally a good idea to have linewidth correspond to the actual
// width of a line (as specified in the Image header) to make your own
// code a bit simpler, but it isn't absolutely necessary.
//
// Returns: 0 if successful, else negative.  (See ERRS.H)

void
GifDecoder::cleanupGifDecoder()
{
    delete dstack;
    delete suffix;
    delete prefix;
}

WORD
GifDecoder::decoder(int iLinewidth)
{
    WORD linewidth;
    UTINY *sp;
    UTINY *bufptr;
    UTINY *buf;
    WORD code;
    WORD fc;
    WORD oc;
    WORD bufcnt;
    WORD c;
    WORD size;
    WORD ret;

    linewidth = (WORD)iLinewidth;

    // Initialize for decoding a new image...
    if ((size = GifFormat::getByte()) < 0) {
        return (size);
    }
    if (size < 2 || 9 < size) {
        return (BAD_CODE_SIZE);
    }
    GifDecoder::initExp((int)size); // changed param to int

    dstack = new UTINY[MAX_CODES + 1];
    suffix = new UTINY[MAX_CODES + 1];
    prefix = new UWORD[MAX_CODES + 1];

    // Initialize in case they forgot to put in a clear code.
    // (This shouldn't happen, but we'll try and decode it anyway...)
    oc = fc = 0;

    buf = GifDecoder::decoderline;

    badCodeCount = 0;

    // Set up the stack pointer and decode buffer pointer
    sp = dstack;
    bufptr = buf;
    bufcnt = linewidth;

    // This is the main loop.  For each code we get we pass through the
    // linked list of prefix codes, pushing the corresponding "character" for
    // each code onto the stack.  When the list reaches a single "character"
    // we push that on the stack too, and then start unstacking each
    // character for output in the correct order.  Special handling is
    // included for the clear code, and the whole thing ends when we get
    // an ending code.
    while ((c = GifDecoder::getNextCode()) != ending) {

        // If we had a file error, return without completing the decode
        if (c < 0) {
            GifDecoder::cleanupGifDecoder();
            return (0);
        }

        // If the code is a clear code, reinitialize all necessary items.
        if (c == clear) {
            currSize = size + 1;
            slot = newcodes;
            topSlot = 1 << currSize;

            // Continue reading codes until we get a non-clear code
            // (Another unlikely, but possible case...)
            while ((c = GifDecoder::getNextCode()) == clear) {
                ;
            }

            // If we get an ending code immediately after a clear code
            // (Yet another unlikely case), then break out of the loop.
            if (c == ending) {
                break;
            }

            // Finally, if the code is beyond the range of already set codes,
            // (This one had better NOT happen...  I have no idea what will
            // result from this, but I doubt it will look good...) then set it
            // to color zero.
            if (c >= slot) {
                c = 0;
            }

            oc = fc = c;

            // And let us not forget to put the char into the buffer... And
            // if, on the off chance, we were exactly one pixel from the end
            // of the line, we have to send the buffer to the
            // GifFormat::outLine() routine...
            *bufptr++ = (UTINY)c;
            if (--bufcnt == 0) {
                if ((ret = GifFormat::outLine(buf, linewidth)) < 0) {
                    GifDecoder::cleanupGifDecoder();
                    return (ret);
                }

                bufptr = buf;
                bufcnt = linewidth;
            }
        } else {

            // In this case, it's not a clear code or an ending code, so
            // it must be a code code...  So we can now decode the code into
            // a stack of character codes. (Clear as mud, right?)
            code = c;

            // Here we go again with one of those off chances...  If, on the
            // off chance, the code we got is beyond the range of those already
            // set up (Another thing which had better NOT happen...) we trick
            // the decoder into thinking it actually got the last code read.
            // (Hmmn... I'm not sure why this works...  But it does...)
            if (code >= slot) {
                if (code > slot) {
                    ++badCodeCount;
                }
                code = oc;
                *sp++ = (UTINY)fc;
            }

            // Here we scan back along the linked list of prefixes, pushing
            // helpless characters (ie. suffixes) onto the stack as we do so.
            while (code >= newcodes) {
                *sp++ = suffix[code];
                code = prefix[code];
            }

            // Push the last character on the stack, and set up the new
            // prefix and suffix, and if the required slot number is greater
            // than that allowed by the current bit size, increase the bit
            // size.  (NOTE - If we are all full, we *don't* save the new
            // suffix and prefix...  I'm not certain if this is correct...
            // it might be more proper to overwrite the last code...
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

            // Now that we've pushed the decoded string (in reverse order)
            // onto the stack, lets pop it off and put it into our decode
            // buffer...  And when the decode buffer is full, write another
            // line...
            while (sp > dstack) {
                *bufptr++ = *(--sp);
                if (--bufcnt == 0) {
                    if ((ret = GifFormat::outLine(buf, linewidth)) < 0) {
                        GifDecoder::cleanupGifDecoder();
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
        ret = GifFormat::outLine(buf, (linewidth - bufcnt));
    }

    GifDecoder::cleanupGifDecoder();
    return (ret);
}
