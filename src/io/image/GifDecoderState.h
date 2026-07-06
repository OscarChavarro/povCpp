#ifndef __GIF_DECODER_STATE__
#define __GIF_DECODER_STATE__

typedef short WORD;

class GifDecoderState {
  public:
    GifDecoderState(
        unsigned char *decoderline,
        const long (&codeMask)[13],
        int badCodeCount,
        WORD currSize,
        WORD clear,
        WORD ending,
        WORD newcodes,
        WORD topSlot,
        WORD slot,
        WORD navailBytes,
        WORD nbitsLeft,
        unsigned char b1,
        unsigned char *pbytes,
        unsigned char *dstack,
        unsigned char *suffix,
        unsigned short *prefix) :
        decoderline(decoderline),
        badCodeCount(badCodeCount),
        currSize(currSize),
        clear(clear),
        ending(ending),
        newcodes(newcodes),
        topSlot(topSlot),
        slot(slot),
        navailBytes(navailBytes),
        nbitsLeft(nbitsLeft),
        b1(b1),
        pbytes(pbytes),
        dstack(dstack),
        suffix(suffix),
        prefix(prefix)
    {
        for (int i = 0; i < 13; i++) {
            this->codeMask[i] = codeMask[i];
        }
        for (int i = 0; i < 257; i++) {
            byteBuff[i] = 0;
        }
    }

    unsigned char *getDecoderline() const { return decoderline; }
    long getCodeMask(int index) const { return codeMask[index]; }
    int getBadCodeCount() const { return badCodeCount; }
    WORD getCurrSize() const { return currSize; }
    WORD getClear() const { return clear; }
    WORD getEnding() const { return ending; }
    WORD getNewcodes() const { return newcodes; }
    WORD getTopSlot() const { return topSlot; }
    WORD getSlot() const { return slot; }
    WORD getNavailBytes() const { return navailBytes; }
    WORD getNbitsLeft() const { return nbitsLeft; }
    unsigned char getB1() const { return b1; }
    unsigned char getByteBuff(int index) const { return byteBuff[index]; }
    unsigned char *getByteBuffArray() const { return const_cast<unsigned char *>(byteBuff); }
    unsigned char *getPbytes() const { return pbytes; }
    unsigned char *getDstack() const { return dstack; }
    unsigned char *getSuffix() const { return suffix; }
    unsigned short *getPrefix() const { return prefix; }

    void setDecoderline(unsigned char *value) { decoderline = value; }
    void setBadCodeCount(int value) { badCodeCount = value; }
    void incrementBadCodeCount() { badCodeCount++; }
    void setCurrSize(WORD value) { currSize = value; }
    void incrementCurrSize() { currSize++; }
    void setClear(WORD value) { clear = value; }
    void setEnding(WORD value) { ending = value; }
    void setNewcodes(WORD value) { newcodes = value; }
    void setTopSlot(WORD value) { topSlot = value; }
    void setSlot(WORD value) { slot = value; }
    void incrementSlot() { slot++; }
    void setNavailBytes(WORD value) { navailBytes = value; }
    void decrementNavailBytes() { navailBytes--; }
    void setNbitsLeft(WORD value) { nbitsLeft = value; }
    void setB1(unsigned char value) { b1 = value; }
    void setByteBuff(int index, unsigned char value) { byteBuff[index] = value; }
    void setPbytes(unsigned char *value) { pbytes = value; }
    // Returns the current pointer, then advances it - matches the
    // pre-extraction `*state.pbytes++` call-site idiom exactly.
    unsigned char *advancePbytes() { return pbytes++; }
    void setDstack(unsigned char *value) { dstack = value; }
    void setSuffix(unsigned char *value) { suffix = value; }
    void setPrefix(unsigned short *value) { prefix = value; }

  private:
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

#endif
