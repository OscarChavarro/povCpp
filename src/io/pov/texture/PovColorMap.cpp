#include "io/pov/texture/PovColorMap.h"
#include "java/util/ArrayList.txx"

PovColorMap::PovColorMap() : _transparencyFlag(false) {
}

PovColorMap::~PovColorMap() {
    for (long int i = 0; i < _spans.size(); i++) {
        if (_spans[i] != nullptr) {
            delete _spans[i];
            _spans[i] = nullptr;
        }
    }
    _spans.clear();
}

void PovColorMap::addSpan(
    double start, double end,
    const ColorRgba& sc, const ColorRgba& ec)
{
    Span* s = new Span();
    s->start = start;
    s->end = end;
    s->startColor = sc;
    s->endColor = ec;
    if (sc.getA() != 0.0 || ec.getA() != 0.0) {
        _transparencyFlag = true;
    }
    _spans.add(s);
}

int PovColorMap::size() const {
    return (int)_spans.size();
}

bool PovColorMap::transparencyFlag() const {
    return _transparencyFlag;
}

const PovColorMap::Span* PovColorMap::getSpanAt(int i) const {
    if (i < 0 || i >= (int)_spans.size()) {
        return nullptr;
    }
    return _spans.get(i);
}

ColorRgba PovColorMap::evalLinear(double t) const {
    ColorRgba result;
    result.setR(0.0); result.setG(0.0); result.setB(0.0); result.setA(0.0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    for (int i = 0; i < (int)_spans.size(); i++) {
        const Span* s = _spans.get(i);
        if (s != nullptr && t >= s->start && t <= s->end) {
            double frac = (t - s->start) / (s->end - s->start);
            result.setR(s->startColor.getR() + frac * (s->endColor.getR() - s->startColor.getR()));
            result.setG(s->startColor.getG() + frac * (s->endColor.getG() - s->startColor.getG()));
            result.setB(s->startColor.getB() + frac * (s->endColor.getB() - s->startColor.getB()));
            result.setA(s->startColor.getA() + frac * (s->endColor.getA() - s->startColor.getA()));
            return result;
        }
    }
    return result;
}
