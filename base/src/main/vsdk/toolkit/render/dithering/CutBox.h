#ifndef __CUT_BOX__
#define __CUT_BOX__
#include <stdint.h>

class CutBox {
public:
    unsigned first;
    unsigned end;
    uint64_t pixels;
    unsigned low[3];
    unsigned high[3];
    unsigned longest;
    unsigned score;
};

#endif
