#include "render/shaders/BumpNormalShader.h"
#include "common/linealAlgebra/Transformation.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/material/RendererConfiguration.h"
#include "media/BumpTextureFixture.h"
#include "media/MapTextureFixture.h"
#include "media/TextureFixture.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, Texture *texture,
    Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;
    const int debugEnabled = (RenderingConfiguration::global().options & DEBUGGING);

    if (texture->bumpNumber == NO_BUMPS) {
        *newNormal = *surfaceNormal;
        return;
    }

    if (texture->Texture_Transformation) {
        Transformation::MInverseTransformVector(&transformedPoint,
            intersectionPoint, texture->Texture_Transformation);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    switch (texture->bumpNumber) {

    case WAVES:
        BumpTextureFixture::waves(x, y, z, texture, newNormal, debugEnabled);
        break;

    case RIPPLES:
        BumpTextureFixture::ripples(x, y, z, texture, newNormal, debugEnabled);
        break;

    case WRINKLES:
        BumpTextureFixture::wrinkles(
            x, y, z, texture, newNormal, debugEnabled);
        break;

    case BUMPS:
        BumpTextureFixture::bumps(x, y, z, texture, newNormal, debugEnabled);
        break;

    case DENTS:
        BumpTextureFixture::dents(x, y, z, texture, newNormal, debugEnabled);
        break;

    case BUMPY1:
        TextureFixture::bumpy1(x, y, z, texture, newNormal, debugEnabled);
        break;

    case BUMPY2:
        TextureFixture::bumpy2(x, y, z, texture, newNormal, debugEnabled);
        break;

    case BUMPY3:
        TextureFixture::bumpy3(x, y, z, texture, newNormal, debugEnabled);
        break;

    case BUMPMAP:
        MapTextureFixture::bumpMap(
            x, y, z, texture, newNormal, debugEnabled, Small_Tolerance);
        break;
    }
}

