#include "render/shaders/BumpNormalShader.h"

void
BumpNormalShader::shade(Vector3Dd *newNormal, const PovRayMaterial *texture,
    const Vector3Dd *intersectionPoint, const Vector3Dd *surfaceNormal,
    TextureUtils *textureUtils)
{
    if (texture->getNormal() == nullptr) {
        *newNormal = *surfaceNormal;
        return;
    }

    // Unlike SolidTexturePigment::transformToObjectSpace, this intentionally has no
    // COORDINATE_LIMIT guard: the original bump shading path never had one either.
    Vector3Dd transformedPoint;
    if (texture->getTextureTransformation()) {
        transformedPoint = texture->getTextureTransformationInverse()->transpose().multiply(
            *intersectionPoint);
    } else {
        transformedPoint = *intersectionPoint;
    }

    BumpTextureFixture bumpFixture(&textureUtils->getProceduralNoise(), textureUtils);
    ImageTexture mapFixture;

    texture->getNormal()->applyTo(&transformedPoint, newNormal, bumpFixture, mapFixture);
}
