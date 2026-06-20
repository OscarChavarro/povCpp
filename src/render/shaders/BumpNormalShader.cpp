#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/BumpTextureFixture.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/geometry/GeometryConstants.h"
#include "render/shaders/BumpNormalShader.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, const PovRayMaterial *texture,
    const Vector3Dd *intersectionPoint, const Vector3Dd *surfaceNormal,
    TextureUtils *textureUtils)
{
    Vector3Dd transformedPoint;

    if (texture->getBumpPatternType() == SolidTextureBumpyNames::NO_BUMPS) {
        *newNormal = *surfaceNormal;
        return;
    }

    if (texture->getTextureTransformation()) {
        transformedPoint = texture->getTextureTransformationInverse()->transpose().multiply(
            *intersectionPoint);
    } else {
        transformedPoint = *intersectionPoint;
    }

    double x = transformedPoint.x();
    double y = transformedPoint.y();
    double z = transformedPoint.z();

    BumpTextureFixture bumpFixture(&textureUtils->getProceduralNoise(), textureUtils);
    ImageTexture mapFixture;

    switch (texture->getBumpPatternType()) {

    case SolidTextureBumpyNames::WAVES:
        bumpFixture.waves(
            x, y, z, texture->getBumpAmount(),
            texture->getBumpFrequency(), texture->getBumpPhase(),
            texture->getBumpNumberOfWaves(), newNormal);
        break;

    case SolidTextureBumpyNames::RIPPLES:
        bumpFixture.ripples(
            x, y, z, texture->getBumpAmount(),
            texture->getBumpFrequency(), texture->getBumpPhase(),
            texture->getBumpNumberOfWaves(), newNormal);
        break;

    case SolidTextureBumpyNames::WRINKLES:
        bumpFixture.wrinkles(x, y, z, texture->getBumpAmount(), newNormal);
        break;

    case SolidTextureBumpyNames::BUMPS:
        bumpFixture.bumps(x, y, z, texture->getBumpAmount(), newNormal);
        break;

    case SolidTextureBumpyNames::DENTS:
        bumpFixture.dents(x, y, z, texture->getBumpAmount(), newNormal);
        break;

    case SolidTextureBumpyNames::BUMP_MAP:
        mapFixture.bumpMap(
            x, y, z, texture->getBumpImage(), texture->getBumpAmount(), newNormal,
            GeometryConstants::Small_Tolerance);
        break;

    default: break;
    }
}
