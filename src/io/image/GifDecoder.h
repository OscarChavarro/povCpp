#ifndef __GIFDECOD_H__
#define __GIFDECOD_H__

typedef short WORD;

class GifDecoder {
  public:
    static unsigned char *decoderline;
    static void cleanupGifDecoder(void);
    static WORD initExp(int iSize);
    static WORD getNextCode(void);
    static WORD decoder(int iLinewidth);

  private:
    static long codeMask[13];
    static int badCodeCount;
    static WORD currSize;
    static WORD clear;
    static WORD ending;
    static WORD newcodes;
    static WORD topSlot;
    static WORD slot;
    static WORD navailBytes;
    static WORD nbitsLeft;
    static unsigned char b1;
    static unsigned char byteBuff[257];
    static unsigned char *pbytes;
    static unsigned char *dstack;
    static unsigned char *suffix;
    static unsigned short *prefix;
};

#endif
