#include "java/util/PriorityQueue.txx"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/AmbientLightShader.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"
#include "environment/scene/SimpleBody.h"

void
LocalSurfaceShader::shade(const RayWithSegments *ray, PovRayMaterial *texture,
    IntersectionCandidate *rayIntersection, ColorRgba *surfaceColor,
    const ColorRgba *filterColor, ColorRgba *color,
    const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
    const BakedScene &bakedScene,
    int &traceLevel, TextureUtils *textureUtils)
{
    PovRayHit hit = PovRayHit::fromCandidate(*rayIntersection);
    Vector3Dd surfaceNormal;
    Vector3Dd texturePoint;
    double normalDirection;
    double attenuation;
    ColorRgba emittedColor(0.0, 0.0, 0.0, 0.0);

    // This variable keeps track of how much color comes from the surface
    // of the object and how much is transmited through

    emittedColor.setR(0.0); emittedColor.setG(0.0); emittedColor.setB(0.0); emittedColor.setA(0);

    // Only a *per-operand* CSG material (hit.material) is defined in the
    // operand's local frame and therefore wants the object-local point below;
    // a top-level object texture must be evaluated in world space. The incoming
    // `texture` argument has already been resolved to hit.material OR
    // hit.objectTexture by the caller, so `texture != nullptr` can no longer
    // tell the two apart - it is true even for a plain object texture. Mirror
    // RayShaderPipeline and key off hit.material directly, otherwise a CSG
    // object's own texture (e.g. fish13's swamp water: an intersection scaled
    // <10000 1 500> with a `ripples 0.7 frequency 0.08` normal) gets its bump
    // sampled in the unit-cube local frame, flattening the waves to nothing.
    const bool usingMaterialTexture = (hit.material != nullptr);
    if (texture == nullptr) {
        texture = static_cast<PovRayMaterial *>(hit.objectTexture);
    }

    if (!ray->getConfig()->withSurfaceLighting()) {
        surfaceColor->setA(0.0);
        color->setR(color->getR() + surfaceColor->getR() * filterColor->getA());
        color->setG(color->getG() + surfaceColor->getG() * filterColor->getA());
        color->setB(color->getB() + surfaceColor->getB() * filterColor->getA());
        return;
    }

    surfaceNormal = hit.n;
    texturePoint = hit.p;
    if (usingMaterialTexture &&
        hit.materialUsesObjectLocalPoint &&
        hit.hitBody != nullptr) {
        texturePoint = static_cast<SimpleBody *>(hit.hitBody)->worldPointToLocal(hit.p);
    }

    if (ray->getConfig()->withBumpMapping()) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &texturePoint, &surfaceNormal,
            textureUtils);
    }

    // If the surface normal points away, flip its direction
    normalDirection = surfaceNormal.dotProduct(ray->getDirection());
    if (normalDirection > 0.0) {
        surfaceNormal = surfaceNormal.multiply(-1.0);
    }

    attenuation = filterColor->getA() * (1.0 - surfaceColor->getA());

    AmbientLightShader::shade(texture, surfaceColor, &emittedColor, attenuation);
    DirectLightShader::shade(texture, &hit.p, ray, &surfaceNormal,
        surfaceColor, &emittedColor, attenuation, traceService,
        lightSources,
        bakedScene);
    color->setR(color->getR() + emittedColor.getR());
    color->setG(color->getG() + emittedColor.getG());
    color->setB(color->getB() + emittedColor.getB());
    if (ray->getConfig()->withReflection()) {
        MirrorReflectionShader::shade(
            texture, &hit.p, ray, &surfaceNormal, color,
            traceService, traceLevel);
    }
}
