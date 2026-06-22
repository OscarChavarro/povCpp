#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.h"
#include "common/Config.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/material/pigment/SolidTexturePigment.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/RayShaderPipeline.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/ExponentialFogShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/TransmissionRefractionShader.h"

void
RayShaderPipeline::shadeSurface(IntersectionCandidate *rayIntersection,
    ColorRgba *color, const RayWithSegments *ray, int shadowRay,
    const TraceService *traceService, TextureUtils *textureUtils,
    const RenderContext &context, int &traceLevel)
{
    ColorRgba surfaceColor(0.0, 0.0, 0.0, 0.0);
    ColorRgba refractedColor(0.0, 0.0, 0.0, 0.0);
    ColorRgba filterColor(0.0, 0.0, 0.0, 0.0);
    PovRayMaterial *tempTexture;
    PovRayMaterial *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;

    if (!shadowRay) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    }

    surfaceColor.setR(0.0); surfaceColor.setG(0.0); surfaceColor.setB(0.0); surfaceColor.setA(0);

    ImageTexture mapFixture;
    ColorTextureFixture colorFixture(&textureUtils->getProceduralNoise(), textureUtils);

    texture = static_cast<PovRayMaterial *>(rayIntersection->getAttributes().getMaterial());
    if (texture == nullptr) {
        texture = static_cast<PovRayMaterial *>(rayIntersection->getAttributes().getObjectTexture());
    }

    // Check to see if this object/shape has a material_map texture, if so
    // then change the texture pointer to point to the mapped texture
    if (texture->getMaterialMapVariants().size() > 0) {
        const int index = mapFixture.materialMap(
            &rayIntersection->getIntersection().point, texture->getTextureTransformationInverse(),
            texture->getMaterialMapImage(),
            texture->getMaterialMapVariants().size(),
            Config::SMALL_TOLERANCE);
        if (index != -1) {
            texture = texture->getMaterialMapVariants().get(index);
        }
    }

    // If this is just a shadow ray, and we're rendering low quality, then return
    if (shadowRay && (context.getConfig().getQuality() <= 5)) {
        return;
    }

    filterColor.setR(1.0); filterColor.setG(1.0); filterColor.setB(1.0); filterColor.setA(1.0);

    // Now, we perform the lighting calculations
    for (long int _layerIdx = -1;
        _layerIdx < texture->getLayers().size() && filterColor.getA() > 0.01;
        _layerIdx++) {
        tempTexture = (_layerIdx < 0) ? texture : texture->getLayers()[_layerIdx];

        surfaceColor.setR(0.0); surfaceColor.setG(0.0); surfaceColor.setB(0.0); surfaceColor.setA(0);
        if (context.getConfig().getQuality() <= 5) {
            if (tempTexture->getQuickColor() != nullptr) {
                surfaceColor = *tempTexture->getQuickColor();
            } else if (rayIntersection->getAttributes().getObjectColor() != nullptr) {
                surfaceColor = *rayIntersection->getAttributes().getObjectColor();
            } else {
                surfaceColor.setR(0.5); surfaceColor.setG(0.5); surfaceColor.setB(0.5); surfaceColor.setA(0);
            }
        } else if (tempTexture->getPigment() != nullptr) {
            const Vector3Dd transformedPoint = SolidTexturePigment::transformToObjectSpace(
                &rayIntersection->getIntersection().point, tempTexture->getTextureTransformationInverse());
            tempTexture->getPigment()->colorAt(&transformedPoint, &surfaceColor,
                Config::SMALL_TOLERANCE, colorFixture, mapFixture);
        }
        // We don't need to compute the lighting characteristics for shadow
        // rays
        if (!shadowRay) {
            LocalSurfaceShader::shade(ray, tempTexture, rayIntersection,
                &surfaceColor, &filterColor, color, traceService,
                context.getScene().getLightSources(),
                context.getScene().getObjects(), traceLevel, textureUtils);
        }

        filterColor.setR(filterColor.getR() * surfaceColor.getR());
        filterColor.setG(filterColor.getG() * surfaceColor.getG());
        filterColor.setB(filterColor.getB() * surfaceColor.getB());

        filterColor.setA(filterColor.getA() * surfaceColor.getA());
    }

    // For shadow rays, we have the filter color now - time to return
    if (shadowRay) {
        if (filterColor.getA() < 0.01) {
            color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
            return;
        }

        if (texture->getObjectRefraction() > 0.0) {
            color->setR(color->getR() * filterColor.getR() * texture->getObjectRefraction() *
                           filterColor.getA());
            color->setG(color->getG() * filterColor.getG() * texture->getObjectRefraction() *
                             filterColor.getA());
            color->setB(color->getB() * filterColor.getB() * texture->getObjectRefraction() *
                            filterColor.getA());
        } else {
            color->setR(color->getR() * filterColor.getR() * filterColor.getA());
            color->setG(color->getG() * filterColor.getG() * filterColor.getA());
            color->setB(color->getB() * filterColor.getB() * filterColor.getA());
        }
        return;
    }

    if ((filterColor.getA() > 0.01) && (context.getConfig().getQuality() > 5)) {
        refractedColor.setR(0.0); refractedColor.setG(0.0);
        refractedColor.setB(0.0); refractedColor.setA(0);

        if (texture->getObjectRefraction() > 0.0) {
            surfaceNormal = rayIntersection->getIntersection().normal;

            if (context.getConfig().getQuality() > 7) {
                BumpNormalShader::shade(&surfaceNormal, texture, &rayIntersection->getIntersection().point,
                    &surfaceNormal, textureUtils);
            }

            // If the surface normal points away, flip its direction
            normalDirection = surfaceNormal.dotProduct(ray->getDirection());
            if (normalDirection > 0.0) {
                surfaceNormal = surfaceNormal.multiply(-1.0);
            }

            TransmissionRefractionShader::shade(texture, &rayIntersection->getIntersection().point, ray, &surfaceNormal,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(), traceLevel);
        } else {
            TransmissionRefractionShader::shade(texture, &rayIntersection->getIntersection().point, ray, nullptr,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(), traceLevel);
        }

        color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
        color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
        color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());

        if (texture->getObjectRefraction() > 0.0 &&
            texture->getObjectTransmit() > 0.0) {
            refractedColor.setR(0.0); refractedColor.setG(0.0);
            refractedColor.setB(0.0); refractedColor.setA(0);
            TransmissionRefractionShader::shade(texture, &rayIntersection->getIntersection().point, ray, nullptr,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(), traceLevel);
            color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
            color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
            color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());
        }
    }

    if (context.getScene().getFogDistance() != 0.0) {
        ExponentialFogShader::shade(rayIntersection->getIntersection().t, &context.getScene().getFogColor(),
            context.getScene().getFogDistance(), color);
    }
}
