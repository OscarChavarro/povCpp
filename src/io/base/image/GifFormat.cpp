/****************************************************************************
 *                         gif.c
 *
 *  Gif-format file reader.
 *
 *****************************************************************************/

#include "io/base/image/GifFormat.h"
#include "io/base/FileLocator.h"
#include "common/LegacyBoolean.h"
#include "io/base/image/GifDecoder.h"
#include <cstring>
#include <string>
#include <stdexcept>

RGBAImage *GifFormat::currentImage = nullptr;
int GifFormat::bitmapLine = 0;
java::FileInputStream *GifFormat::bitStream = nullptr;
RGBAPixel *GifFormat::gifColourMap = nullptr;
int GifFormat::colourmapSize = 0;

int
GifFormat::outLine(unsigned char *pixels, int linelen)
{
    unsigned char *line = currentImage->data.map_lines[bitmapLine++];

    for (int x = 0; x < linelen; x++) {
        if ((int)(*pixels) > currentImage->colourMapSize) {
            Logger::error("Error - GIF Image Map Colour out of range\n");
            exit(1);
        }
        line[x] = *pixels;
        pixels++;
    }
    return 0;
}

int
GifFormat::getByte()
{
    int byte = bitStream->read();
    if (byte != -1) {
        return byte;
    }
    Logger::error("Premature End Of File reading GIF image\n");
    exit(1);
    return 0;
}

void
GifFormat::readGifImage(RGBAImage *image, char *filename)
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
        std::string errmsg = "Cannot open GIF file " + std::string(filename);
        throw std::runtime_error(errmsg);
    }

    GifDecoder::decoderline = new unsigned char[2049];
    if (GifDecoder::decoderline == nullptr) {
        Logger::error("Cannot allocate space for GIF decoder line\n");
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

        Logger::error("Invalid GIF file format: %s\n", filename);
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    planes = ((unsigned)buffer[10] & 0x0F) + 1;
    colourmapSize = (int)(1 << planes);

    gifColourMap = new RGBAPixel[colourmapSize];
    if (gifColourMap == nullptr) {
        Logger::error("Cannot allocate GIF Colour Map\n");
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    if ((buffer[10] & 0x80) == 0) {
        Logger::error("Invalid GIF file format: %s\n", filename);
        bitStream->close();
        delete bitStream;
        exit(1);
    }

    for (i = 0; i < colourmapSize; i++) {
        gifColourMap[i].Red   = (unsigned char)GifFormat::getByte();
        gifColourMap[i].Green = (unsigned char)GifFormat::getByte();
        gifColourMap[i].Blue  = (unsigned char)GifFormat::getByte();
        gifColourMap[i].Alpha = 0;
    }

    finished = LegacyBoolean::FALSE_VALUE;
    while (!finished) {
        switch (GifFormat::getByte()) {
        case ';':
            finished = LegacyBoolean::TRUE_VALUE;
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
                finished = LegacyBoolean::TRUE_VALUE;
                break;
            }

            image->iwidth  = buffer[4] | (buffer[5] << 8);
            image->iheight = buffer[6] | (buffer[7] << 8);
            image->width   = (double)image->iwidth;
            image->height  = (double)image->iheight;

            bitmapLine = 0;
            image->colourMapSize = colourmapSize;
            image->Colour_Map = gifColourMap;

            image->data.map_lines = new unsigned char *[image->iheight];
            if (image->data.map_lines == nullptr) {
                Logger::error("Cannot allocate memory for picture\n");
                exit(1);
            }

            for (i = 0; i < image->iheight; i++) {
                image->data.map_lines[i] = new unsigned char[image->iwidth];
                if (image->data.map_lines[i] == nullptr) {
                    Logger::error("Cannot allocate memory for picture\n");
                    exit(1);
                }
            }

            status = GifDecoder::decoder(image->iwidth);
            finished = LegacyBoolean::TRUE_VALUE;
            break;

        default:
            status = -1;
            finished = LegacyBoolean::TRUE_VALUE;
            break;
        }
    }

    delete GifDecoder::decoderline;
    GifDecoder::decoderline = nullptr;
    bitStream->close();
    delete bitStream;
    bitStream = nullptr;
}
