#ifndef __RAY_SHADER_PIPELINE_H__
#define __RAY_SHADER_PIPELINE_H__

class Intersection;
class RGBAColor;
class RayWithSegments;
class TraceService;

class RayShaderPipeline {
public:
    static void shadeSurface(Intersection *rayIntersection,
        RGBAColor *colour, RayWithSegments *ray, int shadowRay,
        const TraceService *traceService);
};

#endif
