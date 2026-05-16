/****************************************************************************
 *                         gif.c
 *
 *  Gif-format file reader.
 *
 *  NOTE:  Portions of this module were written by Steve Bennett and are used
 *            here with his permission.
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

/*
    The following routines were borrowed freely from FRACTINT, and represent
    a generalized GIF file decoder.  This seems the best, most universal format
    for reading in Bitmapped images.  GIF is a Copyright of Compuserve, Inc.
    Swiped and converted to entirely "C" coded routines by AAC for the most
    in future portability!
*/

#include "io/Gif.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "io/GifDecod.h"

static RGBAImage *currentImage;
static int bitmapLine;
static FILE *bitFile;
unsigned char *decoderline /*  [2049] */; /* write-line routines use this */

static RGBAPixel *gifColourMap;
static int colourmapSize;

/* IMPORT INT out_line(pixels, linelen)
 *      UBYTE pixels[];
 *      INT linelen;
 *
 *    - This function takes a full line of pixels (one byte per pixel) and
 * displays them (or does whatever your program wants with them...).  It
 * should return zero, or negative if an error or some other event occurs
 * which would require aborting the decode process...  Note that the length
 * passed will almost always be equal to the line length passed to the
 * decoder function, with the sole exception occurring when an ending code
 * occurs in an odd place in the GIF file...  In any case, linelen will be
 * equal to the number of pixels passed...
 */
int
out_line(unsigned char *pixels, int linelen)
{
    register int x;
    register unsigned char *line;

    line = currentImage->data.map_lines[bitmapLine++];

    for (x = 0; x < linelen; x++) {
        if ((int)(*pixels) > currentImage->Colour_Map_Size) {
            fprintf(stderr, "Error - GIF Image Map Colour out of range\n");
            exit(1);
        }

        line[x] = *pixels;
        pixels++;
    }

    return (0);
}

#define READ_ERROR -1

int
get_byte() /* get byte from file, return the next byte or an error */
{
    register int byte;

    if ((byte = getc(bitFile)) != EOF) {
        return (byte);
    }
    fprintf(stderr, "Premature End Of File reading GIF image\n");
    exit(1);

    return (0); /* Keep the compiler happy */
}

/* Main GIF file decoder.  */

void
Read_Gif_Image(RGBAImage *image, char *filename)
{
    register int i;
    register int j;
    register int status;
    unsigned finished;
    unsigned planes;
    unsigned char buffer[16];

    status = 0;
    currentImage = image;

    if ((bitFile = Locate_File(filename, READ_FILE_STRING)) == nullptr) {
        fprintf(stderr, "Cannot open GIF file %s\n", filename);
        exit(1);
    }

    /* zero out the full write-line */
    decoderline = new unsigned char[2049];
    if (decoderline == nullptr) {
        fprintf(stderr, "Cannot allocate space for GIF decoder line\n");
        fclose(bitFile);
        exit(1);
    }

    for (i = 0; i < 2049; i++) {
        decoderline[i] = (unsigned char)0;
    }

    /* Get the screen description */
    for (i = 0; i < 13; i++) {
        buffer[i] = (unsigned char)get_byte();
    }

    if (strncmp((char *)buffer, "GIF", 3) || /* use updated GIF specs */
        buffer[3] < '0' || buffer[3] > '9' || buffer[4] < '0' ||
        buffer[4] > '9' || buffer[5] < 'A' || buffer[5] > 'z') {

        fprintf(stderr, "Invalid GIF file format: %s\n", filename);
        fclose(bitFile);
        exit(1);
    }

    planes = ((unsigned)buffer[10] & 0x0F) + 1;
    colourmapSize = (int)(1 << planes);

    gifColourMap = new RGBAPixel[colourmapSize];
    if (gifColourMap == nullptr) {
        fprintf(stderr, "Cannot allocate GIF Colour Map\n");
        fclose(bitFile);
        exit(1);
    }

    if ((buffer[10] & 0x80) == 0) { /* color map (better be!) */
        fprintf(stderr, "Invalid GIF file format: %s\n", filename);
        fclose(bitFile);
        exit(1);
    }

    for (i = 0; i < colourmapSize; i++) {
        gifColourMap[i].Red = (unsigned char)get_byte();
        gifColourMap[i].Green = (unsigned char)get_byte();
        gifColourMap[i].Blue = (unsigned char)get_byte();
        gifColourMap[i].Alpha = 0;
    }

    /* Now display one or more GIF objects */
    finished = FALSE;
    while (!finished) {
        switch (get_byte()) {
        case ';': /* End of the GIF dataset */
            finished = TRUE;
            status = 0;
            break;

        case '!':                          /* GIF Extension Block */
            get_byte();                    /* read (and ignore) the ID */
            while ((i = get_byte()) > 0) { /* get data len*/
                for (j = 0; j < i; j++) {
                    get_byte(); /* flush data */
                }
            }
            break;

        case ',': /* Start of image object. get description */
            for (i = 0; i < 9; i++) {
                if ((j = get_byte()) < 0) { /* EOF test (?) */
                    status = -1;
                    break;
                }
                buffer[i] = (unsigned char)j;
            }

            if (status < 0) {
                finished = TRUE;
                break;
            }

            image->iwidth = buffer[4] | (buffer[5] << 8);
            image->iheight = buffer[6] | (buffer[7] << 8);
            image->width = (DBL)image->iwidth;
            image->height = (DBL)image->iheight;

            bitmapLine = 0;

            image->Colour_Map_Size = colourmapSize;
            image->Colour_Map = gifColourMap;

            image->data.map_lines = new unsigned char *[image->iheight];
            if (image->data.map_lines == nullptr) {
                fprintf(stderr, "Cannot allocate memory for picture\n");
                exit(1);
            }

            for (i = 0; i < image->iheight; i++) {
                image->data.map_lines[i] = new unsigned char[image->iwidth];
                if (image->data.map_lines[i] == nullptr) {
                    fprintf(stderr, "Cannot allocate memory for picture\n");
                    exit(1);
                }
            }

            /* Setup the color palette for the image */
            status = decoder(image->iwidth); /*put bytes in Buf*/
            /* changed param to int */
            finished = TRUE;
            break;

        default:
            status = -1;
            finished = TRUE;
            break;
        }
    }

    delete decoderline;
    fclose(bitFile);
}
