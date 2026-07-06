
#include "io/image/GifDecoder.h"

typedef unsigned short UWORD;
typedef char TEXT;
typedef unsigned char UTINY;
typedef long LONG;
typedef unsigned long ULONG;
typedef int INT;

static constexpr int BAD_CODE_SIZE = -20;

static constexpr int MAX_CODES = 4095;

// This function initializes the decoder for reading a new image.
WORD
GifDecoder::initExp(int iSize, GifDecoderState &state)
{
    WORD size = (WORD)iSize;
    state.setCurrSize(size + 1);
    state.setTopSlot(1 << state.getCurrSize());
    state.setClear(1 << size);
    state.setEnding(state.getClear() + 1);
    state.setNewcodes(state.getEnding() + 1);
    state.setSlot(state.getNewcodes());
    state.setNavailBytes(0);
    state.setNbitsLeft(0);
    return (0);
}

// GifDecoder::getNextCode()
// - gets the next code from the GIF file.  Returns the code, or else
// a negative number in case of file errors...
WORD
GifDecoder::getNextCode(GifInputContext &input, GifDecoderState &state)
{
    WORD i;
    WORD x;
    ULONG ret;

    if (state.getNbitsLeft() == 0) {
        if (state.getNavailBytes() <= 0) {

            // Out of bytes in current block, so read next block
            state.setPbytes(state.getByteBuffArray());
            WORD navailBytes = (WORD)input.getByteReader()(input.getContext());
            state.setNavailBytes(navailBytes);
            if (navailBytes < 0) {
                return (navailBytes);
            }
            if (state.getNavailBytes()) {
                for (i = 0; i < state.getNavailBytes(); ++i) {
                    if ((x = input.getByteReader()(input.getContext())) < 0) {
                        return (x);
                    }
                    state.setByteBuff(i, (UTINY)x);
                }
            }
        }
        state.setB1(*state.advancePbytes());
        state.setNbitsLeft(8);
        state.decrementNavailBytes();
    }

    ret = state.getB1() >> (8 - state.getNbitsLeft());
    while (state.getCurrSize() > state.getNbitsLeft()) {
        if (state.getNavailBytes() <= 0) {

            // Out of bytes in current block, so read next block
            state.setPbytes(state.getByteBuffArray());
            WORD navailBytes = (WORD)input.getByteReader()(input.getContext());
            state.setNavailBytes(navailBytes);
            if (navailBytes < 0) {
                return (navailBytes);
            }
            if (state.getNavailBytes()) {
                for (i = 0; i < state.getNavailBytes(); ++i) {
                    if ((x = input.getByteReader()(input.getContext())) < 0) {
                        return (x);
                    }
                    state.setByteBuff(i, (UTINY)x);
                }
            }
        }
        state.setB1(*state.advancePbytes());
        ret |= state.getB1() << state.getNbitsLeft();
        state.setNbitsLeft(state.getNbitsLeft() + 8);
        state.decrementNavailBytes();
    }
    state.setNbitsLeft(state.getNbitsLeft() - state.getCurrSize());
    ret &= state.getCodeMask(state.getCurrSize());
    return ((WORD)(ret));
}

// The reason we have these separated like this instead of using
// a structure like the original Wilhite code did, is because this
// stuff generally produces significantly faster code when compiled...
// This code is full of similar speedups...  (For a good book on writing
// C for speed or for space optimization, see Efficient C by Tom Plum,
// published by Plum-Hall Associates...)
//
// Decoder work buffers are held as class static members.

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
GifDecoder::cleanupGifDecoder(GifDecoderState &state)
{
    delete[] state.getDstack();
    delete[] state.getSuffix();
    delete[] state.getPrefix();
}

WORD
GifDecoder::decoder(int iLinewidth, GifInputContext &input, GifDecoderState &state)
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
    if ((size = input.getByteReader()(input.getContext())) < 0) {
        return (size);
    }
    if (size < 2 || 9 < size) {
        return (BAD_CODE_SIZE);
    }
    GifDecoder::initExp((int)size, state); // changed param to int

    state.setDstack(new UTINY[MAX_CODES + 1]);
    state.setSuffix(new UTINY[MAX_CODES + 1]);
    state.setPrefix(new UWORD[MAX_CODES + 1]);

    // Initialize in case they forgot to put in a clear code.
    // (This shouldn't happen, but we'll try and decode it anyway...)
    oc = fc = 0;

    buf = state.getDecoderline();

    state.setBadCodeCount(0);

    // Set up the stack pointer and decode buffer pointer
    sp = state.getDstack();
    bufptr = buf;
    bufcnt = linewidth;

    // This is the main loop.  For each code we get we pass through the
    // linked list of prefix codes, pushing the corresponding "character" for
    // each code onto the stack.  When the list reaches a single "character"
    // we push that on the stack too, and then start unstacking each
    // character for output in the correct order.  Special handling is
    // included for the clear code, and the whole thing ends when we get
    // an ending code.
    while ((c = GifDecoder::getNextCode(input, state)) != state.getEnding()) {

        // If we had a file error, return without completing the decode
        if (c < 0) {
            GifDecoder::cleanupGifDecoder(state);
            return (0);
        }

        // If the code is a clear code, reinitialize all necessary items.
        if (c == state.getClear()) {
            state.setCurrSize(size + 1);
            state.setSlot(state.getNewcodes());
            state.setTopSlot(1 << state.getCurrSize());

            // Continue reading codes until we get a non-clear code
            // (Another unlikely, but possible case...)
            while ((c = GifDecoder::getNextCode(input, state)) == state.getClear()) {
                ;
            }

            // If we get an ending code immediately after a clear code
            // (Yet another unlikely case), then break out of the loop.
            if (c == state.getEnding()) {
                break;
            }

            // Finally, if the code is beyond the range of already set codes,
            // (This one had better NOT happen...  I have no idea what will
            // result from this, but I doubt it will look good...) then set it
            // to color zero.
            if (c >= state.getSlot()) {
                c = 0;
            }

            oc = fc = c;

            // And let us not forget to put the char into the buffer... And
            // if, on the off chance, we were exactly one pixel from the end
            // of the line, we have to send the buffer to the
            // GifFormat::outLine() routine...
            *bufptr++ = (UTINY)c;
            if (--bufcnt == 0) {
                if ((ret = input.getLineWriter()(input.getContext(), buf, linewidth)) < 0) {
                    GifDecoder::cleanupGifDecoder(state);
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
            if (code >= state.getSlot()) {
                if (code > state.getSlot()) {
                    state.incrementBadCodeCount();
                }
                code = oc;
                *sp++ = (UTINY)fc;
            }

            // Here we scan back along the linked list of prefixes, pushing
            // helpless characters (ie. suffixes) onto the stack as we do so.
            while (code >= state.getNewcodes()) {
                *sp++ = state.getSuffix()[code];
                code = state.getPrefix()[code];
            }

            // Push the last character on the stack, and set up the new
            // prefix and suffix, and if the required slot number is greater
            // than that allowed by the current bit size, increase the bit
            // size.  (NOTE - If we are all full, we *don't* save the new
            // suffix and prefix...  I'm not certain if this is correct...
            // it might be more proper to overwrite the last code...
            *sp++ = (UTINY)code;
            if (state.getSlot() < state.getTopSlot()) {
                fc = code;
                state.getSuffix()[state.getSlot()] = (UTINY)fc;
                state.getPrefix()[state.getSlot()] = oc;
                state.incrementSlot();
                oc = c;
            }
            if (state.getSlot() >= state.getTopSlot()) {
                if (state.getCurrSize() < 12) {
                    state.setTopSlot(state.getTopSlot() << 1);
                    state.incrementCurrSize();
                }
            }

            // Now that we've pushed the decoded string (in reverse order)
            // onto the stack, lets pop it off and put it into our decode
            // buffer...  And when the decode buffer is full, write another
            // line...
            while (sp > state.getDstack()) {
                *bufptr++ = *(--sp);
                if (--bufcnt == 0) {
                    if ((ret = input.getLineWriter()(input.getContext(), buf, linewidth)) < 0) {
                        GifDecoder::cleanupGifDecoder(state);
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
        ret = input.getLineWriter()(input.getContext(), buf, (linewidth - bufcnt));
    }

    GifDecoder::cleanupGifDecoder(state);
    return (ret);
}
