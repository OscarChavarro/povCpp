/****************************************************************************
*                         iff.c
*
*  This file implements a simple IFF format file reader.
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

#include "common/frame.h"
#include "common/povproto.h"
#include "io/iff.h"

static RGBAPixel *iff_colour_map;
static int colourmap_size;
static ChunkHeader GLOBAL_chunkHeader;

#define FORM 0x464f524dL
#define ILBM 0x494c424dL
#define BMHD 0x424d4844L
#define CAMG 0x43414d47L
#define CMAP 0x434d4150L
#define BODY 0x424f4459L
#define CMPNONE 0

#define HAM 0x800

void
iff_error()
{
    fprintf (stderr, "Invalid IFF file\n");
    exit(1);
}

int
read_byte(FILE *f)
{
    int c;
    if ((c = getc(f)) == EOF)
        iff_error();
    return (c);
}

int
read_word(FILE *f)
{
    int result;

    result = read_byte(f)*256;
    result += read_byte(f);
    return (result);
}

long
read_long(FILE *f)
{
    int i;
    long result;

    result = 0;
    for (i = 0 ; i < 4 ; i++)
        result = result * 256 + read_byte(f);

    return (result);
}

void
Read_Chunk_Header(FILE *f, ChunkHeader *dest)
{
    dest->name = read_long(f);
    dest->size = read_long(f);
}

void
Read_Iff_Image(RGBAImage *image, char *filename)
{
    FILE *f;
    unsigned char **row_bytes;
    int c, i, j, k, nBytes, nPlanes = 0, compression = 0,
    mask, byte_index, count, viewmodes;
    int Previous_Red, Previous_Green, Previous_Blue;
    ImageLine *line;
    unsigned long creg;

    if ((f = Locate_File(filename, READ_FILE_STRING)) == NULL) {
        fprintf (stderr, "Cannot open IFF file %s\n", filename);
        exit(1);
    }

    Previous_Red = Previous_Green = Previous_Blue = 0;

    viewmodes = 0;
    iff_colour_map = NULL;

    while (1) {
        Read_Chunk_Header(f, &GLOBAL_chunkHeader);
        switch ( IFF_SWITCH_CAST GLOBAL_chunkHeader.name) {
        case FORM:  
            if (read_long(f) != ILBM)
                iff_error();
            break;

        case BMHD:  
            image->iwidth = read_word(f);
            image->width = (DBL)image->iwidth;
            image->iheight = read_word(f);
            image->height = (DBL)image->iheight;

            read_word(f);  /* x position ignored */
            read_word(f);  /* y position ignored */
            nPlanes = read_byte(f);
            colourmap_size = 1<<nPlanes;
            read_byte(f);    /* masking ignored */
            compression = read_byte(f);    /* masking ignored */
            read_byte(f);    /* pad */
            read_word(f);    /* Transparent colour ignored */
            read_word(f);    /* Aspect ratio ignored */
            read_word(f);    /* page width ignored */
            read_word(f);    /* page height ignored */
            break;

        case CAMG:  
            viewmodes = (int) read_long(f);    /* Viewmodes */
            if (viewmodes & HAM)
                colourmap_size = 16;

            break;

        case CMAP:  
            colourmap_size = (int) GLOBAL_chunkHeader.size / 3;
            iff_colour_map = new RGBAPixel[colourmap_size];
            if ( iff_colour_map == NULL ) {
                fprintf(stderr, "Cannot allocate memory for IFF colour map\n");
                exit(1);
            }

            for (i = 0 ; i < colourmap_size ; i++) {
                iff_colour_map[i].Red = read_byte(f);
                iff_colour_map[i].Green = read_byte(f);
                iff_colour_map[i].Blue = read_byte(f);
                iff_colour_map[i].Alpha = 0;
            }

            Previous_Red = iff_colour_map[0].Red;
            Previous_Green = iff_colour_map[0].Green;
            Previous_Blue = iff_colour_map[0].Blue;
            for (i = colourmap_size * 3 ; (long) i < GLOBAL_chunkHeader.size ; i++)
                read_byte(f);

            break;

        case BODY:  
            if ((iff_colour_map == NULL) || (viewmodes & HAM)) {
                image->Colour_Map_Size = 0;
                image->Colour_Map = NULL;
            }
            else {
                image->Colour_Map_Size = colourmap_size;
                image->Colour_Map = iff_colour_map;
            }
                row_bytes = new unsigned char *[nPlanes];
                if ( row_bytes == NULL ) {
                    fprintf (stderr, "Cannot allocate memory for row bytes\n");
                    exit(1);
                }

            for (i = 0 ; i < nPlanes ; i++)
                 if ( (row_bytes[i] = new unsigned char[((image->iwidth+7)/8)]) == NULL ) {
                    fprintf (stderr, "Cannot allocate memory for row bytes\n");
                    exit(1);
                }

            if (image->Colour_Map == NULL) {
                image->data.rgb_lines = new ImageLine[image->iheight];
                if ( image->data.rgb_lines == NULL ) {
                    fprintf (stderr, "Cannot allocate memory for picture\n");
                    exit(1);
                }
            }
            else {
                image->data.map_lines = new unsigned char *[image->iheight];
                if ( image->data.map_lines == NULL ) {
                    fprintf (stderr, "Cannot allocate memory for picture\n");
                    exit(1);
                }
            }

            for (i = 0 ; i < image->iheight ; i++) {

                if (image->Colour_Map == NULL) {
                    if ( 
((image->data.rgb_lines[i].red = new unsigned char[image->iwidth])==NULL) ||
((image->data.rgb_lines[i].green = new unsigned char[image->iwidth])==NULL) ||
((image->data.rgb_lines[i].blue = new unsigned char[image->iwidth])==NULL)
                        ) {
                        fprintf (stderr, "Cannot allocate memory for picture\n");
                        exit(1);
                    }
                }
                else {
                    image->data.map_lines[i] = new unsigned char[image->iwidth];
                    if ( image->data.map_lines[i] == NULL ) {
                        fprintf (stderr, "Cannot allocate memory for picture\n");
                        exit(1);
                    }
                }

                for (j = 0 ; j < nPlanes ; j++)
                    if (compression == CMPNONE) {
                        for (k = 0 ; k < (image->iwidth+7)/8 ; k++)
                            row_bytes[j][k] = (unsigned char)read_byte(f);
                        if ((k & 1) != 0)
                            read_byte(f);
                    }

                    else {
                        nBytes = 0;
                        while (nBytes != (image->iwidth+7)/8) {
                            c = read_byte(f);
                            if ((c >= 0) && (c <= 127))
                                for (k = 0 ; k <= c ; k++)
                                    row_bytes[j][nBytes++] = (unsigned char)read_byte(f);
                            else if ((c >= 129) && (c <= 255)) {
                                count = 257-c;
                                c = read_byte(f);
                                for (k = 0 ; k < count ; k++)
                                    row_bytes[j][nBytes++] = (unsigned char)c;
                            }
                        }
                    }

                mask = 0x80;
                byte_index = 0;
                for (j = 0 ; j < image->iwidth ; j++) {
                    creg = 0;
                    for (k = nPlanes-1 ; k >= 0 ; k--)
                        if (row_bytes[k][byte_index] & mask)
                            creg = creg*2 + 1;
                        else
                            creg *= 2;


                    if (viewmodes & HAM) {
                        line = &image->data.rgb_lines[i];
                        switch (creg >> 4) {
                        case 0:
                            Previous_Red = line->red[j] = (unsigned char)iff_colour_map[creg].Red;
                            Previous_Green = line->green[j] = (unsigned char)iff_colour_map[creg].Green;
                            Previous_Blue = line->blue[j] = (unsigned char)iff_colour_map[creg].Blue;
                            break;

                        case 1:
                            line->red[j] = (unsigned char)Previous_Red;
                            line->green[j] = (unsigned char)Previous_Green;
                            line->blue[j] = (unsigned char)(((creg & 0xf)<<4) + (creg&0xf));
                            Previous_Blue = (int) line->blue[j];
                            break;

                        case 2:
                            line->red[j] = (unsigned char)(((creg & 0xf)<<4) + (creg&0xf));
                            Previous_Red = (int) line->red[j];
                            line->green[j] = (unsigned char)Previous_Green;
                            line->blue[j] = (unsigned char)Previous_Blue;
                            break;

                        case 3:
                            line->red[j] = (unsigned char)Previous_Red;
                            line->green[j] = (unsigned char)(((creg & 0xf)<<4) + (creg&0xf));
                            Previous_Green = (int) line->green[j];
                            line->blue[j] = (unsigned char)Previous_Blue;
                            break;
                        }
                    }
                    else if (nPlanes == 24) {
                        line = &image->data.rgb_lines[i];
                        line->red[j] = (unsigned char)((creg >> 16) & 0xFF);
                        line->green[j] = (unsigned char)((creg >> 8) & 0xFF);
                        line->blue[j] = (unsigned char)(creg & 0xFF);
                    }
                    else {
                        if (creg > (unsigned long)image->Colour_Map_Size) {
                            fprintf (stderr, "Error - IFF Image Map Colour out of range\n");
                            exit(1);
                        }
                        image->data.map_lines[i][j] = (char)creg;
                    }

                    mask >>= 1;
                    if (mask == 0) {
                        mask = 0x80;
                        byte_index++;
                    }
                }
            }

            delete row_bytes;
            fclose (f);
            return;

        default:
            for (i = 0 ; (long) i < GLOBAL_chunkHeader.size ; i++)
                if (getc(f) == EOF)
                    iff_error();
            break;
        }
    }
}
