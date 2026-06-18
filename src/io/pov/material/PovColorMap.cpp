#include "java/util/ArrayList.txx"

#include "io/pov/material/PovColorMap.h"

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
    PovColorMapSpan *s = new PovColorMapSpan();
    s->setStart(start);
    s->setEnd(end);
    s->setStartColor(sc);
    s->setEndColor(ec);
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

const PovColorMapSpan *PovColorMap::getSpanAt(int i) const {
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
        const PovColorMapSpan *s = _spans.get(i);
        if (s != nullptr && t >= s->getStart() && t <= s->getEnd()) {
            const double frac = (t - s->getStart()) / (s->getEnd() - s->getStart());
            result.setR(s->getStartColor().getR() + frac * (s->getEndColor().getR() - s->getStartColor().getR()));
            result.setG(s->getStartColor().getG() + frac * (s->getEndColor().getG() - s->getStartColor().getG()));
            result.setB(s->getStartColor().getB() + frac * (s->getEndColor().getB() - s->getStartColor().getB()));
            result.setA(s->getStartColor().getA() + frac * (s->getEndColor().getA() - s->getStartColor().getA()));
            return result;
        }
    }
    return result;
}
