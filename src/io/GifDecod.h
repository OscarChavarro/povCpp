#ifndef __GIFDECOD_H__
#define __GIFDECOD_H__

#include "common/Frame.h"

extern void cleanupGifDecoder(void);
extern WORD initExp(int i_size); /* changed param to int to avoid
                                                   problems with 32bit int ANSI
                                                   compilers. */
extern WORD getNextCode(void);
extern WORD decoder(int i_linewidth); /* same as above */

#endif
