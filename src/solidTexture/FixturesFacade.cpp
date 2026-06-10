#include "solidTexture/FixturesFacade.h"

#include "java/util/ArrayList.txx"
#include "solidTexture/ColorTextureFixture.h"
#include "solidTexture/MapTextureFixture.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/TextureFixture.h"

FixturesFacade::FixturesFacade(
    ProceduralNoise *proceduralNoise, TextureUtils *textureUtils)
    : proceduralNoise(proceduralNoise), textureUtils(textureUtils)
{
}

void
FixturesFacade::checkerTexture(
    double x, double y, double z, ColorRgba *color, ColorRgba *color1,
    ColorRgba *color2, double smallTolerance)
{
    int brkindx;
    Vector3Dd point;
    FixturesFacade fixturesFacade(proceduralNoise, textureUtils);
    Texture *texture1 = (Texture *)color1;
    Texture *texture2 = (Texture *)color2;

    x += smallTolerance; // add a small offset to x, y, z, axes to prevent noise
    y += smallTolerance;
    z += smallTolerance;

    brkindx = (int)(textureUtils->floorInline(x) + textureUtils->floorInline(y) + textureUtils->floorInline(z));

    *&point = Vector3Dd(x, y, z);

    if (brkindx & 1) {
        fixturesFacade.colorAt(
            color, texture1->textureNumber, texture1->textureTransformationInverse,
            texture1->image, texture1->color1, texture1->color2,
            texture1->turbulence, texture1->octaves, texture1->colorMap,
            texture1->textureGradient, texture1->mortar, &point, smallTolerance);
    } else {
        fixturesFacade.colorAt(
            color, texture2->textureNumber, texture2->textureTransformationInverse,
            texture2->image, texture2->color1, texture2->color2,
            texture2->turbulence, texture2->octaves, texture2->colorMap,
            texture2->textureGradient, texture2->mortar, &point, smallTolerance);
    }
}

void
FixturesFacade::colorAt(
    ColorRgba *color, int textureNumber,
    Matrix4x4d *textureTransformationInverse, TextureImage *image,
    ColorRgba *color1, ColorRgba *color2, double turbulence, int octaves,
    RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
    Vector3Dd *intersectionPoint, double smallTolerance)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    MapTextureFixture mapFixture;
    ColorTextureFixture colorFixture(proceduralNoise, textureUtils);
    TextureFixture textureFixture(proceduralNoise);

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
    case (int)SolidTextureColorTextures::NO_TEXTURE:
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0.0);
        break;

    case (int)SolidTextureColorTextures::COLOUR_TEXTURE:
        color->setR(color->getR() + color1->getR());
        color->setG(color->getG() + color1->getG());
        color->setB(color->getB() + color1->getB());
        color->setA(color->getA() + color1->getA());
        break;

    case (int)SolidTextureColorTextures::BOZO_TEXTURE:
        colorFixture.bozo(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::MARBLE_TEXTURE:
        colorFixture.marble(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::WOOD_TEXTURE:
        colorFixture.wood(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::BRICK_TEXTURE:
        colorFixture.brick(x, y, z, color, color1, color2, mortar);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE:
        colorFixture.checker(x, y, z, color, color1, color2, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE:
        checkerTexture(x, y, z, color, color1, color2, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::SPOTTED_TEXTURE:
        colorFixture.spotted(x, y, z, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::AGATE_TEXTURE:
        colorFixture.agate(x, y, z, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::GRANITE_TEXTURE:
        colorFixture.granite(x, y, z, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::GRADIENT_TEXTURE:
        colorFixture.gradient(
            x, y, z, turbulence, colorMap, textureGradient, octaves, color);
        break;

    case (int)SolidTextureColorTextures::IMAGEMAP_TEXTURE:
        mapFixture.imageMap(x, y, z, image, color, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::ONION_TEXTURE:
        colorFixture.onion(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::LEOPARD_TEXTURE:
        colorFixture.leopard(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED1_TEXTURE:
        textureFixture.painted1(x, y, z, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED2_TEXTURE:
        textureFixture.painted2(x, y, z, turbulence, octaves, colorMap, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED3_TEXTURE:
        textureFixture.painted3(x, y, z, color);
        break;
    }
}
