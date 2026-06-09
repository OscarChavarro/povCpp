#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "java/util/ArrayList.txx"

RGBAColorPalette::RGBAColorPalette() {
    init(0);
}

RGBAColorPalette::~RGBAColorPalette() {
    for (long int i = 0; i < colors.size(); i++) {
        if (colors[i] != nullptr) {
            delete colors[i];
            colors[i] = nullptr;
        }
    }
    colors.clear();
    positions.clear();
}

void RGBAColorPalette::init(int size) {
    for (long int i = 0; i < colors.size(); i++) {
        if (colors[i] != nullptr) {
            delete colors[i];
            colors[i] = nullptr;
        }
    }
    colors.clear();
    positions.clear();

    for (int i = 0; i < size; i++) {
        colors.add(new ColorRgba());
    }
}

int RGBAColorPalette::size() const {
    return (int)colors.size();
}

bool RGBAColorPalette::hasPositions() const {
    return positions.size() > 0;
}

ColorRgba* RGBAColorPalette::getColorAt(int i) const {
    if (i < 0 || i >= (int)colors.size()) {
        return nullptr;
    }
    if (colors.get(i) != nullptr) {
        return new ColorRgba(*colors.get(i));
    }
    return nullptr;
}

double RGBAColorPalette::getPositionAt(int i) const {
    if (i < 0 || i >= (int)positions.size()) {
        return 0.0;
    }
    return positions.get(i);
}

void RGBAColorPalette::setColorAt(int i, const ColorRgba& c) {
    if (i < 0 || i >= (int)colors.size()) {
        return;
    }
    if (colors[i] != nullptr) {
        delete colors[i];
    }
    colors[i] = new ColorRgba(c);
}

void RGBAColorPalette::setColorAt(int i, double r, double g, double b, double a) {
    if (i < 0 || i >= (int)colors.size()) {
        return;
    }
    if (colors[i] != nullptr) {
        delete colors[i];
    }
    ColorRgba* nc = new ColorRgba();
    nc->setR(r); nc->setG(g); nc->setB(b); nc->setA(a);
    colors[i] = nc;
}

void RGBAColorPalette::addColor(const ColorRgba& c) {
    colors.add(new ColorRgba(c));
}

void RGBAColorPalette::addColor(double r, double g, double b, double a) {
    ColorRgba* nc = new ColorRgba();
    nc->setR(r); nc->setG(g); nc->setB(b); nc->setA(a);
    colors.add(nc);
}

void RGBAColorPalette::addColorAt(double position, const ColorRgba& c) {
    colors.add(new ColorRgba(c));
    positions.add(position);
}

ColorRgba* RGBAColorPalette::evalNearest(double t) const {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    int N = (int)colors.size();
    int i = (int)(t * ((double)N));

    if (i < 0) i = 0;
    if (i >= N) i = N - 1;

    if (colors.get(i) != nullptr) {
        return new ColorRgba(*colors.get(i));
    }
    ColorRgba* zero = new ColorRgba();
    zero->setR(0.0); zero->setG(0.0); zero->setB(0.0); zero->setA(0.0);
    return zero;
}

ColorRgba* RGBAColorPalette::evalLinear(double t) const {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    // Position-aware path: exact span-boundary interpolation.
    // Zero-width pairs (a == b) mark discontinuities and are skipped to
    // avoid division by zero; the preceding non-zero pair already handles t==a.
    if (positions.size() > 0 && positions.size() == colors.size()) {
        int N = (int)colors.size() - 1;
        for (int i = 0; i < N; i++) {
            double a = positions.get(i);
            double b = positions.get(i + 1);
            if (a == b) continue;
            if (t >= a && t <= b) {
                double frac = (t - a) / (b - a);
                ColorRgba* ca = colors.get(i);
                ColorRgba* cb = colors.get(i + 1);
                ColorRgba* result = new ColorRgba();
                result->setR(ca->getR() + frac * (cb->getR() - ca->getR()));
                result->setG(ca->getG() + frac * (cb->getG() - ca->getG()));
                result->setB(ca->getB() + frac * (cb->getB() - ca->getB()));
                result->setA(ca->getA() + frac * (cb->getA() - ca->getA()));
                return result;
            }
        }
        ColorRgba* zero = new ColorRgba();
        zero->setR(0.0); zero->setG(0.0); zero->setB(0.0); zero->setA(0.0);
        return zero;
    }

    // Uniform-spacing path (analogous to RGBColorPalette::evalLinear)
    int N = (int)colors.size() - 1;
    int inf = (int)(t * ((double)N));
    int sup = inf + 1;

    double delta = 1.0 / ((double)N);
    double r = t - inf * delta;
    double p = r / delta;

    if (inf < 0) inf = 0;
    if (inf > N) inf = N;
    if (sup < 0) sup = 0;
    if (sup > N) sup = N;

    ColorRgba* a = colors.get(inf);
    ColorRgba* b = colors.get(sup);

    ColorRgba* result = new ColorRgba();
    result->setR(a->getR() + (b->getR() - a->getR()) * p);
    result->setG(a->getG() + (b->getG() - a->getG()) * p);
    result->setB(a->getB() + (b->getB() - a->getB()) * p);
    result->setA(a->getA() + (b->getA() - a->getA()) * p);
    return result;
}
