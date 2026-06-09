#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "java/util/ArrayList.txx"

RGBAColorPalette::RGBAColorPalette() : _transparencyFlag(false) {
}

RGBAColorPalette::~RGBAColorPalette() {
    for (long int i = 0; i < spans.size(); i++) {
        if (spans[i] != nullptr) {
            delete spans[i];
            spans[i] = nullptr;
        }
    }
    spans.clear();
}

void RGBAColorPalette::addSpan(
    double start, double end,
    const ColorRgba& startColor, const ColorRgba& endColor)
{
    RGBAColorPaletteSpan* s = new RGBAColorPaletteSpan();
    s->start = start;
    s->end = end;
    s->startColor = startColor;
    s->endColor = endColor;
    if (startColor.getA() != 0.0 || endColor.getA() != 0.0) {
        _transparencyFlag = true;
    }
    spans.add(s);
}

int RGBAColorPalette::size() const {
    return (int)spans.size();
}

bool RGBAColorPalette::transparencyFlag() const {
    return _transparencyFlag;
}

const RGBAColorPaletteSpan* RGBAColorPalette::getSpanAt(int i) const {
    if (i < 0 || i >= (int)spans.size()) {
        return nullptr;
    }
    return spans.get(i);
}

ColorRgba RGBAColorPalette::evalLinear(double t) const {
    ColorRgba result;
    result.setR(0.0);
    result.setG(0.0);
    result.setB(0.0);
    result.setA(0.0);

    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    for (int i = 0; i < (int)spans.size(); i++) {
        const RGBAColorPaletteSpan* ent = spans.get(i);
        if (ent != nullptr && t >= ent->start && t <= ent->end) {
            double fraction = (t - ent->start) / (ent->end - ent->start);
            result.setR(ent->startColor.getR() +
                fraction * (ent->endColor.getR() - ent->startColor.getR()));
            result.setG(ent->startColor.getG() +
                fraction * (ent->endColor.getG() - ent->startColor.getG()));
            result.setB(ent->startColor.getB() +
                fraction * (ent->endColor.getB() - ent->startColor.getB()));
            result.setA(ent->startColor.getA() +
                fraction * (ent->endColor.getA() - ent->startColor.getA()));
            return result;
        }
    }

    return result;
}
