#include <cmath>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/environment/material/MicroFacetedMaterial.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/render/TraceWorkspace.h"
#include "vsdk/toolkit/render/shaders/CookTorranceShader.h"
#include "vsdk/toolkit/render/shaders/CpuTextureSamplingConfig.h"
const double DEFAULT_ROUGHNESS = 0.35;
const double MIN_ROUGHNESS = 0.02;
const double EPS = 1e-8;

struct LightDirection {
    double x, y, z;
    double maxShadowDistance;
};

struct MicrofacetParams {
    double roughness;
    double alpha;
    ColorRgb fresnelF0;
    double kd;
    double ks;
};

static Vector3Dd computeBlinnPerturbedNormal(const RayHit* info, const Vector3Dd& surfaceNormal)
{
    if ( info == 0 || info->normalMap == 0 ) return surfaceNormal;
    Vector3Dd normalVariation = CpuTextureSamplingConfig::sampleNormal(info->normalMap, info->u, 1.0 - info->v);
    if ( normalVariation.length() <= VSDK::EPSILON ) return surfaceNormal;

    Vector3Dd baseNormal = surfaceNormal.normalized();
    Vector3Dd surfaceTangentU = info->tangent.normalized();
    if ( surfaceTangentU.length() <= VSDK::EPSILON ) return surfaceNormal;

    Vector3Dd surfaceTangentV = baseNormal.crossProduct(surfaceTangentU).normalized();
    Vector3Dd nCrossPv = baseNormal.crossProduct(surfaceTangentV);
    Vector3Dd nCrossPu = baseNormal.crossProduct(surfaceTangentU);

    Vector3Dd* bumpScalePtr = info->normalMap->getBumpMapScale();
    if ( bumpScalePtr == 0 ) return surfaceNormal;
    Vector3Dd bumpScale = *bumpScalePtr;
    delete bumpScalePtr;

    double nz = normalVariation.z();
    if ( std::abs(nz) <= VSDK::EPSILON ) nz = (nz < 0.0) ? -VSDK::EPSILON : VSDK::EPSILON;
    double derivativeFu = -2.0 * (bumpScale.z() / bumpScale.x()) * (normalVariation.x() / nz);
    double derivativeFv = -2.0 * (bumpScale.z() / bumpScale.y()) * (normalVariation.y() / nz);
    Vector3Dd normalPerturbation = nCrossPv.multiply(derivativeFu).subtract(nCrossPu.multiply(derivativeFv));
    return surfaceNormal.add(normalPerturbation).normalized();
}

static double beckmannDistribution(double ndotH, double roughness)
{
    if ( ndotH <= 0.0 ) return 0.0;
    double ndotHSquared = ndotH * ndotH;
    double tanSquaredTheta = (1.0 - ndotHSquared) / java::Math::max(EPS, ndotHSquared);
    double mSquared = roughness * roughness;
    double exponent = -tanSquaredTheta / java::Math::max(EPS, mSquared);
    return std::exp(exponent) / (M_PI * java::Math::max(EPS, mSquared) * ndotHSquared * ndotHSquared);
}

static double smithSchlickGeometry(double ndotV, double ndotL, double alpha)
{
    double k = alpha * 0.5;
    double gV = ndotV / (ndotV * (1.0 - k) + k);
    double gL = ndotL / (ndotL * (1.0 - k) + k);
    return gV * gL;
}

static LightDirection resolveLightDirection(const Light* light, const RayHit* info, bool* ok)
{
    *ok = false;
    LightDirection ld = {0, 0, 0, 0};
    if ( light == 0 || info == 0 ) return ld;
    Vector3Dd direction;
    double maxShadowDistance;
    light->getDirectionAndDistance(info->point, &direction, &maxShadowDistance);
    if ( maxShadowDistance <= VSDK::EPSILON ) return ld;
    ld.x = direction.x(); ld.y = direction.y(); ld.z = direction.z();
    ld.maxShadowDistance = maxShadowDistance;
    *ok = true;
    return ld;
}

static bool isShadowed(
    const RayHit* info,
    double lightDirX, double lightDirY, double lightDirZ,
    double maxShadowDistance,
    java::ArrayList<SimpleBody*>& objects,
    TraceWorkspace* workspace)
{
    if ( maxShadowDistance <= VSDK::EPSILON ) return true;
    if ( workspace == 0 ) return false;
    Vector3Dd shadowOrigin(
        info->point.x() + VSDK::EPSILON * lightDirX,
        info->point.y() + VSDK::EPSILON * lightDirY,
        info->point.z() + VSDK::EPSILON * lightDirZ);
    Ray shadowRay(shadowOrigin, Vector3Dd(lightDirX, lightDirY, lightDirZ));
    RayHit* shadowCandidateHit = workspace->shadowCandidateHit();
    shadowCandidateHit->setStoreRay(false);
    for ( long int i = 0; i < objects.size(); i++ ) {
        SimpleBody* candidateObject = objects.get(i);
        shadowCandidateHit->resetForDistanceOnly();
        if ( candidateObject && candidateObject->doIntersectionFirstHit(shadowRay, shadowCandidateHit) ) {
            double hitDistance = shadowCandidateHit->getHitDistance();
            if ( hitDistance > VSDK::EPSILON && hitDistance < maxShadowDistance ) {
                return true;
            }
        }
    }
    return false;
}

static MicrofacetParams resolveMicrofacetParams(SimpleMaterial* material)
{
    MicrofacetParams p;
    p.roughness = DEFAULT_ROUGHNESS;
    p.alpha = p.roughness * p.roughness;
    p.fresnelF0 = material->getSpecularReference();
    p.kd = 1.0;
    p.ks = 1.0;
    MicroFacetedMaterial* mf = dynamic_cast<MicroFacetedMaterial*>(material);
    if ( mf != 0 ) {
        p.roughness = mf->getRoughness();
        p.alpha = mf->getAlpha();
        p.fresnelF0 = mf->getFresnelF0();
        p.kd = mf->getKd();
        p.ks = mf->getKs();
    }
    return p;
}
CookTorranceShader::CookTorranceShader(bool textureEnabledIn, bool bumpMapEnabledIn)
    : textureEnabled(textureEnabledIn), bumpMapEnabled(bumpMapEnabledIn)
{
}

Shader::LocalShadingResult CookTorranceShader::shadeLocal(RayHit* info,double viewX,double viewY,double viewZ,java::ArrayList<Light*>& lights,java::ArrayList<SimpleBody*>& objects,SimpleMaterial* material,TraceWorkspace* workspace)
{
    Vector3Dd surfaceNormal = info->normal;
    if ( bumpMapEnabled ) {
        surfaceNormal = computeBlinnPerturbedNormal(info, surfaceNormal);
    }

    Vector3Dd normal = surfaceNormal.normalized();
    double normalX = normal.x();
    double normalY = normal.y();
    double normalZ = normal.z();

    double viewLength = std::sqrt(viewX * viewX + viewY * viewY + viewZ * viewZ);
    if ( viewLength <= EPS ) return LocalShadingResult(surfaceNormal, ColorRgb(0.0, 0.0, 0.0));
    double invViewLength = 1.0 / viewLength;
    double viewDirX = viewX * invViewLength;
    double viewDirY = viewY * invViewLength;
    double viewDirZ = viewZ * invViewLength;

    const ColorRgb& ambient = material->getAmbientReference();
    double outR = 0.0;
    double outG = 0.0;
    double outB = 0.0;

    for ( long int i = 0; i < lights.size(); i++ ) {
        Light* light = lights.get(i);
        if ( light == 0 ) continue;
        const ColorRgb& lightEmission = light->getEmission();
        if ( light->isAmbient() ) {
            outR += ambient.r() * lightEmission.r();
            outG += ambient.g() * lightEmission.g();
            outB += ambient.b() * lightEmission.b();
            continue;
        }

        bool ok = false;
        LightDirection ld = resolveLightDirection(light, info, &ok);
        if ( !ok ) continue;

        Ray lightSourceRay(info->point, Vector3Dd(ld.x, ld.y, ld.z));
        double attenuation = light->evaluateLightResponseFactor(&lightSourceRay);
        if ( attenuation <= 0.0 ) continue;
        if ( isShadowed(info, ld.x, ld.y, ld.z, ld.maxShadowDistance, objects, workspace) ) continue;

        double ndotL = java::Math::max(0.0, normalX * ld.x + normalY * ld.y + normalZ * ld.z);
        if ( ndotL <= 0.0 ) continue;
        double ndotV = java::Math::max(0.0, normalX * viewDirX + normalY * viewDirY + normalZ * viewDirZ);
        if ( ndotV <= 0.0 ) continue;

        double halfX = ld.x + viewDirX;
        double halfY = ld.y + viewDirY;
        double halfZ = ld.z + viewDirZ;
        double halfLength = std::sqrt(halfX * halfX + halfY * halfY + halfZ * halfZ);
        if ( halfLength <= EPS ) continue;
        double invHalf = 1.0 / halfLength;
        halfX *= invHalf; halfY *= invHalf; halfZ *= invHalf;
        double ndotH = java::Math::max(0.0, normalX * halfX + normalY * halfY + normalZ * halfZ);
        double vdotH = java::Math::max(0.0, viewDirX * halfX + viewDirY * halfY + viewDirZ * halfZ);

        MicrofacetParams p = resolveMicrofacetParams(material);
        double roughness = java::Math::max(MIN_ROUGHNESS, p.roughness);
        double alpha = java::Math::max(MIN_ROUGHNESS * MIN_ROUGHNESS, p.alpha);

        const ColorRgb& baseDiffuse = material->getDiffuseReference();
        double diffuseR = baseDiffuse.r();
        double diffuseG = baseDiffuse.g();
        double diffuseB = baseDiffuse.b();
        if ( textureEnabled && info->texture != 0 ) {
            ColorRgb tc = CpuTextureSamplingConfig::sample(info->texture, info->u, 1.0 - info->v);
            diffuseR *= tc.r();
            diffuseG *= tc.g();
            diffuseB *= tc.b();
        }

        double distribution = beckmannDistribution(ndotH, roughness);
        double geometry = smithSchlickGeometry(ndotV, ndotL, alpha);
        double fresnelPower = std::pow(java::Math::max(0.0, 1.0 - vdotH), 5.0);
        double fresnelR = p.fresnelF0.r() + (1.0 - p.fresnelF0.r()) * fresnelPower;
        double fresnelG = p.fresnelF0.g() + (1.0 - p.fresnelF0.g()) * fresnelPower;
        double fresnelB = p.fresnelF0.b() + (1.0 - p.fresnelF0.b()) * fresnelPower;
        double denominator = java::Math::max(EPS, 4.0 * ndotL * ndotV);

        double specR = p.ks * distribution * geometry * fresnelR / denominator;
        double specG = p.ks * distribution * geometry * fresnelG / denominator;
        double specB = p.ks * distribution * geometry * fresnelB / denominator;

        outR += attenuation * lightEmission.r() * (p.kd * diffuseR * ndotL + specR);
        outG += attenuation * lightEmission.g() * (p.kd * diffuseG * ndotL + specG);
        outB += attenuation * lightEmission.b() * (p.kd * diffuseB * ndotL + specB);
    }

    return LocalShadingResult(surfaceNormal, ColorRgb(outR, outG, outB));
}
