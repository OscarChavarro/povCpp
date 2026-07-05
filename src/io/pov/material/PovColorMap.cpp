#include "io/pov/material/PovColorMap.h"
#include "java/util/ArrayList.txx"

PovColorMap::PovColorMap() {
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
    _spans.add(s);
}

int PovColorMap::size() const {
    return (int)_spans.size();
}

const PovColorMapSpan *PovColorMap::getSpanAt(int i) const {
    if (i < 0 || i >= (int)_spans.size()) {
        return nullptr;
    }
    return _spans.get(i);
}
