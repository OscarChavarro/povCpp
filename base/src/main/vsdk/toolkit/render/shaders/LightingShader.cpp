#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/render/TraceWorkspace.h"
#include "vsdk/toolkit/render/shaders/CpuTextureSamplingConfig.h"
#include "vsdk/toolkit/render/shaders/LightingShader.h"
static Vector3Dd computeBlinnPerturbedNormal(const RayHit* info, const Vector3Dd& surfaceNormal)
{
    if ( info == 0 || info->normalMap == 0 ) {
        return surfaceNormal;
    }

    Vector3Dd normalVariation =
        CpuTextureSamplingConfig::sampleNormal(info->normalMap, info->u, 1.0 - info->v);
    if ( normalVariation.length() <= VSDK::EPSILON ) {
        return surfaceNormal;
    }

    Vector3Dd baseNormal = surfaceNormal.normalized();
    Vector3Dd surfaceTangentU = info->tangent.normalized();
    if ( surfaceTangentU.length() <= VSDK::EPSILON ) {
        return surfaceNormal;
    }

    Vector3Dd surfaceTangentV = baseNormal.crossProduct(surfaceTangentU).normalized();
    Vector3Dd nCrossPv = baseNormal.crossProduct(surfaceTangentV);
    Vector3Dd nCrossPu = baseNormal.crossProduct(surfaceTangentU);

    Vector3Dd* bumpScalePtr = info->normalMap->getBumpMapScale();
    if ( bumpScalePtr == 0 ) {
        return surfaceNormal;
    }
    Vector3Dd bumpScale = *bumpScalePtr;
    delete bumpScalePtr;

    if ( std::abs(bumpScale.x()) <= VSDK::EPSILON || std::abs(bumpScale.y()) <= VSDK::EPSILON ) {
        return surfaceNormal;
    }

    double nz = normalVariation.z();
    if ( std::abs(nz) <= VSDK::EPSILON ) {
        nz = (nz < 0.0) ? -VSDK::EPSILON : VSDK::EPSILON;
    }

    double derivativeFu =
        -2.0 * (bumpScale.z() / bumpScale.x()) * (normalVariation.x() / nz);
    double derivativeFv =
        -2.0 * (bumpScale.z() / bumpScale.y()) * (normalVariation.y() / nz);
    Vector3Dd normalPerturbation =
        nCrossPv.multiply(derivativeFu).subtract(nCrossPu.multiply(derivativeFv));

    return surfaceNormal.add(normalPerturbation).normalized();
}

LightingShader::LightingShader(bool specularEnabledIn, bool textureEnabledIn, bool bumpMapEnabledIn)
    : specularEnabled(specularEnabledIn), textureEnabled(textureEnabledIn), bumpMapEnabled(bumpMapEnabledIn)
{
}

Shader::LocalShadingResult LightingShader::shadeLocal(
    RayHit* info, double viewX, double viewY, double viewZ,
    java::ArrayList<Light*>& lights, java::ArrayList<SimpleBody*>& objects,
    SimpleMaterial* material, TraceWorkspace* workspace)
{
    Vector3Dd surfaceNormal = info->normal;
    if ( bumpMapEnabled ) {
        surfaceNormal = computeBlinnPerturbedNormal(info, surfaceNormal);
    }

    double nx = surfaceNormal.x();
    double ny = surfaceNormal.y();
    double nz = surfaceNormal.z();
    double outR = 0.0, outG = 0.0, outB = 0.0;

    for ( long int i = 0; i < lights.size(); i++ ) {
        Light* light = lights.get(i);
        if ( light == 0 ) continue;
        const ColorRgb& lightEmission = light->getEmission();

        if ( light->isAmbient() ) {
            const ColorRgb& ambient = material->getAmbientReference();
            outR += ambient.r() * lightEmission.r();
            outG += ambient.g() * lightEmission.g();
            outB += ambient.b() * lightEmission.b();
            continue;
        }

        Vector3Dd direction;
        double maxShadowDistance;
        light->getDirectionAndDistance(info->point, &direction, &maxShadowDistance);
        if ( maxShadowDistance <= VSDK::EPSILON ) continue;
        double lx = direction.x();
        double ly = direction.y();
        double lz = direction.z();

        Vector3Dd shadowOrigin(
            info->point.x() + VSDK::EPSILON * lx,
            info->point.y() + VSDK::EPSILON * ly,
            info->point.z() + VSDK::EPSILON * lz);
        Ray shadowRay(shadowOrigin, Vector3Dd(lx, ly, lz));

        double attenuation = light->evaluateLightResponseFactor(&shadowRay);
        if ( attenuation <= 0.0 ) continue;

        if ( workspace != 0 ) {
            RayHit* shadowCandidateHit = workspace->shadowCandidateHit();
            shadowCandidateHit->setStoreRay(false);

            bool shadowed = false;
            for ( long int oi = 0; oi < objects.size(); oi++ ) {
                SimpleBody* candidateObject = objects.get(oi);
                shadowCandidateHit->resetForDistanceOnly();
                RaytraceStatistics::recordShadowRay();
                if ( candidateObject != 0 && candidateObject->doIntersectionFirstHit(shadowRay, shadowCandidateHit) ) {
                    double hitDistance = shadowCandidateHit->getHitDistance();
                    if ( hitDistance > VSDK::EPSILON && hitDistance < maxShadowDistance ) {
                        shadowed = true;
                        break;
                    }
                }
            }
            if ( shadowed ) {
                continue;
            }
        }

        double lambert = nx*lx + ny*ly + nz*lz;
        if ( lambert <= 0 ) continue;

        ColorRgb diffuse = material->getDiffuseReference();
        double dr = diffuse.r(), dg = diffuse.g(), db = diffuse.b();
        if ( textureEnabled && info->texture != 0 ) {
            ColorRgb tc = CpuTextureSamplingConfig::sample(info->texture, info->u, 1 - info->v);
            dr *= tc.r(); dg *= tc.g(); db *= tc.b();
        }

        outR += attenuation * lambert * dr * lightEmission.r();
        outG += attenuation * lambert * dg * lightEmission.g();
        outB += attenuation * lambert * db * lightEmission.b();

        if ( specularEnabled ) {
            const ColorRgb& specular = material->getSpecularReference();
            double spec = viewX*(2*lambert*nx-lx) + viewY*(2*lambert*ny-ly) + viewZ*(2*lambert*nz-lz);
            if ( spec > 0 ) {
                spec = ((specular.r()+specular.g()+specular.b())/3.0) * std::pow(spec, material->getPhongExponent());
                outR += attenuation * spec*lightEmission.r();
                outG += attenuation * spec*lightEmission.g();
                outB += attenuation * spec*lightEmission.b();
            }
        }
    }

    return LocalShadingResult(surfaceNormal, ColorRgb(outR, outG, outB));
}
