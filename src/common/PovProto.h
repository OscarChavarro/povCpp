#ifndef __POVPROTO_H__
#define __POVPROTO_H__
/****************************************************************************
 *                         povproto.h
 *
 *  This module defines the prototypes for all system-independent functions.
 *
 *****************************************************************************/

/* Some extrange globally used functions */
extern void Error(const char *str);
#include "app/PovApp.h"
extern void displayPlot(
    int x, int y, unsigned char Red, unsigned char Green, unsigned char Blue);

#endif
