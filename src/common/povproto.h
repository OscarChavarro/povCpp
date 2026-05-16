#ifndef __POVPROTO_H__
#define __POVPROTO_H__
/****************************************************************************
*                         povproto.h
*
*  This module defines the prototypes for all system-independent functions.
*
*  from Persistence of Vision Raytracer 
*  Copyright 1992 Persistence of Vision Team
*---------------------------------------------------------------------------
*  Copying, distribution and legal info is in the file povlegal.doc which
*  should be distributed with this file. If povlegal.doc is not available
*  or for more info please contact:
*
*         Drew Wells [POV-Team Leader] 
*         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
*         Phone: (213) 254-4041
* 
* This program is based on the popular DKB raytracer version 2.12.
* DKBTrace was originally written by David K. Buck.
* DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
*
*****************************************************************************/

/* Some extrange globally used functions */
extern void Error(const char *str);
extern void close_all(void);
extern void print_stats(void);
extern FILE *Locate_File(const char *filename, const char *mode);
extern void display_plot(int x, int y, unsigned char Red, unsigned char Green, unsigned char Blue);

#endif
