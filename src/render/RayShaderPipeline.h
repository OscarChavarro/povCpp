#ifndef __RAY_SHADER_PIPELINE_H__
#define __RAY_SHADER_PIPELINE_H__

class Intersection;
class ColorRgba;
class RayWithSegments;
class TraceService;
class TextureUtils;

class RayShaderPipeline {
public:
    static void shadeSurface(Intersection *rayIntersection,
        ColorRgba *color, const RayWithSegments *ray, int shadowRay,
        const TraceService *traceService, TextureUtils *textureUtils);
};

#endif
