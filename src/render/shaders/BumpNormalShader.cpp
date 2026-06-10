#include "solidTexture/BumpTextureFixture.h"
#include "solidTexture/MapTextureFixture.h"
#include "solidTexture/SolidTextureBumpyTextures.h"
#include "environment/geometry/GeometryConstants.h"
#include "render/shaders/BumpNormalShader.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, Material *texture,
    Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;

    if (texture->bumpNumber == SolidTextureBumpyTextures::NO_BUMPS) {
        *newNormal = *surfaceNormal;
        return;
    }

    if (texture->textureTransformation) {
        transformedPoint = texture->textureTransformationInverse->transpose().multiply(
            *intersectionPoint);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x();
    y = transformedPoint.y();
    z = transformedPoint.z();

    BumpTextureFixture bumpFixture(&TextureUtils::instance().getProceduralNoise());
    MapTextureFixture mapFixture;

    switch (texture->bumpNumber) {

    case SolidTextureBumpyTextures::WAVES:
        bumpFixture.waves(
            x, y, z, texture->bumpAmount, texture->frequency, texture->phase,
            TextureUtils::NUMBER_OF_WAVES, newNormal);
        break;

    case SolidTextureBumpyTextures::RIPPLES:
        bumpFixture.ripples(
            x, y, z, texture->bumpAmount, texture->frequency, texture->phase,
            TextureUtils::NUMBER_OF_WAVES, newNormal);
        break;

    case SolidTextureBumpyTextures::WRINKLES:
        bumpFixture.wrinkles(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyTextures::BUMPS:
        bumpFixture.bumps(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyTextures::DENTS:
        bumpFixture.dents(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyTextures::BUMP_MAP:
        mapFixture.bumpMap(
            x, y, z, texture->bumpImage, texture->bumpAmount, newNormal,
            GeometryConstants::Small_Tolerance);
        break;

    default: break;
    }
}
