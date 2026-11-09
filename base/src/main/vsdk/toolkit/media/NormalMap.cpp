#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
static int positiveMod(int value, int modulus) {
    int result = value % modulus;
    if (result < 0) {
        result += modulus;
    }
    return result;
}

NormalMap::NormalMap() : xSize(0), ySize(0), bumpMapScale(nullptr) {
    bumpMapScale = new Vector3Dd(1.0, 1.0, 1.0);
}

NormalMap::NormalMap(const NormalMap& other) : xSize(0), ySize(0), bumpMapScale(nullptr)
{
    bumpMapScale = new Vector3Dd(1.0, 1.0, 1.0);
    if (other.bumpMapScale != nullptr) {
        setBumpMapScale(*other.bumpMapScale);
    }
    init(other.xSize, other.ySize);
    for (int y = 0; y < ySize; y++) {
        for (int x = 0; x < xSize; x++) {
            Vector3Dd* n = other.getNormal(x, y);
            if (n != nullptr) {
                putNormal(x, y, *n);
                delete n;
            }
        }
    }
}

NormalMap::~NormalMap() {
    for (long int i = 0; i < data.size(); i++) {
        if (data[i] != nullptr) {
            delete data[i];
            data[i] = nullptr;
        }
    }
    data.clear();

    if (bumpMapScale != nullptr) {
        delete bumpMapScale;
        bumpMapScale = nullptr;
    }
}

bool NormalMap::init(int width, int height) {
    try {
        for (long int i = 0; i < data.size(); i++) {
            if (data[i] != nullptr) {
                delete data[i];
            }
        }
        data.clear();

        for (int i = 0; i < width * height; i++) {
            data.add(new Vector3Dd());
        }
    } catch (...) {
        return false;
    }
    xSize = width;
    ySize = height;
    return true;
}

int NormalMap::getXSize() const {
    return xSize;
}

int NormalMap::getYSize() const {
    return ySize;
}

Vector3Dd* NormalMap::getBumpMapScale() const {
    if (bumpMapScale != nullptr) {
        return new Vector3Dd(*bumpMapScale);
    }
    return nullptr;
}

void NormalMap::setBumpMapScale(const Vector3Dd& scale) {
    if (bumpMapScale != nullptr) {
        delete bumpMapScale;
    }
    bumpMapScale = new Vector3Dd(scale);
}

void NormalMap::putNormal(int i, int j, const Vector3Dd& n) {
    if (i < 0 || j < 0 || i >= xSize || j >= ySize) {
        return;
    }
    int index = j * xSize + i;
    if (data[index] != nullptr) {
        delete data[index];
    }
    data[index] = new Vector3Dd(n);
}

Vector3Dd* NormalMap::getNormal(int u, int v) const {
    if (u < 0 || v < 0 || u >= xSize || v >= ySize) {
        return nullptr;
    }
    int index = v * xSize + u;
    if (data.get(index) != nullptr) {
        return new Vector3Dd(*data.get(index));
    }
    return nullptr;
}

Vector3Dd* NormalMap::getNormalBiLinear(double x, double y) const {
    double u = x - std::floor(x);
    double v = y - std::floor(y);
    int width = getXSize();
    int height = getYSize();
    double U = u * ((double)width);
    double V = v * ((double)height);
    int i0 = positiveMod((int)std::floor(U), width);
    int j0 = positiveMod((int)std::floor(V), height);
    int i1 = positiveMod(i0 + 1, width);
    int j1 = positiveMod(j0 + 1, height);
    double du = U - (double)i0;
    double dv = V - (double)j0;

    Vector3Dd* F00 = getNormal(i0, j0);
    Vector3Dd* F01 = getNormal(i0, j1);
    Vector3Dd* F10 = getNormal(i1, j0);
    Vector3Dd* F11 = getNormal(i1, j1);

    Vector3Dd* FU0 = new Vector3Dd(F00->add(F10->subtract(*F00).multiply(du)));
    Vector3Dd* FU1 = new Vector3Dd(F01->add(F11->subtract(*F01).multiply(du)));
    Vector3Dd* FVAL = new Vector3Dd(FU0->add(FU1->subtract(*FU0).multiply(dv)));

    delete F00;
    delete F01;
    delete F10;
    delete F11;
    delete FU0;
    delete FU1;

    return FVAL;
}

RGBImageUncompressed* NormalMap::exportToRgbImage() const
{
    RGBImageUncompressed* output = new RGBImageUncompressed();
    if (!output->init(xSize, ySize)) {
        delete output;
        return nullptr;
    }

    for (int y = 0; y < ySize; y++) {
        for (int x = 0; x < xSize; x++) {
            Vector3Dd* n = getNormal(x, y);
            if (n == nullptr) {
                output->putPixel(x, y, (char)127, (char)127, (char)255);
                continue;
            }
            Vector3Dd nn = n->normalized();
            delete n;
            Vector3Dd mapped((nn.x() + 1.0) * 0.5, (nn.y() + 1.0) * 0.5, (nn.z() + 1.0) * 0.5);
            int rr = (int)(mapped.x() * 255.0);
            int gg = (int)(mapped.y() * 255.0);
            int bb = (int)(mapped.z() * 255.0);
            if (rr < 0) rr += 256;
            if (gg < 0) gg += 256;
            if (bb < 0) bb += 256;
            output->putPixel(
                x,
                y,
                (char)(unsigned char)rr,
                (char)(unsigned char)gg,
                (char)(unsigned char)bb);
        }
    }
    return output;
}

Vector3Dd NormalMap::importBumpMap(IndexedColorImageUncompressed* inBumpmap, const Vector3Dd& inScale)
{
    if (inBumpmap == nullptr) {
        return Vector3Dd(1.0, 1.0, 1.0);
    }

    int xxSize = inBumpmap->getXSize();
    int yySize = inBumpmap->getYSize();
    Vector3Dd scale = inScale;

    if (scale.x() < VSDK::EPSILON || scale.y() < VSDK::EPSILON || scale.z() < VSDK::EPSILON) {
        double val = ((double)xxSize) / ((double)yySize);
        if (val < 1.0) {
            scale = Vector3Dd(1.0, 1.0 / val, 1.0);
        }
        else {
            scale = Vector3Dd(val, 1.0, 1.0);
        }
    }
    setBumpMapScale(scale);
    init(xxSize, yySize);

    for (int u = 1; u < xxSize - 1; u++) {
        for (int v = 1; v < yySize - 1; v++) {
            int a = (unsigned char)inBumpmap->getPixel(u + 1, v);
            int b = (unsigned char)inBumpmap->getPixel(u - 1, v);
            int c = (unsigned char)inBumpmap->getPixel(u, v + 1);
            int d = (unsigned char)inBumpmap->getPixel(u, v - 1);

            Vector3Dd df_du(2, 0, ((double)(a - b)) / 255.0);
            Vector3Dd df_dv(0, 2, ((double)(d - c)) / 255.0);

            Vector3Dd normal = df_du.crossProduct(df_dv);
            normal = Vector3Dd(
                normal.x() * scale.x(),
                normal.y() * scale.y(),
                normal.z() * scale.z()).normalized();
            putNormal(u, v, normal);
        }
    }

    for (int u = 0; u < xxSize; u++) {
        Vector3Dd* n1 = getNormal(u, 1);
        if (n1) { putNormal(u, 0, *n1); delete n1; }
        Vector3Dd* n2 = getNormal(u, yySize - 3);
        if (n2) { putNormal(u, yySize - 2, *n2); delete n2; }
        Vector3Dd* n3 = getNormal(u, yySize - 2);
        if (n3) { putNormal(u, yySize - 1, *n3); delete n3; }
    }
    for (int v = 0; v < yySize; v++) {
        Vector3Dd* n1 = getNormal(2, v);
        if (n1) { putNormal(1, v, *n1); delete n1; }
        Vector3Dd* n2 = getNormal(1, v);
        if (n2) { putNormal(0, v, *n2); delete n2; }
        Vector3Dd* n3 = getNormal(xxSize - 2, v);
        if (n3) { putNormal(xxSize - 1, v, *n3); delete n3; }
    }

    return scale;
}

NormalMap* NormalMap::clone() const
{
    return new NormalMap(*this);
}

RGBImageUncompressed* NormalMap::exportToRgbImageGradient() const
{
    RGBImageUncompressed* output = new RGBImageUncompressed();

    if ( !output->init(xSize, ySize) ) {
        delete output;
        return nullptr;
    }

    int x;
    int y;
    int val;
    char col;
    Vector3Dd k(0, 0, 1);

    for ( y = 0; y < ySize; y++ ) {
        for ( x = 0; x < xSize; x++ ) {
            Vector3Dd* normal = getNormal(x, y);
            Vector3Dd n = normal != nullptr ? normal->normalized() : k;
            delete normal;

            val = (int)((1.0-k.dotProduct(n)) * 255.0);
            col = (char)(val & 0xFF);
            output->putPixel(x, y, col, col, col);
        }
    }
    return output;
}

RGBAImageUncompressed* NormalMap::exportToRgbaImageGradient() const
{
    RGBAImageUncompressed* output = new RGBAImageUncompressed();

    if ( !output->init(xSize, ySize) ) {
        delete output;
        return nullptr;
    }

    int x;
    int y;
    int val;
    char col;
    Vector3Dd k(0, 0, 1);

    for ( y = 0; y < ySize; y++ ) {
        for ( x = 0; x < xSize; x++ ) {
            Vector3Dd* normal = getNormal(x, y);
            Vector3Dd n = normal != nullptr ? normal->normalized() : k;
            delete normal;

            val = (int)((1.0-k.dotProduct(n)) * 255.0);
            col = (char)(val & 0xFF);
            output->putPixel(x, y, col, col, col, (char)128);
        }
    }
    return output;
}
