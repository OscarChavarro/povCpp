#include "solidTexture/ColorTextureFixture.h"
#include "solidTexture/MapTextureFixture.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/TextureUtils.h"
#include "solidTexture/FixturesFacade.h"

FixturesFacade::FixturesFacade(
    ProceduralNoise *proceduralNoise, TextureUtils *textureUtils)
    : proceduralNoise(proceduralNoise), textureUtils(textureUtils)
{
}

void
FixturesFacade::checkerTexture(
    double x, double y, double z, ColorRgba *color,
    int textureNumber1, Matrix4x4d *textureTransformationInverse1,
    TextureImage *image1, ColorRgba *color1_1, ColorRgba *color2_1,
    double turbulence1, int octaves1, RGBAColorPalette *colorMap1,
    Vector3Dd textureGradient1, double mortar1,
    int textureNumber2, Matrix4x4d *textureTransformationInverse2,
    TextureImage *image2, ColorRgba *color1_2, ColorRgba *color2_2,
    double turbulence2, int octaves2, RGBAColorPalette *colorMap2,
    Vector3Dd textureGradient2, double mortar2,
    double smallTolerance
    )
{
    int index;
    Vector3Dd point;
    FixturesFacade fixturesFacade(proceduralNoise, textureUtils);

    x += smallTolerance;
    y += smallTolerance;
    z += smallTolerance;

    index = (int)(textureUtils->floorInline(x) + textureUtils->floorInline(y) + textureUtils->floorInline(z));

    *&point = Vector3Dd(x, y, z);

    if (index & 1) {
        fixturesFacade.colorAt(
            color, textureNumber1, textureTransformationInverse1,
            image1, color1_1, color2_1,
            turbulence1, octaves1, colorMap1,
            textureGradient1, mortar1,
            &point, smallTolerance);
    } else {
        fixturesFacade.colorAt(
            color, textureNumber2, textureTransformationInverse2,
            image2, color1_2, color2_2,
            turbulence2, octaves2, colorMap2,
            textureGradient2, mortar2,
            &point, smallTolerance);
    }
}

void
FixturesFacade::colorAt(
    ColorRgba *color, int textureNumber,
    Matrix4x4d *textureTransformationInverse, TextureImage *image,
    ColorRgba *color1, ColorRgba *color2, double turbulence, int octaves,
    RGBAColorPalette *colorMap, Vector3Dd textureGradient, double mortar,
    Vector3Dd *intersectionPoint, double smallTolerance,
    int textureNumber1, Matrix4x4d *textureTransformationInverse1,
    TextureImage *image1, ColorRgba *color1_1, ColorRgba *color2_1,
    double turbulence1, int octaves1, RGBAColorPalette *colorMap1,
    Vector3Dd textureGradient1, double mortar1,
    int textureNumber2, Matrix4x4d *textureTransformationInverse2,
    TextureImage *image2, ColorRgba *color1_2, ColorRgba *color2_2,
    double turbulence2, int octaves2, RGBAColorPalette *colorMap2,
    Vector3Dd textureGradient2, double mortar2)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    MapTextureFixture mapFixture;
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
        checkerTexture(
            x, y, z, color,
            textureNumber1, textureTransformationInverse1,
            image1, color1_1, color2_1,
            turbulence1, octaves1, colorMap1,
            textureGradient1, mortar1,
            textureNumber2, textureTransformationInverse2,
            image2, color1_2, color2_2,
            turbulence2, octaves2, colorMap2,
            textureGradient2, mortar2,
            smallTolerance);
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
    }
}
