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
    RGBAColor *colour, RayWithSegments *ray, int shadowRay,
    const TraceService *traceService)
{
    RGBAColor surfaceColour;
    RGBAColor refractedColour;
    RGBAColor filterColour;
    Texture *tempTexture;
    Texture *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;
    int surface;

    if (!shadowRay) {
        Color::makeColor(colour, 0.0, 0.0, 0.0);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
        if (rayIntersection->Shape->Shape_Colour) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "Depth: %f Object %d Colour %f %f %f ", rayIntersection->Depth, rayIntersection->Shape->Type,                 rayIntersection->Shape->Shape_Colour->Red,                 rayIntersection->Shape->Shape_Colour->Green,                 rayIntersection->Shape->Shape_Colour->Blue);
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

    Color::makeColor(&surfaceColour, 0.0, 0.0, 0.0);

    mapTextureFixture mapFixture;
    colorTextureFixture colourFixture;

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

    Color::makeColor(&filterColour, 1.0, 1.0, 1.0);
    filterColour.Alpha = 1.0;

    /* Now, we perform the lighting calculations. */
    surface = 0;
    for (long int _layerIdx = -1;
        _layerIdx < texture->layers.size() && filterColour.Alpha > 0.01;
        _layerIdx++) {
        tempTexture = (_layerIdx < 0) ? texture : texture->layers[_layerIdx];
        surface++;

        Color::makeColor(&surfaceColour, 0.0, 0.0, 0.0);
        if (RenderingConfiguration::global().quality <= 5) {
            if (rayIntersection->Shape->Shape_Colour != nullptr) {
                surfaceColour = *rayIntersection->Shape->Shape_Colour;
            } else if (rayIntersection->Object->objectColour != nullptr) {
                surfaceColour = *rayIntersection->Object->objectColour;
            } else {
                Color::makeColor(&surfaceColour, 0.5, 0.5, 0.5);
            }
        } else {
            colourFixture.colorAt(
                &surfaceColour, tempTexture, &rayIntersection->Point, GeometryConstants::Small_Tolerance);
        }
        /* We don't need to compute the lighting characteristics for shadow
         * rays. */
        if (!shadowRay) {
            LocalSurfaceShader::shade(ray, tempTexture, rayIntersection,
                &surfaceColour, &filterColour, colour, traceService,
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
                snprintf(_logMsg, sizeof(_logMsg), "    Surf: %6.4f %6.4f %6.4f %6.4f\n", surfaceColour.Red,                 surfaceColour.Green, surfaceColour.Blue, surfaceColour.Alpha);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), "    Filter_Colour:    %6.4f %6.4f %6.4f %6.4f  Final "                    "Colour: %6.4f %6.4f %6.4f %6.4f  \n", filterColour.Red, filterColour.Green, filterColour.Blue,                 filterColour.Alpha, colour->Red, colour->Green, colour->Blue,                 colour->Alpha);
                Logger::reportMessage("RayShaderPipeline", Logger::WARNING, "", _logMsg);
            }
        }

        filterColour.Red *= surfaceColour.Red;
        filterColour.Green *= surfaceColour.Green;
        filterColour.Blue *= surfaceColour.Blue;

        filterColour.Alpha *= surfaceColour.Alpha;
    }

    /* For shadow rays, we have the filter colour now - time to return */
    if (shadowRay) {

        if (filterColour.Alpha < 0.01) {
            Color::makeColor(colour, 0.0, 0.0, 0.0);
            return;
        }

        if (texture->objectRefraction > 0.0) {
            colour->Red *= filterColour.Red * texture->objectRefraction *
                           filterColour.Alpha;
            colour->Green *= filterColour.Green * texture->objectRefraction *
                             filterColour.Alpha;
            colour->Blue *= filterColour.Blue * texture->objectRefraction *
                            filterColour.Alpha;
        } else {
            colour->Red *= filterColour.Red * filterColour.Alpha;
            colour->Green *= filterColour.Green * filterColour.Alpha;
            colour->Blue *= filterColour.Blue * filterColour.Alpha;
        }
        return;
    }

    if ((filterColour.Alpha > 0.01) && (RenderingConfiguration::global().quality > 5)) {
        Color::makeColor(&refractedColour, 0.0, 0.0, 0.0);

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
                &refractedColour, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
        } else {
            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
        }

        colour->Red +=
            filterColour.Red * refractedColour.Red * filterColour.Alpha;
        colour->Green +=
            filterColour.Green * refractedColour.Green * filterColour.Alpha;
        colour->Blue +=
            filterColour.Blue * refractedColour.Blue * filterColour.Alpha;

        if (texture->objectRefraction > 0.0 &&
            texture->objectTransmit > 0.0) {
            Color::makeColor(&refractedColour, 0.0, 0.0, 0.0);
            TransmissionRefractionShader::shade(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour, traceService,
                RenderEngine::renderFrame().atmosphereIor,
                RenderEngine::traceLevel());
            colour->Red +=
                filterColour.Red * refractedColour.Red * filterColour.Alpha;
            colour->Green +=
                filterColour.Green * refractedColour.Green * filterColour.Alpha;
            colour->Blue +=
                filterColour.Blue * refractedColour.Blue * filterColour.Alpha;
        }
    }

    if (RenderEngine::renderFrame().fogDistance != 0.0) {
        ExponentialFogShader::shade(rayIntersection->Depth, &RenderEngine::renderFrame().fogColour,
            RenderEngine::renderFrame().fogDistance, colour);
    }
}
