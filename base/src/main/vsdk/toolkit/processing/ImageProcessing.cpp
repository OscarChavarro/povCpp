#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/processing/ImageProcessing.h"

namespace {

int unsignedValue(char c)
{
    return (int)(unsigned char)c;
}

char signedByte(int value)
{
    return (char)(value & 0xFF);
}

int grayLevel(const RGBPixel& p)
{
    return (unsignedValue(p.r) + unsignedValue(p.g) + unsignedValue(p.b)) / 3;
}

}

int ImageProcessing::gammaCorrection8bits(int in, double gamma)
{
    double a;
    double b;
    int out;

    a = ((double)in) / 255.0;
    b = std::pow(a, 1.0/gamma);
    out = (int)(b*255.0);

    return out;
}

void ImageProcessing::gammaCorrection(IndexedColorImageUncompressed* img,
                                      double gamma)
{
    int x;
    int y;
    int val;

    for ( x = 0; x < img->getXSize(); x++ ) {
        for ( y = 0; y < img->getYSize(); y++ ) {
            val = unsignedValue(img->getPixel(x, y));
            val = gammaCorrection8bits(val, gamma);
            img->putPixel(x, y, signedByte(val));
        }
    }
}

void ImageProcessing::gammaCorrection(RGBImageUncompressed* img, double gamma)
{
    int x;
    int y;
    RGBPixel p;
    int r;
    int g;
    int b;

    for ( x = 0; x < img->getXSize(); x++ ) {
        for ( y = 0; y < img->getYSize(); y++ ) {
            img->getPixelRgb(x, y, &p);
            r = gammaCorrection8bits(unsignedValue(p.r), gamma);
            g = gammaCorrection8bits(unsignedValue(p.g), gamma);
            b = gammaCorrection8bits(unsignedValue(p.b), gamma);
            p.r = signedByte(r);
            p.g = signedByte(g);
            p.b = signedByte(b);
            img->putPixelRgb(x, y, &p);
        }
    }
}

void ImageProcessing::resize(Image* input, Image* output)
{
    int xSizeIn = input->getXSize();
    int ySizeIn = input->getYSize();
    int xSizeOut = output->getXSize();
    int ySizeOut = output->getYSize();
    double u;
    double v;
    int x;
    int y;
    RGBPixel target;
    RGBPixel acum;

    if ( xSizeOut == xSizeIn && ySizeOut == ySizeIn ) {
        copy(input, output);
    }
    else if ( xSizeOut > xSizeIn && ySizeOut > ySizeIn ) {
        for ( x = 0; x < xSizeOut; x++ ) {
            for ( y = 0; y < ySizeOut; y++ ) {
                u = ((double)x)/((double)(xSizeOut));
                v = ((double)y)/((double)(ySizeOut));
                ColorRgb* source = input->getColorRgbBiLinear(u, v);
                target.r = signedByte((int)(source->r()*255));
                target.g = signedByte((int)(source->g()*255));
                target.b = signedByte((int)(source->b()*255));
                delete source;
                output->putPixelRgb(x, y, &target);
            }
        }
    }
    else {
        output->init(xSizeOut, ySizeOut);
        double xf = (((double)xSizeIn) / ((double)xSizeOut));
        double yf = (((double)ySizeIn) / ((double)ySizeOut));
        int xfi = (int)xf;
        int yfi = (int)yf;
        double w = (xfi * yfi);
        double acumr;
        double acumg;
        double acumb;

        int xx;
        int yy;
        int x0;
        int y0;
        int x1;
        int y1;
        for ( xx = 0; xx < xSizeOut; xx++ ) {
            for ( yy = 0; yy < ySizeOut; yy++ ) {
                acumr = acumg = acumb = 0.0;

                x0 = (int)(((double)xx)*xf);
                x1 = x0 + xfi;
                for ( x = x0; x < x1 && x < xSizeIn; x++ ) {
                    y0 = (int)(((double)yy)*yf);
                    y1 = y0 + yfi;
                    for ( y = y0; y < y1 && y < ySizeIn; y++ ) {
                        input->getPixelRgb(x, y, &target);
                        acumr += ((double)unsignedValue(target.r)) / w;
                        acumg += ((double)unsignedValue(target.g)) / w;
                        acumb += ((double)unsignedValue(target.b)) / w;
                    }
                }

                if ( acumr >= 255.0 ) acumr = 255.0;
                if ( acumg >= 255.0 ) acumg = 255.0;
                if ( acumb >= 255.0 ) acumb = 255.0;

                acum.r = signedByte((int)(acumr));
                acum.g = signedByte((int)(acumg));
                acum.b = signedByte((int)(acumb));

                output->putPixelRgb(xx, yy, &acum);
            }
        }
    }
}

void ImageProcessing::copy(Image* input, Image* output)
{
    int xSize = input->getXSize();
    int ySize = input->getYSize();
    int x;
    int y;
    RGBPixel target;

    output->init(xSize, ySize);
    for ( x = 0; x < xSize; x++ ) {
        for ( y = 0; y < ySize; y++ ) {
            input->getPixelRgb(x, y, &target);
            output->putPixelRgb(x, y, &target);
        }
    }
}

void ImageProcessing::squareFill(Image* input, Image* output)
{
    int xSize = input->getXSize();
    int ySize = input->getYSize();
    int x;
    int y;
    RGBPixel target;
    int maxSize = xSize;

    if ( ySize > maxSize ) {
        maxSize = ySize;
    }
    output->init(maxSize, maxSize);

    int dx;
    int dy;
    dx = (maxSize-xSize)/2;
    dy = (maxSize-ySize)/2;
    for ( x = 0; x < xSize; x++ ) {
        for ( y = 0; y < ySize; y++ ) {
            input->getPixelRgb(x, y, &target);
            output->putPixelRgb(x+dx, y+dy, &target);
        }
    }
}

void ImageProcessing::frame(Image* input, Image* output, int border)
{
    int xSize = input->getXSize();
    int ySize = input->getYSize();
    int x;
    int y;
    RGBPixel target;

    output->init(xSize+2*border, ySize+2*border);

    for ( x = 0; x < xSize; x++ ) {
        for ( y = 0; y < ySize; y++ ) {
            input->getPixelRgb(x, y, &target);
            output->putPixelRgb(x+border, y+border, &target);
        }
    }
}

void ImageProcessing::extractRoi(Image* source, Image* roi,
    int x0Roi, int y0Roi, int x1Roi, int y1Roi)
{
    int tmp;
    int dx;
    int dy;

    //-----------------------------------------------------------------
    if ( x0Roi > x1Roi ) {
        tmp = x0Roi;
        x0Roi = x1Roi;
        x1Roi = tmp;
    }
    if ( y0Roi > y1Roi ) {
        tmp = y0Roi;
        y0Roi = y1Roi;
        y1Roi = tmp;
    }
    if ( x0Roi < 0 ) x0Roi = 0;
    if ( y0Roi < 0 ) y0Roi = 0;
    if ( x1Roi >= source->getXSize() ) x1Roi = source->getXSize()-1;
    if ( y1Roi >= source->getYSize() ) y1Roi = source->getYSize()-1;
    if ( x0Roi >= source->getXSize() ||
         y0Roi >= source->getYSize()) {
        return;
    }
    dx = x1Roi - x0Roi + 1;
    dy = y1Roi - y0Roi + 1;

    //-----------------------------------------------------------------
    RGBPixel target;
    int x;
    int y;

    roi->init(dx, dy);
    for ( x = 0; x < dx; x++ ) {
        for ( y = 0; y < dy; y++ ) {
            source->getPixelRgb(x0Roi+x, y0Roi+y, &target);
            roi->putPixelRgb(x, y, &target);
        }
    }
}

bool ImageProcessing::processDistanceField(Image* inInput,
    IndexedColorImageUncompressed* outOutput, int threshold)
{
    if ( inInput == nullptr || outOutput == nullptr ) {
        return false;
    }

    int dx = inInput->getXSize();
    int dy = inInput->getYSize();

    if ( dx != outOutput->getXSize() || dy != outOutput->getYSize() ) {
        return false;
    }

    int x;
    int y;
    int xx;
    int yy;
    RGBPixel p;
    double dist2;
    double maxdist2 = ((double)dx)*((double)dx) + ((double)dy)*((double)dy);
    double mindist2;
    double maxdist = std::sqrt(maxdist2);
    int val;

    for ( x = 0; x < dx; x++ ) {
        for ( y = 0; y < dy; y++ ) {
            // Calculate the nearest distance to output (x, y)
            mindist2 = maxdist2;

            for ( xx = 0; xx < dx; xx++ ) {
                for ( yy = 0; yy < dy; yy++ ) {
                    inInput->getPixelRgb(xx, yy, &p);
                    val = grayLevel(p);

                    if ( val >= threshold ) {
                        dist2 =
                     ((double)xx - (double)x) * ((double)xx - (double)x) +
                     ((double)yy - (double)y) * ((double)yy - (double)y);

                        if ( dist2 < mindist2 ) {
                            mindist2 = dist2;
                        }
                    }
                }
            }

            // Set output value to current mindistance
            val = (int)((std::sqrt(mindist2) / maxdist)*255.0);
            outOutput->putPixel(x, y, signedByte(val));
        }
    }

    return true;
}

bool ImageProcessing::processDistanceFieldWithArray(Image* inInput,
    IndexedColorImageUncompressed* outOutput, int threshold)
{
    if ( inInput == nullptr || outOutput == nullptr ) {
        return false;
    }

    int dx = inInput->getXSize();
    int dy = inInput->getYSize();

    if ( dx != outOutput->getXSize() || dy != outOutput->getYSize() ) {
        return false;
    }

    int x;
    int y;
    RGBPixel p;
    int val;

    //- Preprocessing: fill arrays with 0-distance pixel coords. ------
    java::ArrayList<int> xcoords;
    java::ArrayList<int> ycoords;
    for ( x = 0; x < dx; x++ ) {
        for ( y = 0; y < dy; y++ ) {
            inInput->getPixelRgb(x, y, &p);
            val = grayLevel(p);
            if ( val >= threshold ) {
                xcoords.add(x);
                ycoords.add(y);
            }
        }
    }

    //- Optimized distance field algorithm ----------------------------
    int i;
    int arrSize = (int)xcoords.size();
    int xx;
    int yy;
    double dist2;
    double maxdist2 = ((double)dx)*((double)dx) + ((double)dy)*((double)dy);
    double mindist2;
    double maxdist = std::sqrt(maxdist2);

    for ( x = 0; x < dx; x++ ) {
        for ( y = 0; y < dy; y++ ) {
            mindist2 = maxdist2;
            for ( i = 0; i < arrSize; i++ ) {
                xx = xcoords[i];
                yy = ycoords[i];
                dist2 =
                  ((double)xx - (double)x) * ((double)xx - (double)x) +
                  ((double)yy - (double)y) * ((double)yy - (double)y);
                if ( dist2 < mindist2 ) {
                    mindist2 = dist2;
                }
            }
            // Set output value to current mindistance
            val = (int)((std::sqrt(mindist2) / maxdist)*255.0);
            outOutput->putPixel(x, y, signedByte(val));
        }
    }
    return true;
}
