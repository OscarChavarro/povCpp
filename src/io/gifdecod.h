#ifndef __GIFDECOD_H__
#define __GIFDECOD_H__

#include "common/frame.h"

extern void cleanup_gif_decoder(void);
extern WORD init_exp(int i_size); /* changed param to int to avoid
                                                   problems with 32bit int ANSI
                                                   compilers. */
extern WORD get_next_code(void);
extern WORD decoder(int i_linewidth); /* same as above */

#endif
