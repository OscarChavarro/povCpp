#ifndef __GIF_DECODER__
#define __GIF_DECODER__

#include "io/image/GifDecoderState.h"
#include "io/image/GifInputContext.h"

class GifDecoder {
  public:
    static void cleanupGifDecoder(GifDecoderState &state);
    static WORD initExp(int iSize, GifDecoderState &state);
    static WORD getNextCode(GifInputContext &input, GifDecoderState &state);
    static WORD decoder(int iLinewidth, GifInputContext &input, GifDecoderState &state);
};

#endif
