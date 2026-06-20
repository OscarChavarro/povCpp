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

namespace {

struct GifReadContext {
    IndexedColorImageHDRUncompressed *currentImage;
    int bitmapLine;
    java::FileInputStream *bitStream;
    RGBAPixelHDR *gifColorMap;
    int colorMapSize;
};

int
outLine(void *context, const unsigned char *pixels, int linelen)
{
    GifReadContext &gif = *static_cast<GifReadContext *>(context);
    for (int x = 0; x < linelen; x++) {
        if ((int)(*pixels) > gif.currentImage->getColorMapSize()) {
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Error - GIF Image Map Colour out of range\n");
        }
        gif.currentImage->setPixel(x, gif.bitmapLine, *pixels);
        pixels++;
    }
    gif.bitmapLine++;
    return 0;
}

int
getByte(void *context)
{
    GifReadContext &gif = *static_cast<GifReadContext *>(context);
    const int byte = gif.bitStream->read();
    if (byte != -1) {
        return byte;
    }
    Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Premature End Of File reading GIF image\n");
    return 0;
}

}

void
GifFormat::readGifImage(IndexedColorImageHDRUncompressed *image, const char *filename,
    const FileLocator &locator)
{
    int i;
    int j;
    int status = 0;
    unsigned finished;
    unsigned planes;
    unsigned char buffer[16];
    GifReadContext gif{image, 0, nullptr, nullptr, 0};
    GifInputContext input{getByte, outLine, &gif};
    GifDecoderState decoderState{nullptr, {0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F,
        0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        {0}, nullptr, nullptr, nullptr, nullptr};

    gif.bitStream = locator.locateAsStream(filename);
    if (gif.bitStream == nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open GIF file %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    decoderState.decoderline = new unsigned char[2049];
    if (decoderState.decoderline == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate space for GIF decoder line\n");
        gif.bitStream->close();
        delete gif.bitStream;
        exit(1);
    }

    for (i = 0; i < 2049; i++) {
        decoderState.decoderline[i] = (unsigned char)0;
    }

    for (i = 0; i < 13; i++) {
        buffer[i] = (unsigned char)getByte(&gif);
    }

    if (strncmp((char *)buffer, "GIF", 3) ||
        buffer[3] < '0' || buffer[3] > '9' || buffer[4] < '0' ||
        buffer[4] > '9' || buffer[5] < 'A' || buffer[5] > 'z') {

        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Invalid GIF file format: %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::ERROR, "", _logMsg);
        }
        gif.bitStream->close();
        delete gif.bitStream;
        exit(1);
    }

    planes = ((unsigned)buffer[10] & 0x0F) + 1;
    gif.colorMapSize = (int)(1 << planes);

    gif.gifColorMap = new RGBAPixelHDR[gif.colorMapSize];
    if (gif.gifColorMap == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate GIF Colour Map\n");
        gif.bitStream->close();
        delete gif.bitStream;
        exit(1);
    }

    if ((buffer[10] & 0x80) == 0) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Invalid GIF file format: %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::ERROR, "", _logMsg);
        }
        gif.bitStream->close();
        delete gif.bitStream;
        exit(1);
    }

    for (i = 0; i < gif.colorMapSize; i++) {
        gif.gifColorMap[i].r = (unsigned char)getByte(&gif);
        gif.gifColorMap[i].g = (unsigned char)getByte(&gif);
        gif.gifColorMap[i].b = (unsigned char)getByte(&gif);
        gif.gifColorMap[i].a = 0;
    }

    finished = false;
    while (!finished) {
        switch (getByte(&gif)) {
        case ';':
            finished = true;
            status = 0;
            break;

        case '!':
            getByte(&gif);
            while ((i = getByte(&gif)) > 0) {
                for (j = 0; j < i; j++) {
                    getByte(&gif);
                }
            }
            break;

        case ',':
            for (i = 0; i < 9; i++) {
                if ((j = getByte(&gif)) < 0) {
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
                const int gifW = buffer[4] | (buffer[5] << 8);
                const int gifH = buffer[6] | (buffer[7] << 8);

                gif.bitmapLine = 0;
                image->setColorMapSize(gif.colorMapSize);
                image->setColorTable(gif.gifColorMap);
                image->allocate(gifW, gifH);

                status = GifDecoder::decoder(image->getXSize(), input, decoderState);
            }
            finished = true;
            break;

        default:
            status = -1;
            finished = true;
            break;
        }
    }

    delete[] decoderState.decoderline;
    decoderState.decoderline = nullptr;
    gif.bitStream->close();
    delete gif.bitStream;
    gif.bitStream = nullptr;
}
