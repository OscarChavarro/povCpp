#include "environment/material/pigment/CheckerTexturePigment.h"
#include "environment/material/PovRayMaterial.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

CheckerTexturePigment::CheckerTexturePigment(PovRayMaterial *texture1, PovRayMaterial *texture2) :
    texture1(texture1),
    texture2(texture2)
{
}

void
CheckerTexturePigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    const double x = point->x() + smallTolerance;
    const double y = point->y() + smallTolerance;
    const double z = point->z() + smallTolerance;
    const int index = (int)(TextureUtils::floorInline(x) + TextureUtils::floorInline(y) +
        TextureUtils::floorInline(z));
    const Vector3Dd offsetPoint(x, y, z);
    PovRayMaterial * const chosen = (index & 1) ? texture1 : texture2;
    const Vector3Dd transformedPoint = SolidTexturePigment::transformToObjectSpace(
        &offsetPoint, chosen->getTextureTransformationInverse());

    if (chosen->getPigment() != nullptr) {
        chosen->getPigment()->colorAt(&transformedPoint, color, smallTolerance, colorFixture, mapFixture);
    } else {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0.0);
    }
}

SolidTexturePigment *
CheckerTexturePigment::copy() const
{
    return new CheckerTexturePigment(
        texture1 != nullptr ? PovRayMaterial::copyTexture(texture1) : nullptr,
        texture2 != nullptr ? PovRayMaterial::copyTexture(texture2) : nullptr);
}

void
CheckerTexturePigment::rotate(Vector3Dd *vector)
{
    if (texture1 != nullptr) {
        texture1->rotate(vector);
    }
    if (texture2 != nullptr) {
        texture2->rotate(vector);
    }
}

void
CheckerTexturePigment::scale(Vector3Dd *vector)
{
    if (texture1 != nullptr) {
        texture1->scale(vector);
    }
    if (texture2 != nullptr) {
        texture2->scale(vector);
    }
}

void
CheckerTexturePigment::translate(Vector3Dd *vector)
{
    if (texture1 != nullptr) {
        texture1->translate(vector);
    }
    if (texture2 != nullptr) {
        texture2->translate(vector);
    }
}

PovRayMaterial *
CheckerTexturePigment::getTexture1() const
{
    return texture1;
}

PovRayMaterial *
CheckerTexturePigment::getTexture2() const
{
    return texture2;
}
