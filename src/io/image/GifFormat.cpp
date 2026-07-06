/**
gif.c

Gif-format file reader.
*/

#include <cstdlib>
#include <cstring>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/image/GifDecoder.h"
#include "io/image/GifFormat.h"

int
GifFormat::outLine(void *context, const unsigned char *pixels, int linelen)
{
    GifReadContext &gif = *static_cast<GifReadContext *>(context);
    for (int x = 0; x < linelen; x++) {
        if ((int)(*pixels) > gif.getCurrentImage()->getColorMapSize()) {
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Error - GIF Image Map Colour out of range\n");
        }
        gif.getCurrentImage()->setPixel(x, gif.getBitmapLine(), *pixels);
        pixels++;
    }
    gif.incrementBitmapLine();
    return 0;
}

int
GifFormat::getByte(void *context)
{
    GifReadContext &gif = *static_cast<GifReadContext *>(context);
    const int byte = gif.getBitStream()->read();
    if (byte != -1) {
        return byte;
    }
    Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", "Premature End Of File reading GIF image\n");
    return 0;
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
    GifReadContext gif(image, 0, nullptr, nullptr, 0);
    GifInputContext input{getByte, outLine, &gif};
    const long codeMask[13] = {0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F,
        0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF};
    GifDecoderState decoderState(nullptr, codeMask, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, nullptr, nullptr, nullptr, nullptr);

    {
        java::File * const located = locator.locate(filename);
        if (located != nullptr) {
            gif.setBitStream(new java::FileInputStream(located->getPath().toCString()));
            delete located;
        }
    }
    if (gif.getBitStream() == nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open GIF file %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    decoderState.setDecoderline(new unsigned char[2049]);
    if (decoderState.getDecoderline() == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate space for GIF decoder line\n");
        gif.getBitStream()->close();
        delete gif.getBitStream();
        exit(1);
    }

    for (i = 0; i < 2049; i++) {
        decoderState.getDecoderline()[i] = (unsigned char)0;
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
        gif.getBitStream()->close();
        delete gif.getBitStream();
        exit(1);
    }

    planes = ((unsigned)buffer[10] & 0x0F) + 1;
    gif.setColorMapSize((int)(1 << planes));

    gif.setGifColorMap(new RGBAPixelHDR[gif.getColorMapSize()]);
    if (gif.getGifColorMap() == nullptr) {
        Logger::reportMessage("GifFormat", Logger::ERROR, "", "Cannot allocate GIF Colour Map\n");
        gif.getBitStream()->close();
        delete gif.getBitStream();
        exit(1);
    }

    if ((buffer[10] & 0x80) == 0) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Invalid GIF file format: %s\n", filename);
            Logger::reportMessage("GifFormat", Logger::ERROR, "", _logMsg);
        }
        gif.getBitStream()->close();
        delete gif.getBitStream();
        exit(1);
    }

    for (i = 0; i < gif.getColorMapSize(); i++) {
        gif.getGifColorMap()[i].r = (unsigned char)getByte(&gif);
        gif.getGifColorMap()[i].g = (unsigned char)getByte(&gif);
        gif.getGifColorMap()[i].b = (unsigned char)getByte(&gif);
        gif.getGifColorMap()[i].a = 0;
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

                gif.setBitmapLine(0);
                image->setColorMapSize(gif.getColorMapSize());
                image->setColorTable(gif.getGifColorMap());
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

    delete[] decoderState.getDecoderline();
    decoderState.setDecoderline(nullptr);
    gif.getBitStream()->close();
    delete gif.getBitStream();
    gif.setBitStream(nullptr);
}
