#include "environment/material/pigment/SolidTexturePigment.h"

static constexpr double COORDINATE_LIMIT = 1.0e17;

RGBAColorPalette *
SolidTexturePigment::cloneColorMap(const RGBAColorPalette *source)
{
    if (source == nullptr) {
        return nullptr;
    }
    RGBAColorPalette * const newMap = new RGBAColorPalette();
    for (int i = 0; i < source->size(); i++) {
        const ColorRgba *c = source->getColorAt(i);
        if (source->hasPositions()) {
            newMap->addColorAt(source->getPositionAt(i), *c);
        } else {
            newMap->addColor(*c);
        }
        delete c;
    }
    return newMap;
}

Vector3Dd
SolidTexturePigment::transformToObjectSpace(
    const Vector3Dd *point, const Matrix4x4d *transformationInverse)
{
    if ((point->x() > COORDINATE_LIMIT) || (point->y() > COORDINATE_LIMIT) ||
        (point->z() > COORDINATE_LIMIT) || (point->x() < -COORDINATE_LIMIT) ||
        (point->y() < -COORDINATE_LIMIT) || (point->z() < -COORDINATE_LIMIT)) {
        return Vector3Dd(0.0, 0.0, 0.0);
    }
    if (transformationInverse) {
        return transformationInverse->transpose().multiply(*point);
    }
    return *point;
}
