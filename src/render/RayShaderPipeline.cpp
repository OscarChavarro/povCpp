#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/from2d/ImageTexture.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/SceneFrame.h"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/SolidTextureFixturesFacade.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/ExponentialFogShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/TransmissionRefractionShader.h"

void
RayShaderPipeline::shadeSurface(Intersection *rayIntersection,
    ColorRgba *color, RayWithSegments *ray, int shadowRay,
    const TraceService *traceService, TextureUtils *textureUtils)
{
    ColorRgba surfaceColor;
    ColorRgba refractedColor;
    ColorRgba filterColor;
    Material *tempTexture;
    Material *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;
    int surface;

    if (!shadowRay) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
        if (rayIntersection->Shape->shapeColor) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Depth: %f Object %d Colour %f %f %f ", rayIntersection->Depth, static_cast<int>(rayIntersection->Shape->geometryType),                 rayIntersection->Shape->shapeColor->getR(),                 rayIntersection->Shape->shapeColor->getG(),                 rayIntersection->Shape->shapeColor->getB());
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        } else {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Depth: %f Object %d Colour NIL ", rayIntersection->Depth,                 static_cast<int>(rayIntersection->Shape->geometryType));
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        }
    }

    surfaceColor.setR(0.0); surfaceColor.setG(0.0); surfaceColor.setB(0.0); surfaceColor.setA(0);

    ImageTexture mapFixture;
    SolidTextureFixturesFacade fixturesFacade(&textureUtils->getProceduralNoise(), textureUtils);

    // Is there a texture in the shape?  If not, use the one in the object
    if ((texture = rayIntersection->Shape->material) == nullptr) {
        texture = rayIntersection->Object->objectTexture;
    }
    // Check to see if this object/shape has a material_map texture, if so
    // then change the texture pointer to point to the mapped texture - CdW 7/91
    if (texture->textureNumber == (int)SolidTextureColorNames::MATERIAL_MAP_TEXTURE) {
        int index = mapFixture.materialMap(
            &rayIntersection->Point, texture->textureTransformationInverse,
            texture->materialImage, texture->materials.size(),
            GeometryConstants::Small_Tolerance);
        if (index != -1) {
            texture = texture->materials.get(index);
        }
    }

    // If this is just a shadow ray, and we're rendering low quality, then return

    if (shadowRay && (RenderingConfiguration::global().quality <= 5)) {
        return;
    }

    filterColor.setR(1.0); filterColor.setG(1.0); filterColor.setB(1.0); filterColor.setA(1.0);

    // Now, we perform the lighting calculations
    surface = 0;
    for (long int _layerIdx = -1;
        _layerIdx < texture->layers.size() && filterColor.getA() > 0.01;
        _layerIdx++) {
        tempTexture = (_layerIdx < 0) ? texture : texture->layers[_layerIdx];
        surface++;

        surfaceColor.setR(0.0); surfaceColor.setG(0.0); surfaceColor.setB(0.0); surfaceColor.setA(0);
        if (RenderingConfiguration::global().quality <= 5) {
            if (rayIntersection->Shape->shapeColor != nullptr) {
                surfaceColor = *rayIntersection->Shape->shapeColor;
            } else if (rayIntersection->Object->objectColor != nullptr) {
                surfaceColor = *rayIntersection->Object->objectColor;
            } else {
                surfaceColor.setR(0.5); surfaceColor.setG(0.5); surfaceColor.setB(0.5); surfaceColor.setA(0);
            }
        } else if (tempTexture->textureNumber == (int)SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE) {
            Material *texture1 = (Material *)tempTexture->color1;
            Material *texture2 = (Material *)tempTexture->color2;
            fixturesFacade.colorAt(
                &surfaceColor, tempTexture->textureNumber,
                tempTexture->textureTransformationInverse, tempTexture->image,
                tempTexture->color1, tempTexture->color2, tempTexture->turbulence,
                tempTexture->octaves, tempTexture->colorMap,
                tempTexture->textureGradient, tempTexture->mortar,
                &rayIntersection->Point, GeometryConstants::Small_Tolerance,
                texture1->textureNumber, texture1->textureTransformationInverse,
                texture1->image, texture1->color1, texture1->color2,
                texture1->turbulence, texture1->octaves, texture1->colorMap,
                texture1->textureGradient, texture1->mortar,
                texture2->textureNumber, texture2->textureTransformationInverse,
                texture2->image, texture2->color1, texture2->color2,
                texture2->turbulence, texture2->octaves, texture2->colorMap,
                texture2->textureGradient, texture2->mortar);
        } else {
            fixturesFacade.colorAt(
                &surfaceColor, tempTexture->textureNumber,
                tempTexture->textureTransformationInverse, tempTexture->image,
                tempTexture->color1, tempTexture->color2, tempTexture->turbulence,
                tempTexture->octaves, tempTexture->colorMap,
                tempTexture->textureGradient, tempTexture->mortar,
                &rayIntersection->Point, GeometryConstants::Small_Tolerance);
        }
        // We don't need to compute the lighting characteristics for shadow
        // rays
        if (!shadowRay) {
            LocalSurfaceShader::shade(ray, tempTexture, rayIntersection,
                &surfaceColor, &filterColor, color, traceService,
                RenderEngine::renderFrame().Light_Sources,
                RenderEngine::renderFrame().Objects, RenderEngine::traceLevel());
        }

        if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Surface %d\n", surface);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "    Surf: %6.4f %6.4f %6.4f %6.4f\n", surfaceColor.getR(),                 surfaceColor.getG(), surfaceColor.getB(), surfaceColor.getA());
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "    Filter_Colour:    %6.4f %6.4f %6.4f %6.4f  Final "                    "Colour: %6.4f %6.4f %6.4f %6.4f  \n", filterColor.getR(), filterColor.getG(), filterColor.getB(),                 filterColor.getA(), color->getR(), color->getG(), color->getB(),                 color->getA());
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
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

        if (texture->objectRefraction > 0.0) {
            color->setR(color->getR() * filterColor.getR() * texture->objectRefraction *
                           filterColor.getA());
            color->setG(color->getG() * filterColor.getG() * texture->objectRefraction *
                             filterColor.getA());
            color->setB(color->getB() * filterColor.getB() * texture->objectRefraction *
                            filterColor.getA());
        } else {
            color->setR(color->getR() * filterColor.getR() * filterColor.getA());
            color->setG(color->getG() * filterColor.getG() * filterColor.getA());
            color->setB(color->getB() * filterColor.getB() * filterColor.getA());
        }
        return;
    }

    if ((filterColor.getA() > 0.01) && (RenderingConfiguration::global().quality > 5)) {
        refractedColor.setR(0.0); refractedColor.setG(0.0); refractedColor.setB(0.0); refractedColor.setA(0);

        if (texture->objectRefraction > 0.0) {
            GeometryOperations::normal(&surfaceNormal,
                (SimpleBody *)rayIntersection->Shape, &rayIntersection->Point);

            if (RenderingConfiguration::global().quality > 7) {
                BumpNormalShader::shade(&surfaceNormal, texture, &rayIntersection->Point,
                    &surfaceNormal);
            }

            // If the surface normal points away, flip its direction
            normalDirection = surfaceNormal.dotProduct(ray->direction);
            if (normalDirection > 0.0) {
                surfaceNormal = surfaceNormal.multiply(-1.0);
            }

            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, &surfaceNormal,
                &refractedColor, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
        } else {
            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColor, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
        }

        color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
        color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
        color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());

        if (texture->objectRefraction > 0.0 &&
            texture->objectTransmit > 0.0) {
            refractedColor.setR(0.0); refractedColor.setG(0.0); refractedColor.setB(0.0); refractedColor.setA(0);
            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColor, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
            color->setR(color->getR() + filterColor.getR() * refractedColor.getR() * filterColor.getA());
            color->setG(color->getG() + filterColor.getG() * refractedColor.getG() * filterColor.getA());
            color->setB(color->getB() + filterColor.getB() * refractedColor.getB() * filterColor.getA());
        }
    }

    if (RenderEngine::renderFrame().fogDistance != 0.0) {
        ExponentialFogShader::shade(rayIntersection->Depth, &RenderEngine::renderFrame().fogColor,
            RenderEngine::renderFrame().fogDistance, color);
    }
}
