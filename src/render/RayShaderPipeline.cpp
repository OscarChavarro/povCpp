#include "java/util/ArrayList.txx"
#include "media/solidTexture/SolidTextureColorTextures.h"
#include "render/RenderEngine.h"
#include "render/RayShaderPipeline.h"
#include "render/shaders/TraceService.h"
#include "common/color/Color.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include <cstdio>
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/SceneFrame.h"
#include "media/solidTexture/ColorTextureFixture.h"
#include "media/solidTexture/MapTextureFixture.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/ExponentialFogShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/TransmissionRefractionShader.h"


void
RayShaderPipeline::shadeSurface(Intersection *rayIntersection,
    RGBAColor *color, RayWithSegments *ray, int shadowRay,
    const TraceService *traceService)
{
    RGBAColor surfaceColor;
    RGBAColor refractedColor;
    RGBAColor filterColor;
    Texture *tempTexture;
    Texture *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;
    int surface;

    if (!shadowRay) {
        Color::makeColor(color, 0.0, 0.0, 0.0);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
        if (rayIntersection->Shape->shapeColor) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Depth: %f Object %d Colour %f %f %f ", rayIntersection->Depth, rayIntersection->Shape->Type,                 rayIntersection->Shape->shapeColor->Red,                 rayIntersection->Shape->shapeColor->Green,                 rayIntersection->Shape->shapeColor->Blue);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        } else {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Depth: %f Object %d Colour NIL ", rayIntersection->Depth,                 rayIntersection->Shape->Type);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        }
    }

    Color::makeColor(&surfaceColor, 0.0, 0.0, 0.0);

    mapTextureFixture mapFixture;
    colorTextureFixture colorFixture;

    /* Is there a texture in the shape?  If not, use the one in the object. */
    if ((texture = rayIntersection->Shape->Shape_Texture) == nullptr) {
        texture = rayIntersection->Object->objectTexture;
    }
    /* Check to see if this object/shape has a material_map texture, if so */
    /* then change the texture pointer to point to the mapped texture - CdW 7/91
     */
    if (texture->textureNumber == (int)SolidTextureColorTextures::MATERIAL_MAP_TEXTURE) {
        texture = mapFixture.materialMap(
            &rayIntersection->Point, texture, GeometryConstants::Small_Tolerance);
    }

    /* If this is just a shadow ray and we're rendering low quality, then return
     */

    if (shadowRay && (RenderingConfiguration::global().quality <= 5)) {
        return;
    }

    Color::makeColor(&filterColor, 1.0, 1.0, 1.0);
    filterColor.Alpha = 1.0;

    /* Now, we perform the lighting calculations. */
    surface = 0;
    for (long int _layerIdx = -1;
        _layerIdx < texture->layers.size() && filterColor.Alpha > 0.01;
        _layerIdx++) {
        tempTexture = (_layerIdx < 0) ? texture : texture->layers[_layerIdx];
        surface++;

        Color::makeColor(&surfaceColor, 0.0, 0.0, 0.0);
        if (RenderingConfiguration::global().quality <= 5) {
            if (rayIntersection->Shape->shapeColor != nullptr) {
                surfaceColor = *rayIntersection->Shape->shapeColor;
            } else if (rayIntersection->Object->objectColor != nullptr) {
                surfaceColor = *rayIntersection->Object->objectColor;
            } else {
                Color::makeColor(&surfaceColor, 0.5, 0.5, 0.5);
            }
        } else {
            colorFixture.colorAt(
                &surfaceColor, tempTexture, &rayIntersection->Point, GeometryConstants::Small_Tolerance);
        }
        /* We don't need to compute the lighting characteristics for shadow
         * rays. */
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
                snprintf(_logMsg, sizeof(_logMsg), "    Surf: %6.4f %6.4f %6.4f %6.4f\n", surfaceColor.Red,                 surfaceColor.Green, surfaceColor.Blue, surfaceColor.Alpha);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "    Filter_Colour:    %6.4f %6.4f %6.4f %6.4f  Final "                    "Colour: %6.4f %6.4f %6.4f %6.4f  \n", filterColor.Red, filterColor.Green, filterColor.Blue,                 filterColor.Alpha, color->Red, color->Green, color->Blue,                 color->Alpha);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        }

        filterColor.Red *= surfaceColor.Red;
        filterColor.Green *= surfaceColor.Green;
        filterColor.Blue *= surfaceColor.Blue;

        filterColor.Alpha *= surfaceColor.Alpha;
    }

    /* For shadow rays, we have the filter color now - time to return */
    if (shadowRay) {

        if (filterColor.Alpha < 0.01) {
            Color::makeColor(color, 0.0, 0.0, 0.0);
            return;
        }

        if (texture->objectRefraction > 0.0) {
            color->Red *= filterColor.Red * texture->objectRefraction *
                           filterColor.Alpha;
            color->Green *= filterColor.Green * texture->objectRefraction *
                             filterColor.Alpha;
            color->Blue *= filterColor.Blue * texture->objectRefraction *
                            filterColor.Alpha;
        } else {
            color->Red *= filterColor.Red * filterColor.Alpha;
            color->Green *= filterColor.Green * filterColor.Alpha;
            color->Blue *= filterColor.Blue * filterColor.Alpha;
        }
        return;
    }

    if ((filterColor.Alpha > 0.01) && (RenderingConfiguration::global().quality > 5)) {
        Color::makeColor(&refractedColor, 0.0, 0.0, 0.0);

        if (texture->objectRefraction > 0.0) {
            GeometryOperations::normal(&surfaceNormal,
                (SimpleBody *)rayIntersection->Shape, &rayIntersection->Point);

            if (RenderingConfiguration::global().quality > 7) {
                BumpNormalShader::shade(&surfaceNormal, texture, &rayIntersection->Point,
                    &surfaceNormal);
            }

            /* If the surface normal points away, flip its direction. */
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

        color->Red +=
            filterColor.Red * refractedColor.Red * filterColor.Alpha;
        color->Green +=
            filterColor.Green * refractedColor.Green * filterColor.Alpha;
        color->Blue +=
            filterColor.Blue * refractedColor.Blue * filterColor.Alpha;

        if (texture->objectRefraction > 0.0 &&
            texture->objectTransmit > 0.0) {
            Color::makeColor(&refractedColor, 0.0, 0.0, 0.0);
            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColor, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
            color->Red +=
                filterColor.Red * refractedColor.Red * filterColor.Alpha;
            color->Green +=
                filterColor.Green * refractedColor.Green * filterColor.Alpha;
            color->Blue +=
                filterColor.Blue * refractedColor.Blue * filterColor.Alpha;
        }
    }

    if (RenderEngine::renderFrame().fogDistance != 0.0) {
        ExponentialFogShader::shade(rayIntersection->Depth, &RenderEngine::renderFrame().fogColor,
            RenderEngine::renderFrame().fogDistance, color);
    }
}
