#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"

#include "environment/material/SolidTextureColorNames.h"

#include "render/SolidTextureFixturesFacade.h"

SolidTextureFixturesFacade::SolidTextureFixturesFacade(
    const ProceduralNoise *proceduralNoise, const TextureUtils *textureUtils)
    : proceduralNoise(proceduralNoise), textureUtils(textureUtils)
{
}

void
SolidTextureFixturesFacade::checkerTexture(
    double x, double y, double z, ColorRgba *color,
    const SolidTextureFixturesCheckerParameterSet &parametersSet1,
    const SolidTextureFixturesCheckerParameterSet &parametersSet2,
    double smallTolerance
    ) const
{
    int index;
    Vector3Dd point;
    SolidTextureFixturesFacade fixturesFacade(proceduralNoise, textureUtils);

    x += smallTolerance;
    y += smallTolerance;
    z += smallTolerance;

    index = (int)(textureUtils->floorInline(x) + textureUtils->floorInline(y) + textureUtils->floorInline(z));

    *&point = Vector3Dd(x, y, z);

    if (index & 1) {
        fixturesFacade.colorAt(
            color, parametersSet1.getTextureNumber(), parametersSet1.getTextureTransformationInverse(),
            parametersSet1.getImage(), parametersSet1.getColor1(), parametersSet1.getColor2(),
            parametersSet1.getTurbulence(), parametersSet1.getOctaves(), parametersSet1.getColorMap(),
            parametersSet1.getTextureGradient(), parametersSet1.getMortar(),
            &point, smallTolerance);
    } else {
        fixturesFacade.colorAt(
            color, parametersSet2.getTextureNumber(), parametersSet2.getTextureTransformationInverse(),
            parametersSet2.getImage(), parametersSet2.getColor1(), parametersSet2.getColor2(),
            parametersSet2.getTurbulence(), parametersSet2.getOctaves(), parametersSet2.getColorMap(),
            parametersSet2.getTextureGradient(), parametersSet2.getMortar(),
            &point, smallTolerance);
    }
}

void
SolidTextureFixturesFacade::colorAt(
    ColorRgba *color,
    int textureNumber,
    const Matrix4x4d *textureTransformationInverse,
    const ControlledRGBAImageHDRUncompressed *image,
    const ColorRgba *color1,
    const ColorRgba *color2,
    double turbulence,
    int octaves,
    const RGBAColorPalette *colorMap,
    Vector3Dd textureGradient,
    double mortar,
    const Vector3Dd *intersectionPoint,
    double smallTolerance,
    const SolidTextureFixturesColorAtParameterSet &parametersSet1,
    const SolidTextureFixturesColorAtParameterSet &parametersSet2) const
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    ImageTexture mapFixture;
    ColorTextureFixture colorFixture(proceduralNoise, textureUtils);

    if ((intersectionPoint->x() > COORDINATE_LIMIT) ||
        (intersectionPoint->y() > COORDINATE_LIMIT) ||
        (intersectionPoint->z() > COORDINATE_LIMIT) ||
        (intersectionPoint->x() < -COORDINATE_LIMIT) ||
        (intersectionPoint->y() < -COORDINATE_LIMIT) ||
        (intersectionPoint->z() < -COORDINATE_LIMIT)) {
        *&transformedPoint = Vector3Dd(0.0, 0.0, 0.0);
    } else {
        if (textureTransformationInverse) {
            transformedPoint =
                textureTransformationInverse->transpose().multiply(
                    *intersectionPoint);
        } else {
            transformedPoint = *intersectionPoint;
        }
    }

    x = transformedPoint.x();
    y = transformedPoint.y();
    z = transformedPoint.z();

    switch (textureNumber) {
    case SolidTextureColorNames::NO_TEXTURE:
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0.0);
        break;

    case SolidTextureColorNames::COLOUR_TEXTURE:
        color->setR(color->getR() + color1->getR());
        color->setG(color->getG() + color1->getG());
        color->setB(color->getB() + color1->getB());
        color->setA(color->getA() + color1->getA());
        break;

    case SolidTextureColorNames::BOZO_TEXTURE:
        colorFixture.bozo(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case SolidTextureColorNames::MARBLE_TEXTURE:
        colorFixture.marble(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case SolidTextureColorNames::WOOD_TEXTURE:
        colorFixture.wood(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case SolidTextureColorNames::BRICK_TEXTURE:
        colorFixture.brick(x, y, z, color, color1, color2, mortar);
        break;

    case SolidTextureColorNames::CHECKER_TEXTURE:
        colorFixture.checker(x, y, z, color, color1, color2, smallTolerance);
        break;

    case SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE:
        checkerTexture(
            x, y, z, color,
            SolidTextureFixturesCheckerParameterSet(
                parametersSet1.getTextureNumber(), parametersSet1.getTextureTransformationInverse(),
                parametersSet1.getImage(), parametersSet1.getColor1(), parametersSet1.getColor2(),
                parametersSet1.getTurbulence(), parametersSet1.getOctaves(), parametersSet1.getColorMap(),
                parametersSet1.getTextureGradient(), parametersSet1.getMortar()),
            SolidTextureFixturesCheckerParameterSet(
                parametersSet2.getTextureNumber(), parametersSet2.getTextureTransformationInverse(),
                parametersSet2.getImage(), parametersSet2.getColor1(), parametersSet2.getColor2(),
                parametersSet2.getTurbulence(), parametersSet2.getOctaves(), parametersSet2.getColorMap(),
                parametersSet2.getTextureGradient(), parametersSet2.getMortar()),
            smallTolerance);
        break;

    case SolidTextureColorNames::SPOTTED_TEXTURE:
        colorFixture.spotted(x, y, z, colorMap, color);
        break;

    case SolidTextureColorNames::AGATE_TEXTURE:
        colorFixture.agate(x, y, z, octaves, colorMap, color);
        break;

    case SolidTextureColorNames::GRANITE_TEXTURE:
        colorFixture.granite(x, y, z, colorMap, color);
        break;

    case SolidTextureColorNames::GRADIENT_TEXTURE:
        colorFixture.gradient(
            x, y, z, turbulence, colorMap, textureGradient, octaves, color);
        break;

    case SolidTextureColorNames::IMAGE_MAP_TEXTURE:
        mapFixture.imageMap(x, y, z, image, color, smallTolerance);
        break;

    case SolidTextureColorNames::ONION_TEXTURE:
        colorFixture.onion(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case SolidTextureColorNames::LEOPARD_TEXTURE:
        colorFixture.leopard(x, y, z, turbulence, octaves, colorMap, color);
        break;
    }
}
