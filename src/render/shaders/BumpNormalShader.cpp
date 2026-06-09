#include "render/shaders/BumpNormalShader.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/material/RendererConfiguration.h"
#include "media/solidTexture/BumpTextureFixture.h"
#include "media/solidTexture/MapTextureFixture.h"
#include "media/solidTexture/SolidTextureBumpyTextures.h"
#include "media/solidTexture/TextureFixture.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, Texture *texture,
    Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;

    if (texture->bumpNumber == (int)SolidTextureBumpyTextures::NO_BUMPS) {
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

    BumpTextureFixture bumpFixture;
    TextureFixture textureFixture;
    MapTextureFixture mapFixture;

    switch (texture->bumpNumber) {

    case (int)SolidTextureBumpyTextures::WAVES:
        bumpFixture.waves(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::RIPPLES:
        bumpFixture.ripples(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::WRINKLES:
        bumpFixture.wrinkles(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::BUMPS:
        bumpFixture.bumps(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::DENTS:
        bumpFixture.dents(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::BUMPY1:
        textureFixture.bumpy1(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::BUMPY2:
        textureFixture.bumpy2(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::BUMPY3:
        textureFixture.bumpy3(x, y, z, texture, newNormal);
        break;

    case (int)SolidTextureBumpyTextures::BUMPMAP:
        mapFixture.bumpMap(x, y, z, texture, newNormal, GeometryConstants::Small_Tolerance);
        break;
    }
}
