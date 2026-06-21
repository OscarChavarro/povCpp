#include "io/pov/material/PovColorMapSpan.h"

PovColorMapSpan::PovColorMapSpan()
    : start(0.0), end(0.0), startColor(0.0, 0.0, 0.0, 0.0),
      endColor(0.0, 0.0, 0.0, 0.0)
{
}

double
PovColorMapSpan::getStart() const
{
    return start;
}

void
PovColorMapSpan::setStart(double value)
{
    start = value;
}

double
PovColorMapSpan::getEnd() const
{
    return end;
}

void
PovColorMapSpan::setEnd(double value)
{
    end = value;
}

const ColorRgba &
PovColorMapSpan::getStartColor() const
{
    return startColor;
}

void
PovColorMapSpan::setStartColor(const ColorRgba &value)
{
    startColor = value;
}

const ColorRgba &
PovColorMapSpan::getEndColor() const
{
    return endColor;
}

void
PovColorMapSpan::setEndColor(const ColorRgba &value)
{
    endColor = value;
}
