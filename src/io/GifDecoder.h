#ifndef __GIFDECOD_H__
#define __GIFDECOD_H__

#include "common/FrameConfig.h"

class GifDecoder {
  public:
    static void cleanupGifDecoder(void);
    static WORD initExp(int iSize);
    static WORD getNextCode(void);
    static WORD decoder(int iLinewidth);
};

#endif
