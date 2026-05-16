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

#include "io/Iff.h"
#include "common/Frame.h"
#include "common/PovProto.h"

static RGBAPixel *iffColourMap;
static int colourmapSize;
static ChunkHeader globalChunkHeader;

static constexpr long FORM = 0x464f524dL;
static constexpr long ILBM = 0x494c424dL;
static constexpr long BMHD = 0x424d4844L;
static constexpr long CAMG = 0x43414d47L;
static constexpr long CMAP = 0x434d4150L;
static constexpr long BODY = 0x424f4459L;
static constexpr int CMPNONE = 0;

static constexpr int HAM = 0x800;

void
iffError()
{
    fprintf(stderr, "Invalid IFF file\n");
    exit(1);
}

int
readByte(FILE *f)
{
    int c;
    if ((c = getc(f)) == EOF) {
        iffError();
    }
    return (c);
}

int
readWord(FILE *f)
{
    int result;

    result = readByte(f) * 256;
    result += readByte(f);
    return (result);
}

long
readLong(FILE *f)
{
    int i;
    long result;

    result = 0;
    for (i = 0; i < 4; i++) {
        result = result * 256 + readByte(f);
    }

    return (result);
}

void
readChunkHeader(FILE *f, ChunkHeader *dest)
{
    dest->name = readLong(f);
    dest->size = readLong(f);
}

void
readIffImage(RGBAImage *image, char *filename)
{
    FILE *f;
    unsigned char **rowBytes;
    int c;
    int i;
    int j;
    int k;
    int nBytes;
    int nPlanes = 0;
    int compression = 0;
    int mask;
    int byteIndex;
    int count;
    int viewmodes;
    int previousRed;
    int previousGreen;
    int previousBlue;
    ImageLine *line;
    unsigned long creg;

    if ((f = locateFile(filename, READ_FILE_STRING)) == nullptr) {
        fprintf(stderr, "Cannot open IFF file %s\n", filename);
        exit(1);
    }

    previousRed = previousGreen = previousBlue = 0;

    viewmodes = 0;
    iffColourMap = nullptr;

    while (true) {
        readChunkHeader(f, &globalChunkHeader);
        switch (static_cast<int>(globalChunkHeader.name)) {
        case FORM:
            if (readLong(f) != ILBM) {
                iffError();
            }
            break;

        case BMHD:
            image->iwidth = readWord(f);
            image->width = (DBL)image->iwidth;
            image->iheight = readWord(f);
            image->height = (DBL)image->iheight;

            readWord(f); /* x position ignored */
            readWord(f); /* y position ignored */
            nPlanes = readByte(f);
            colourmapSize = 1 << nPlanes;
            readByte(f);               /* masking ignored */
            compression = readByte(f); /* masking ignored */
            readByte(f);               /* pad */
            readWord(f);               /* Transparent colour ignored */
            readWord(f);               /* Aspect ratio ignored */
            readWord(f);               /* page width ignored */
            readWord(f);               /* page height ignored */
            break;

        case CAMG:
            viewmodes = (int)readLong(f); /* Viewmodes */
            if (viewmodes & HAM) {
                colourmapSize = 16;
            }

            break;

        case CMAP:
            colourmapSize = (int)globalChunkHeader.size / 3;
            iffColourMap = new RGBAPixel[colourmapSize];
            if (iffColourMap == nullptr) {
                fprintf(stderr, "Cannot allocate memory for IFF colour map\n");
                exit(1);
            }

            for (i = 0; i < colourmapSize; i++) {
                iffColourMap[i].Red = readByte(f);
                iffColourMap[i].Green = readByte(f);
                iffColourMap[i].Blue = readByte(f);
                iffColourMap[i].Alpha = 0;
            }

            previousRed = iffColourMap[0].Red;
            previousGreen = iffColourMap[0].Green;
            previousBlue = iffColourMap[0].Blue;
            for (i = colourmapSize * 3; (long)i < globalChunkHeader.size; i++) {
                readByte(f);
            }

            break;

        case BODY:
            if ((iffColourMap == nullptr) || (viewmodes & HAM)) {
                image->Colour_Map_Size = 0;
                image->Colour_Map = nullptr;
            } else {
                image->Colour_Map_Size = colourmapSize;
                image->Colour_Map = iffColourMap;
            }
            rowBytes = new unsigned char *[nPlanes];
            if (rowBytes == nullptr) {
                fprintf(stderr, "Cannot allocate memory for row bytes\n");
                exit(1);
            }

            for (i = 0; i < nPlanes; i++) {
                if ((rowBytes[i] =
                            new unsigned char[((image->iwidth + 7) / 8)]) ==
                    nullptr) {
                    fprintf(stderr, "Cannot allocate memory for row bytes\n");
                    exit(1);
                }
            }

            if (image->Colour_Map == nullptr) {
                image->data.rgb_lines = new ImageLine[image->iheight];
                if (image->data.rgb_lines == nullptr) {
                    fprintf(stderr, "Cannot allocate memory for picture\n");
                    exit(1);
                }
            } else {
                image->data.map_lines = new unsigned char *[image->iheight];
                if (image->data.map_lines == nullptr) {
                    fprintf(stderr, "Cannot allocate memory for picture\n");
                    exit(1);
                }
            }

            for (i = 0; i < image->iheight; i++) {

                if (image->Colour_Map == nullptr) {
                    if (((image->data.rgb_lines[i].red =
                                 new unsigned char[image->iwidth]) ==
                            nullptr) ||
                        ((image->data.rgb_lines[i].green =
                                 new unsigned char[image->iwidth]) ==
                            nullptr) ||
                        ((image->data.rgb_lines[i].blue =
                                 new unsigned char[image->iwidth]) ==
                            nullptr)) {
                        fprintf(stderr, "Cannot allocate memory for picture\n");
                        exit(1);
                    }
                } else {
                    image->data.map_lines[i] = new unsigned char[image->iwidth];
                    if (image->data.map_lines[i] == nullptr) {
                        fprintf(stderr, "Cannot allocate memory for picture\n");
                        exit(1);
                    }
                }

                for (j = 0; j < nPlanes; j++) {
                    if (compression == CMPNONE) {
                        for (k = 0; k < (image->iwidth + 7) / 8; k++) {
                            rowBytes[j][k] = (unsigned char)readByte(f);
                        }
                        if ((k & 1) != 0) {
                            readByte(f);
                        }
                    }

                    else {
                        nBytes = 0;
                        while (nBytes != (image->iwidth + 7) / 8) {
                            c = readByte(f);
                            if ((c >= 0) && (c <= 127)) {
                                for (k = 0; k <= c; k++) {
                                    rowBytes[j][nBytes++] =
                                        (unsigned char)readByte(f);
                                }
                            } else if ((c >= 129) && (c <= 255)) {
                                count = 257 - c;
                                c = readByte(f);
                                for (k = 0; k < count; k++) {
                                    rowBytes[j][nBytes++] = (unsigned char)c;
                                }
                            }
                        }
                    }
                }

                mask = 0x80;
                byteIndex = 0;
                for (j = 0; j < image->iwidth; j++) {
                    creg = 0;
                    for (k = nPlanes - 1; k >= 0; k--) {
                        if (rowBytes[k][byteIndex] & mask) {
                            creg = creg * 2 + 1;
                        } else {
                            creg *= 2;
                        }
                    }

                    if (viewmodes & HAM) {
                        line = &image->data.rgb_lines[i];
                        switch (creg >> 4) {
                        case 0:
                            previousRed = line->red[j] =
                                (unsigned char)iffColourMap[creg].Red;
                            previousGreen = line->green[j] =
                                (unsigned char)iffColourMap[creg].Green;
                            previousBlue = line->blue[j] =
                                (unsigned char)iffColourMap[creg].Blue;
                            break;

                        case 1:
                            line->red[j] = (unsigned char)previousRed;
                            line->green[j] = (unsigned char)previousGreen;
                            line->blue[j] =
                                (unsigned char)(((creg & 0xf) << 4) +
                                                (creg & 0xf));
                            previousBlue = (int)line->blue[j];
                            break;

                        case 2:
                            line->red[j] = (unsigned char)(((creg & 0xf) << 4) +
                                                           (creg & 0xf));
                            previousRed = (int)line->red[j];
                            line->green[j] = (unsigned char)previousGreen;
                            line->blue[j] = (unsigned char)previousBlue;
                            break;

                        case 3:
                            line->red[j] = (unsigned char)previousRed;
                            line->green[j] =
                                (unsigned char)(((creg & 0xf) << 4) +
                                                (creg & 0xf));
                            previousGreen = (int)line->green[j];
                            line->blue[j] = (unsigned char)previousBlue;
                            break;
                        }
                    } else if (nPlanes == 24) {
                        line = &image->data.rgb_lines[i];
                        line->red[j] = (unsigned char)((creg >> 16) & 0xFF);
                        line->green[j] = (unsigned char)((creg >> 8) & 0xFF);
                        line->blue[j] = (unsigned char)(creg & 0xFF);
                    } else {
                        if (creg > (unsigned long)image->Colour_Map_Size) {
                            fprintf(stderr,
                                "Error - IFF Image Map Colour out of range\n");
                            exit(1);
                        }
                        image->data.map_lines[i][j] = (char)creg;
                    }

                    mask >>= 1;
                    if (mask == 0) {
                        mask = 0x80;
                        byteIndex++;
                    }
                }
            }

            delete rowBytes;
            fclose(f);
            return;

        default:
            for (i = 0; (long)i < globalChunkHeader.size; i++) {
                if (getc(f) == EOF) {
                    iffError();
                }
            }
            break;
        }
    }
}
