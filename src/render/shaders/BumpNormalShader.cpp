#include "render/shaders/BumpNormalShader.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/material/RendererConfiguration.h"
#include "media/solidTexture/BumpTextureFixture.h"
#include "media/solidTexture/MapTextureFixture.h"
#include "media/solidTexture/TextureFixture.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, Texture *texture,
    Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;

    if (texture->bumpNumber == Texture::NO_BUMPS) {
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

    switch (texture->bumpNumber) {

    case Texture::WAVES:
        BumpTextureFixture::waves(x, y, z, texture, newNormal);
        break;

    case Texture::RIPPLES:
        BumpTextureFixture::ripples(x, y, z, texture, newNormal);
        break;

    case Texture::WRINKLES:
        BumpTextureFixture::wrinkles(
            x, y, z, texture, newNormal);
        break;

    case Texture::BUMPS:
        BumpTextureFixture::bumps(x, y, z, texture, newNormal);
        break;

    case Texture::DENTS:
        BumpTextureFixture::dents(x, y, z, texture, newNormal);
        break;

    case Texture::BUMPY1:
        TextureFixture::bumpy1(x, y, z, texture, newNormal);
        break;

    case Texture::BUMPY2:
        TextureFixture::bumpy2(x, y, z, texture, newNormal);
        break;

    case Texture::BUMPY3:
        TextureFixture::bumpy3(x, y, z, texture, newNormal);
        break;

    case Texture::BUMPMAP:
        MapTextureFixture::bumpMap(
            x, y, z, texture, newNormal, GeometryConstants::Small_Tolerance);
        break;
    }
}

