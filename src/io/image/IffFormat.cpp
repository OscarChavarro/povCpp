/****************************************************************************
 *                         iff.c
 *
 *  This file implements a simple IFF format file reader.
 *
 *****************************************************************************/

#include "io/image/IffFormat.h"
#include "io/FileLocator.h"
#include "common/LegacyBoolean.h"

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
IffFormat::iffError()
{
    Logger::error("Invalid IFF file\n");
    exit(1);
}

int
IffFormat::readByte(java::FileInputStream &is)
{
    int c = is.read();
    if (c == -1) {
        IffFormat::iffError();
    }
    return c;
}

int
IffFormat::readWord(java::FileInputStream &is)
{
    int result = IffFormat::readByte(is) * 256;
    result += IffFormat::readByte(is);
    return result;
}

long
IffFormat::readLong(java::FileInputStream &is)
{
    long result = 0;
    for (int i = 0; i < 4; i++) {
        result = result * 256 + IffFormat::readByte(is);
    }
    return result;
}

void
IffFormat::readChunkHeader(java::FileInputStream &is, ChunkHeader *dest)
{
    dest->name = IffFormat::readLong(is);
    dest->size = IffFormat::readLong(is);
}

void
IffFormat::readIffImage(RGBAImage *image, char *filename)
{
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

    java::FileInputStream *fileStream = FileLocator::locateAsStream(filename);
    if (fileStream == nullptr) {
        Logger::error("Cannot open IFF file %s\n", filename);
        exit(1);
    }

    java::FileInputStream &is = *fileStream;

    previousRed = previousGreen = previousBlue = 0;
    viewmodes = 0;
    iffColourMap = nullptr;

    while (true) {
        IffFormat::readChunkHeader(is, &globalChunkHeader);
        switch (static_cast<int>(globalChunkHeader.name)) {
        case FORM:
            if (IffFormat::readLong(is) != ILBM) {
                IffFormat::iffError();
            }
            break;

        case BMHD:
            image->iwidth  = IffFormat::readWord(is);
            image->width   = (double)image->iwidth;
            image->iheight = IffFormat::readWord(is);
            image->height  = (double)image->iheight;

            IffFormat::readWord(is);
            IffFormat::readWord(is);
            nPlanes = IffFormat::readByte(is);
            colourmapSize = 1 << nPlanes;
            IffFormat::readByte(is);
            compression = IffFormat::readByte(is);
            IffFormat::readByte(is);
            IffFormat::readWord(is);
            IffFormat::readWord(is);
            IffFormat::readWord(is);
            IffFormat::readWord(is);
            break;

        case CAMG:
            viewmodes = (int)IffFormat::readLong(is);
            if (viewmodes & HAM) {
                colourmapSize = 16;
            }
            break;

        case CMAP:
            colourmapSize = (int)globalChunkHeader.size / 3;
            iffColourMap = new RGBAPixel[colourmapSize];
            if (iffColourMap == nullptr) {
                Logger::error("Cannot allocate memory for IFF colour map\n");
                exit(1);
            }

            for (i = 0; i < colourmapSize; i++) {
                iffColourMap[i].Red   = IffFormat::readByte(is);
                iffColourMap[i].Green = IffFormat::readByte(is);
                iffColourMap[i].Blue  = IffFormat::readByte(is);
                iffColourMap[i].Alpha = 0;
            }

            previousRed   = iffColourMap[0].Red;
            previousGreen = iffColourMap[0].Green;
            previousBlue  = iffColourMap[0].Blue;
            for (i = colourmapSize * 3; (long)i < globalChunkHeader.size; i++) {
                IffFormat::readByte(is);
            }
            break;

        case BODY:
            if ((iffColourMap == nullptr) || (viewmodes & HAM)) {
                image->colourMapSize = 0;
                image->Colour_Map = nullptr;
            } else {
                image->colourMapSize = colourmapSize;
                image->Colour_Map = iffColourMap;
            }
            rowBytes = new unsigned char *[nPlanes];
            if (rowBytes == nullptr) {
                Logger::error("Cannot allocate memory for row bytes\n");
                exit(1);
            }

            for (i = 0; i < nPlanes; i++) {
                if ((rowBytes[i] = new unsigned char[((image->iwidth + 7) / 8)]) == nullptr) {
                    Logger::error("Cannot allocate memory for row bytes\n");
                    exit(1);
                }
            }

            if (image->Colour_Map == nullptr) {
                image->data.rgb_lines = new ImageLine[image->iheight];
                if (image->data.rgb_lines == nullptr) {
                    Logger::error("Cannot allocate memory for picture\n");
                    exit(1);
                }
            } else {
                image->data.map_lines = new unsigned char *[image->iheight];
                if (image->data.map_lines == nullptr) {
                    Logger::error("Cannot allocate memory for picture\n");
                    exit(1);
                }
            }

            for (i = 0; i < image->iheight; i++) {
                if (image->Colour_Map == nullptr) {
                    if (((image->data.rgb_lines[i].red   = new unsigned char[image->iwidth]) == nullptr) ||
                        ((image->data.rgb_lines[i].green = new unsigned char[image->iwidth]) == nullptr) ||
                        ((image->data.rgb_lines[i].blue  = new unsigned char[image->iwidth]) == nullptr)) {
                        Logger::error("Cannot allocate memory for picture\n");
                        exit(1);
                    }
                } else {
                    image->data.map_lines[i] = new unsigned char[image->iwidth];
                    if (image->data.map_lines[i] == nullptr) {
                        Logger::error("Cannot allocate memory for picture\n");
                        exit(1);
                    }
                }

                for (j = 0; j < nPlanes; j++) {
                    if (compression == CMPNONE) {
                        for (k = 0; k < (image->iwidth + 7) / 8; k++) {
                            rowBytes[j][k] = (unsigned char)IffFormat::readByte(is);
                        }
                        if ((k & 1) != 0) {
                            IffFormat::readByte(is);
                        }
                    } else {
                        nBytes = 0;
                        while (nBytes != (image->iwidth + 7) / 8) {
                            c = IffFormat::readByte(is);
                            if ((c >= 0) && (c <= 127)) {
                                for (k = 0; k <= c; k++) {
                                    rowBytes[j][nBytes++] = (unsigned char)IffFormat::readByte(is);
                                }
                            } else if ((c >= 129) && (c <= 255)) {
                                count = 257 - c;
                                c = IffFormat::readByte(is);
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
                            previousRed   = line->red[j]   = (unsigned char)iffColourMap[creg].Red;
                            previousGreen = line->green[j] = (unsigned char)iffColourMap[creg].Green;
                            previousBlue  = line->blue[j]  = (unsigned char)iffColourMap[creg].Blue;
                            break;
                        case 1:
                            line->red[j]   = (unsigned char)previousRed;
                            line->green[j] = (unsigned char)previousGreen;
                            line->blue[j]  = (unsigned char)(((creg & 0xf) << 4) + (creg & 0xf));
                            previousBlue   = (int)line->blue[j];
                            break;
                        case 2:
                            line->red[j]   = (unsigned char)(((creg & 0xf) << 4) + (creg & 0xf));
                            previousRed    = (int)line->red[j];
                            line->green[j] = (unsigned char)previousGreen;
                            line->blue[j]  = (unsigned char)previousBlue;
                            break;
                        case 3:
                            line->red[j]   = (unsigned char)previousRed;
                            line->green[j] = (unsigned char)(((creg & 0xf) << 4) + (creg & 0xf));
                            previousGreen  = (int)line->green[j];
                            line->blue[j]  = (unsigned char)previousBlue;
                            break;
                        }
                    } else if (nPlanes == 24) {
                        line = &image->data.rgb_lines[i];
                        line->red[j]   = (unsigned char)((creg >> 16) & 0xFF);
                        line->green[j] = (unsigned char)((creg >> 8) & 0xFF);
                        line->blue[j]  = (unsigned char)(creg & 0xFF);
                    } else {
                        if (creg > (unsigned long)image->colourMapSize) {
                            Logger::error("Error - IFF Image Map Colour out of range\n");
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
            fileStream->close();
            delete fileStream;
            return;

        default:
            for (i = 0; (long)i < globalChunkHeader.size; i++) {
                if (is.read() == -1) {
                    IffFormat::iffError();
                }
            }
            break;
        }
    }
}
