/****************************************************************************
 *                         iff.c
 *
 *  This file implements a simple IFF format file reader.
 *
 *****************************************************************************/

#include "io/image/IffFormat.h"
#include "io/binaryIo/FileLocator.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include <cstdio>
#include <cstdlib>

RGBAPixelHDR *IffFormat::sIffColorMap = nullptr;
int IffFormat::sColorMapSize = 0;
ChunkHeader IffFormat::sGlobalChunkHeader;

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
    Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", "Invalid IFF file\n");
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

IndexedColorImageHDRUncompressed *
IffFormat::readIffImage(RGBAImageHDRUncompressed *directOut, char *filename)
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
    unsigned long creg;

    // Dimensions are written to directOut in all cases so callers can read
    // width/height regardless of which branch is taken.
    int iwidth = 0;
    int iheight = 0;

    java::FileInputStream *fileStream = FileLocator::locateAsStream(filename);
    if (fileStream == nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open IFF file %s\n", filename);
            Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    java::FileInputStream &is = *fileStream;

    previousRed = previousGreen = previousBlue = 0;
    viewmodes = 0;
    sIffColorMap = nullptr;

    while (true) {
        IffFormat::readChunkHeader(is, &sGlobalChunkHeader);
        switch (static_cast<int>(sGlobalChunkHeader.name)) {
        case FORM:
            if (IffFormat::readLong(is) != ILBM) {
                IffFormat::iffError();
            }
            break;

        case BMHD:
            iwidth  = IffFormat::readWord(is);
            iheight = IffFormat::readWord(is);

            IffFormat::readWord(is);
            IffFormat::readWord(is);
            nPlanes = IffFormat::readByte(is);
            sColorMapSize = 1 << nPlanes;
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
                sColorMapSize = 16;
            }
            break;

        case CMAP:
            sColorMapSize = (int)sGlobalChunkHeader.size / 3;
            sIffColorMap = new RGBAPixelHDR[sColorMapSize];
            if (sIffColorMap == nullptr) {
                Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", "Cannot allocate memory for IFF color map\n");
            }

            for (i = 0; i < sColorMapSize; i++) {
                sIffColorMap[i].r   = IffFormat::readByte(is);
                sIffColorMap[i].g = IffFormat::readByte(is);
                sIffColorMap[i].b  = IffFormat::readByte(is);
                sIffColorMap[i].a = 0;
            }

            previousRed   = sIffColorMap[0].r;
            previousGreen = sIffColorMap[0].g;
            previousBlue  = sIffColorMap[0].b;
            for (i = sColorMapSize * 3; (long)i < sGlobalChunkHeader.size; i++) {
                IffFormat::readByte(is);
            }
            break;

        case BODY: {
            const bool isIndexed = (sIffColorMap != nullptr) && !(viewmodes & HAM);

            rowBytes = new unsigned char *[nPlanes];
            if (rowBytes == nullptr) {
                Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", "Cannot allocate memory for row bytes\n");
            }

            for (i = 0; i < nPlanes; i++) {
                if ((rowBytes[i] = new unsigned char[((iwidth + 7) / 8)]) == nullptr) {
                    Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", "Cannot allocate memory for row bytes\n");
                }
            }

            if (isIndexed) {
                // --- Indexed (paletted) path ---
                IndexedColorImageHDRUncompressed *indexed = new IndexedColorImageHDRUncompressed;
                indexed->setColorMapSize(sColorMapSize);
                indexed->setColorMap(sIffColorMap);

                indexed->allocate(iwidth, iheight);

                for (i = 0; i < iheight; i++) {
                    for (j = 0; j < nPlanes; j++) {
                        if (compression == CMPNONE) {
                            for (k = 0; k < (iwidth + 7) / 8; k++) {
                                rowBytes[j][k] = (unsigned char)IffFormat::readByte(is);
                            }
                            if ((k & 1) != 0) {
                                IffFormat::readByte(is);
                            }
                        } else {
                            nBytes = 0;
                            while (nBytes != (iwidth + 7) / 8) {
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
                    for (j = 0; j < iwidth; j++) {
                        creg = 0;
                        for (k = nPlanes - 1; k >= 0; k--) {
                            if (rowBytes[k][byteIndex] & mask) {
                                creg = creg * 2 + 1;
                            } else {
                                creg *= 2;
                            }
                        }

                        if (creg > (unsigned long)indexed->getColorMapSize()) {
                            Logger::reportMessage("IffFormat", Logger::FATAL_ERROR, "", "Error - IFF Image Map Colour out of range\n");
                        }
                        indexed->setPixel(j, i, (unsigned char)creg);

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
                return indexed;

            } else {
                // --- Direct-color path (HAM or 24-plane) ---
                directOut->allocate(iwidth, iheight);

                for (i = 0; i < iheight; i++) {
                    for (j = 0; j < nPlanes; j++) {
                        if (compression == CMPNONE) {
                            for (k = 0; k < (iwidth + 7) / 8; k++) {
                                rowBytes[j][k] = (unsigned char)IffFormat::readByte(is);
                            }
                            if ((k & 1) != 0) {
                                IffFormat::readByte(is);
                            }
                        } else {
                            nBytes = 0;
                            while (nBytes != (iwidth + 7) / 8) {
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
                    for (j = 0; j < iwidth; j++) {
                        creg = 0;
                        for (k = nPlanes - 1; k >= 0; k--) {
                            if (rowBytes[k][byteIndex] & mask) {
                                creg = creg * 2 + 1;
                            } else {
                                creg *= 2;
                            }
                        }

                        RGBAPixelHDR pixel;
                        pixel.a = 0;

                        if (viewmodes & HAM) {
                            switch (creg >> 4) {
                            case 0:
                                pixel.r = (unsigned short)sIffColorMap[creg].r;
                                pixel.g = (unsigned short)sIffColorMap[creg].g;
                                pixel.b = (unsigned short)sIffColorMap[creg].b;
                                previousRed   = pixel.r;
                                previousGreen = pixel.g;
                                previousBlue  = pixel.b;
                                break;
                            case 1:
                                pixel.r = (unsigned short)previousRed;
                                pixel.g = (unsigned short)previousGreen;
                                pixel.b = (unsigned short)(((creg & 0xf) << 4) + (creg & 0xf));
                                previousBlue = pixel.b;
                                break;
                            case 2:
                                pixel.r = (unsigned short)(((creg & 0xf) << 4) + (creg & 0xf));
                                pixel.g = (unsigned short)previousGreen;
                                pixel.b = (unsigned short)previousBlue;
                                previousRed = pixel.r;
                                break;
                            case 3:
                                pixel.r = (unsigned short)previousRed;
                                pixel.g = (unsigned short)(((creg & 0xf) << 4) + (creg & 0xf));
                                pixel.b = (unsigned short)previousBlue;
                                previousGreen = pixel.g;
                                break;
                            default:
                                pixel.r = pixel.g = pixel.b = 0;
                                break;
                            }
                        } else {
                            // 24-plane direct-color
                            pixel.r = (unsigned short)((creg >> 16) & 0xFF);
                            pixel.g = (unsigned short)((creg >> 8) & 0xFF);
                            pixel.b = (unsigned short)(creg & 0xFF);
                        }
                        directOut->setPixel(j, i, pixel);

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
                return nullptr;
            }
        }

        default:
            for (i = 0; (long)i < sGlobalChunkHeader.size; i++) {
                if (is.read() == -1) {
                    IffFormat::iffError();
                }
            }
            break;
        }
    }
}
