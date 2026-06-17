#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/BumpTextureFixture.h"

#include "environment/material/SolidTextureBumpyNames.h"

#include "environment/geometry/GeometryConstants.h"

#include "render/shaders/BumpNormalShader.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, const PovrayMaterial *texture,
    const Vector3Dd *intersectionPoint, const Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;

    if (texture->bumpNumber == SolidTextureBumpyNames::NO_BUMPS) {
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
    ImageTexture mapFixture;

    switch (texture->bumpNumber) {

    case SolidTextureBumpyNames::WAVES:
        bumpFixture.waves(
            x, y, z, texture->bumpAmount, texture->frequency, texture->phase,
            texture->numberOfWaves, newNormal);
        break;

    case SolidTextureBumpyNames::RIPPLES:
        bumpFixture.ripples(
            x, y, z, texture->bumpAmount, texture->frequency, texture->phase,
            texture->numberOfWaves, newNormal);
        break;

    case SolidTextureBumpyNames::WRINKLES:
        bumpFixture.wrinkles(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyNames::BUMPS:
        bumpFixture.bumps(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyNames::DENTS:
        bumpFixture.dents(x, y, z, texture->bumpAmount, newNormal);
        break;

    case SolidTextureBumpyNames::BUMP_MAP:
        mapFixture.bumpMap(
            x, y, z, texture->bumpImage, texture->bumpAmount, newNormal,
            GeometryConstants::Small_Tolerance);
        break;

    default: break;
    }
}
