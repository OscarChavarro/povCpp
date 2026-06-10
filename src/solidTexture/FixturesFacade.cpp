#include "solidTexture/FixturesFacade.h"

#include "java/util/ArrayList.txx"
#include "solidTexture/ColorTextureFixture.h"
#include "solidTexture/MapTextureFixture.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/TextureUtils.h"

class TextureFixture {
  public:
    TextureFixture(ProceduralNoise *proceduralNoise);
    void painted1(
        double x, double y, double z, RGBAColorPalette *colorMap,
        ColorRgba *color);
    void painted2(
        double x, double y, double z, double turbulence, int octaves,
        RGBAColorPalette *colorMap, ColorRgba *color);
    void painted3(double x, double y, double z, ColorRgba *color);

  private:
    ProceduralNoise *proceduralNoise;
};

TextureFixture::TextureFixture(ProceduralNoise *proceduralNoise)
    : proceduralNoise(proceduralNoise)
{
}

/**
Painted1: takes an x,y,z point on an object and returns the color at that point.
See ColorTextureFixture for similar finished textures (granite, agate, marble, etc.).
*/
void
TextureFixture::painted1(
    double x, double y, double z, RGBAColorPalette *colorMap, ColorRgba *color)
{

    // Swirled()
    Vector3Dd colorVector;
    Vector3Dd result;
    int i;
    double scale = 1.0;
    double temp;
    ColorRgba newColor;


    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        proceduralNoise->dNoise(&colorVector, x, y, z);
        temp = proceduralNoise->noise(colorVector.x() * 4 * scale,
            colorVector.y() * 4 * scale, colorVector.z() * 4 * scale);
        temp = TextureUtils::instance().fabsInline(temp);
        rx += temp / scale;
        ry += temp / scale;
        rz += temp / scale;
    }
    result = Vector3Dd(rx, ry, rz);

    temp = result.x();
    if (colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, colorMap, temp);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + temp);
    color->setG(color->getG() + temp);
    color->setB(color->getB() + temp);
}

void
TextureFixture::painted2(
    double x, double y, double z, double turbulence, int octaves,
    RGBAColorPalette *colorMap, ColorRgba *color)
{
    int brkindx;
    double turb;
    Vector3Dd textureTurbulence;
    ColorRgba colour1;
    ColorRgba color2;

    // You could change the parser to take two colors after PAINTED2, but since
    // the colormap is already parsed it's easier to use it during testing.
    // If the texture works out right you can change the parser later.
    if (colorMap != nullptr) {
        TextureUtils::instance().computeColor(&colour1, colorMap, 0.1);
        TextureUtils::instance().computeColor(&color2, colorMap, 0.9);
    } else {
        colour1.setR(1.0); colour1.setG(1.0); colour1.setB(1.0); colour1.setA(0.0);
        color2.setR(0.0); color2.setG(1.0); color2.setB(0.0); color2.setA(0.0);
    }

    if ((turb = turbulence) != 0.0) {
        proceduralNoise->dTurbulence(
            &textureTurbulence, x, y, z, octaves);
        x += textureTurbulence.x() * turb;
        y += textureTurbulence.y() * turb;
        z += textureTurbulence.z() * turb;
    }

    brkindx = (int)TextureUtils::instance().floorInline(x) + (int)TextureUtils::instance().floorInline(z);


    if (brkindx & 1) {
        color->setR(colour1.getR());
        color->setG(colour1.getG());
        color->setB(colour1.getB());
        color->setA(colour1.getA());
    } else {
        color->setR(color2.getR());
        color->setG(color2.getG());
        color->setB(color2.getB());
        color->setA(color2.getA());
    }
    return;
}

void
TextureFixture::painted3(
    double x, double y, double z, ColorRgba *color)
{
    ;
}

FixturesFacade::FixturesFacade(
    ProceduralNoise *proceduralNoise, TextureUtils *textureUtils)
    : proceduralNoise(proceduralNoise), textureUtils(textureUtils)
{
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
