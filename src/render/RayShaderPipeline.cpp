#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/scene/Scene.h"
#include "render/RayShaderPipeline.h"
#include "render/SolidTextureFixturesFacade.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/ExponentialFogShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/TransmissionRefractionShader.h"

void
RayShaderPipeline::shadeSurface(Intersection *rayIntersection,
    ColorRgba *color, const RayWithSegments *ray, int shadowRay,
    const TraceService *traceService, TextureUtils *textureUtils,
    const RenderContext &context, int &traceLevel)
{
    ColorRgba surfaceColor(0.0, 0.0, 0.0, 0.0);
    ColorRgba refractedColor(0.0, 0.0, 0.0, 0.0);
    ColorRgba filterColor(0.0, 0.0, 0.0, 0.0);
    PovrayMaterial *tempTexture;
    PovrayMaterial *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;

    if (!shadowRay) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    }

    surfaceColor.setR(0.0); surfaceColor.setG(0.0); surfaceColor.setB(0.0); surfaceColor.setA(0);

    ImageTexture mapFixture;
    SolidTextureFixturesFacade fixturesFacade(&textureUtils->getProceduralNoise(), textureUtils);

    // Is there a texture in the shape?  If not, use the one in the object
    texture = static_cast<PovrayMaterial *>(rayIntersection->getOwnerSimpleBody()->getMaterial());
    if (texture == nullptr) {
        texture = static_cast<PovrayMaterial *>(rayIntersection->getBoundedGeometry()->getObjectTexture());
    }
    // Check to see if this object/shape has a material_map texture, if so
    // then change the texture pointer to point to the mapped texture - CdW 7/91
    if (texture->getTextureNumber() == (int)SolidTextureColorNames::MATERIAL_MAP_TEXTURE) {
        const int index = mapFixture.materialMap(
            &rayIntersection->getPoint(), texture->getTextureTransformationInverse(),
            texture->getMaterialImage(), texture->getMaterials().size(),
            GeometryConstants::Small_Tolerance);
        if (index != -1) {
            texture = texture->getMaterials().get(index);
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
            if (rayIntersection->getOwnerSimpleBody()->getShapeColor() != nullptr) {
                surfaceColor = *rayIntersection->getOwnerSimpleBody()->getShapeColor();
            } else if (rayIntersection->getBoundedGeometry()->getObjectColor() != nullptr) {
                surfaceColor = *rayIntersection->getBoundedGeometry()->getObjectColor();
            } else {
                surfaceColor.setR(0.5); surfaceColor.setG(0.5); surfaceColor.setB(0.5); surfaceColor.setA(0);
            }
        } else if (tempTexture->getTextureNumber() == (int)SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE) {
            PovrayMaterial * const texture1 = (PovrayMaterial *)tempTexture->getColor1();
            PovrayMaterial * const texture2 = (PovrayMaterial *)tempTexture->getColor2();
            fixturesFacade.colorAt(
                &surfaceColor, tempTexture->getTextureNumber(),
                tempTexture->getTextureTransformationInverse(), tempTexture->getImage(),
                tempTexture->getColor1(), tempTexture->getColor2(), tempTexture->getTurbulence(),
                tempTexture->getOctaves(), tempTexture->getColorMap(),
                tempTexture->getTextureGradient(), tempTexture->getMortar(),
                &rayIntersection->getPoint(), GeometryConstants::Small_Tolerance,
                SolidTextureFixturesColorAtParameterSet(
                    texture1->getTextureNumber(), texture1->getTextureTransformationInverse(),
                    texture1->getImage(), texture1->getColor1(), texture1->getColor2(),
                    texture1->getTurbulence(), texture1->getOctaves(), texture1->getColorMap(),
                    texture1->getTextureGradient(), texture1->getMortar()),
                SolidTextureFixturesColorAtParameterSet(
                    texture2->getTextureNumber(), texture2->getTextureTransformationInverse(),
                    texture2->getImage(), texture2->getColor1(), texture2->getColor2(),
                    texture2->getTurbulence(), texture2->getOctaves(), texture2->getColorMap(),
                    texture2->getTextureGradient(), texture2->getMortar()));
        } else {
            fixturesFacade.colorAt(
                &surfaceColor, tempTexture->getTextureNumber(),
                tempTexture->getTextureTransformationInverse(), tempTexture->getImage(),
                tempTexture->getColor1(), tempTexture->getColor2(), tempTexture->getTurbulence(),
                tempTexture->getOctaves(), tempTexture->getColorMap(),
                tempTexture->getTextureGradient(), tempTexture->getMortar(),
                &rayIntersection->getPoint(), GeometryConstants::Small_Tolerance);
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
        refractedColor.setR(0.0); refractedColor.setG(0.0); refractedColor.setB(0.0); refractedColor.setA(0);

        if (texture->getObjectRefraction() > 0.0) {
            rayIntersection->getOwnerSimpleBody()->normal(
                &surfaceNormal, &rayIntersection->getPoint(), ray->getConfig());

            if (context.getConfig().getQuality() > 7) {
                BumpNormalShader::shade(&surfaceNormal, texture, &rayIntersection->getPoint(),
                    &surfaceNormal, textureUtils);
            }

            // If the surface normal points away, flip its direction
            normalDirection = surfaceNormal.dotProduct(ray->getDirection());
            if (normalDirection > 0.0) {
                surfaceNormal = surfaceNormal.multiply(-1.0);
            }

            TransmissionRefractionShader::shade(texture, &rayIntersection->getPoint(), ray, &surfaceNormal,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(),
                traceLevel);
        } else {
            TransmissionRefractionShader::shade(texture, &rayIntersection->getPoint(), ray, nullptr,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(),
                traceLevel);
        }

        color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
        color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
        color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());

        if (texture->getObjectRefraction() > 0.0 &&
            texture->getObjectTransmit() > 0.0) {
            refractedColor.setR(0.0); refractedColor.setG(0.0); refractedColor.setB(0.0); refractedColor.setA(0);
            TransmissionRefractionShader::shade(texture, &rayIntersection->getPoint(), ray, nullptr,
                &refractedColor, traceService,
                context.getScene().getAtmosphereIor(),
                traceLevel);
            color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
            color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
            color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());
        }
    }

    if (context.getScene().getFogDistance() != 0.0) {
        ExponentialFogShader::shade(rayIntersection->getT(), &context.getScene().getFogColor(),
            context.getScene().getFogDistance(), color);
    }
}
