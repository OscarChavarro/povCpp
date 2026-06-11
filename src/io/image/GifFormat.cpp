/**
gif.c

Gif-format file reader.
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/binaryIo/FileLocator.h"
#include "io/image/GifDecoder.h"
#include "io/image/GifFormat.h"

IndexedColorImageHDRUncompressed *GifFormat::currentImage = nullptr;
int GifFormat::bitmapLine = 0;
java::FileInputStream *GifFormat::bitStream = nullptr;
RGBAPixelHDR *GifFormat::gifColorMap = nullptr;
int GifFormat::colorMapSize = 0;

int
GifFormat::outLine(unsigned char *pixels, int linelen)
{
    for (int x = 0; x < linelen; x++) {
        if ((int)(*pixels) > currentImage->getColorMapSize()) {
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Error - GIF Image Map Colour out of range\n");
        }
        currentImage->setPixel(x, bitmapLine, *pixels);
        pixels++;
    }
    bitmapLine++;
    return 0;
}

int
GifFormat::getByte()
{
    int byte = bitStream->read();
    if (byte != -1) {
        return byte;
    }
    Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Premature End Of File reading GIF image\n");
    return 0;
}

void
GifFormat::readGifImage(IndexedColorImageHDRUncompressed *image, char *filename)
{
    int i;
    int j;
    int status = 0;
    unsigned finished;
    unsigned planes;
    unsigned char buffer[16];

    currentImage = image;

    bitStream = FileLocator::locateAsStream(filename);
    if (bitStream == nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open GIF file %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    GifDecoder::decoderline = new unsigned char[2049];
    if (GifDecoder::decoderline == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate space for GIF decoder line\n");
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    for (i = 0; i < 2049; i++) {
        GifDecoder::decoderline[i] = (unsigned char)0;
    }

    for (i = 0; i < 13; i++) {
        buffer[i] = (unsigned char)GifFormat::getByte();
    }

    if (strncmp((char *)buffer, "GIF", 3) ||
        buffer[3] < '0' || buffer[3] > '9' || buffer[4] < '0' ||
        buffer[4] > '9' || buffer[5] < 'A' || buffer[5] > 'z') {

        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Invalid GIF file format: %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::ERROR, "", _logMsg);
        }
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    planes = ((unsigned)buffer[10] & 0x0F) + 1;
    colorMapSize = (int)(1 << planes);

    gifColorMap = new RGBAPixelHDR[colorMapSize];
    if (gifColorMap == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate GIF Colour Map\n");
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    if ((buffer[10] & 0x80) == 0) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Invalid GIF file format: %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::ERROR, "", _logMsg);
        }
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    for (i = 0; i < colorMapSize; i++) {
        gifColorMap[i].r   = (unsigned char)GifFormat::getByte();
        gifColorMap[i].g = (unsigned char)GifFormat::getByte();
        gifColorMap[i].b  = (unsigned char)GifFormat::getByte();
        gifColorMap[i].a = 0;
    }

    finished = false;
    while (!finished) {
        switch (GifFormat::getByte()) {
        case ';':
            finished = true;
            status = 0;
            break;

        case '!':
            GifFormat::getByte();
            while ((i = GifFormat::getByte()) > 0) {
                for (j = 0; j < i; j++) {
                    GifFormat::getByte();
                }
            }
            break;

        case ',':
            for (i = 0; i < 9; i++) {
                if ((j = GifFormat::getByte()) < 0) {
                    status = -1;
                    break;
                }
                buffer[i] = (unsigned char)j;
            }

            if (status < 0) {
                finished = true;
                break;
            }

            {
                int gifW = buffer[4] | (buffer[5] << 8);
                int gifH = buffer[6] | (buffer[7] << 8);

                bitmapLine = 0;
                image->setColorMapSize(colorMapSize);
                image->setColorTable(gifColorMap);
                image->allocate(gifW, gifH);

                status = GifDecoder::decoder(image->getXSize());
            }
            finished = true;
            break;

        default:
            status = -1;
            finished = true;
            break;
        }
    }

    delete GifDecoder::decoderline;
    GifDecoder::decoderline = nullptr;
    bitStream->close();
    delete bitStream;
    bitStream = nullptr;
}
