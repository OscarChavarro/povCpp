#ifndef __GIF_DECODER__
#define __GIF_DECODER__

typedef short WORD;

struct GifInputContext {
    int (*getByte)(void *context);
    int (*outLine)(void *context, const unsigned char *pixels, int linelen);
    void *context;
};

struct GifDecoderState {
    unsigned char *decoderline;
    long codeMask[13];
    int badCodeCount;
    WORD currSize;
    WORD clear;
    WORD ending;
    WORD newcodes;
    WORD topSlot;
    WORD slot;
    WORD navailBytes;
    WORD nbitsLeft;
    unsigned char b1;
    unsigned char byteBuff[257];
    unsigned char *pbytes;
    unsigned char *dstack;
    unsigned char *suffix;
    unsigned short *prefix;
};

class GifDecoder {
  public:
    static void cleanupGifDecoder(GifDecoderState &state);
    static WORD initExp(int iSize, GifDecoderState &state);
    static WORD getNextCode(GifInputContext &input, GifDecoderState &state);
    static WORD decoder(int iLinewidth, GifInputContext &input, GifDecoderState &state);
};

#endif
