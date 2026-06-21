#include <cstdlib>
#include <mutex>

#include "java/lang/Math.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/LambertShader.h"

void
LambertShader::shade(const PovRayMaterial *texture, const RayWithSegments *lightSourceRay,
    const Vector3Dd *surfaceNormal, ColorRgba *color, const ColorRgba *lightColor,
    const ColorRgba *surfaceColor, double attenuation)
{
    double cosAngleOfIncidence;
    double intensity;
    double randomNumber;

    cosAngleOfIncidence =
        (*surfaceNormal).dotProduct(lightSourceRay->getDirection());
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->getObjectBrilliance() != 1.0) {
        intensity = java::Math::pow(cosAngleOfIncidence, texture->getObjectBrilliance());
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->getObjectDiffuse() * attenuation;

    // texture_randomness dither. rand() is global, non-reentrant state, so
    // concurrent RenderTask threads calling it under -parallel is a real
    // data race (confirmed: it made parallel output non-deterministic
    // run-to-run). Switching to a thread-local generator would remove the
    // race but also change the exact value sequence vs. the single serial
    // rand() stream, which broke byte-for-byte parity with several golden
    // images (kscope/ntreal/piece1/pool/roman/snack/snail/tomb/wg5 - all use
    // texture_randomness). So instead this keeps the SAME rand() call/stream
    // (serial output unchanged, golden-safe) and only adds a mutex so
    // concurrent calls under -parallel are serialized rather than racing.
    // -parallel still won't reproduce serial's exact dither pattern (the
    // interleaving order across threads differs from the single-threaded
    // order) - an accepted AE-similar-not-bit-identical limitation, the same
    // class as antialiasing (AdaptiveAntiAliasing, M1/B4). No gate scene
    // sets texture_randomness, so the gate is unaffected either way.
    static std::mutex randMutex;
    {
        std::lock_guard<std::mutex> lock(randMutex);
        randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;
    }

    intensity -= randomNumber * texture->getTextureRandomness();

    color->setR(color->getR() + intensity * (surfaceColor->getR()) * (lightColor->getR()));
    color->setG(color->getG() + intensity * (surfaceColor->getG()) * (lightColor->getG()));
    color->setB(color->getB() + intensity * (surfaceColor->getB()) * (lightColor->getB()));
}
