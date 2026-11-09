#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/Calligraphic2DBuffer.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
Calligraphic2DBuffer::Calligraphic2DBuffer() {
}

void Calligraphic2DBuffer::init() {
    lineData.clear();
}

void Calligraphic2DBuffer::add2DLine(const Vector3Dd& p0, const Vector3Dd& p1) {
    add2DLine(p0.x(), p0.y(), p1.x(), p1.y());
}

void Calligraphic2DBuffer::add2DLine(double x0, double y0, double x1, double y1) {
    lineData.add(x0);
    lineData.add(y0);
    lineData.add(x1);
    lineData.add(y1);
    lineData.add(0.0);     // R
    lineData.add(0.0);     // G
    lineData.add(0.0);     // B
    lineData.add(1.0);     // Width
}

Vector3Dd* Calligraphic2DBuffer::get2DLinePoint0(int i) const {
    if (i < 0 || i >= getNumLines()) {
        return nullptr;
    }
    return new Vector3Dd(lineData.get(8*i), lineData.get(8*i+1), 0.0);
}

Vector3Dd* Calligraphic2DBuffer::get2DLinePoint1(int i) const {
    if (i < 0 || i >= getNumLines()) {
        return nullptr;
    }
    return new Vector3Dd(lineData.get(8*i+2), lineData.get(8*i+3), 0.0);
}

int Calligraphic2DBuffer::getNumLines() const {
    return (int)lineData.size() / 8;
}

void Calligraphic2DBuffer::exportRgbImage(RGBImageUncompressed* inOutRasterViewport) {
    double xt = inOutRasterViewport->getXSize();
    double yt = inOutRasterViewport->getYSize();

    Vector3Dd* e0;
    Vector3Dd* e1;
    int x0, y0, x1, y1;
    RGBPixel pixel;

    pixel.r = (char)255;
    pixel.g = (char)255;
    pixel.b = (char)255;

    for (int j = 0; j < getNumLines(); j++) {
        e0 = get2DLinePoint0(j);
        e1 = get2DLinePoint1(j);

        x0 = (int)((xt - 1) * ((e0->x() + 1.0) / 2.0));
        y0 = (int)((yt - 1) * (1.0 - ((e0->y() + 1.0) / 2.0)));
        x1 = (int)((xt - 1) * ((e1->x() + 1.0) / 2.0));
        y1 = (int)((yt - 1) * (1.0 - ((e1->y() + 1.0) / 2.0)));

        delete e0;
        delete e1;
    }
}
