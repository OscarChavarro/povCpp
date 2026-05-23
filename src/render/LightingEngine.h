#ifndef __LIGHTING_ENGINE_H__
#define __LIGHTING_ENGINE_H__

#include "common/dataStructures/PriorityQueueNode.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

class Light;
class RayWithSegments;
class RGBAColor;
class Intersection;

class LightingEngine {
  public:
    static void fog(double distance, RGBAColor *fogColour, double fogDistance,
        RGBAColor *colour);
    static void perturbNormal(Vector3Dd *newNormal, Texture *texture,
        Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal);
    static void ambient(Texture *texture, RGBAColor *surfaceColour,
        RGBAColor *colour, double attenuation);
    static void diffuse(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *eye, Vector3Dd *surfaceNormal,
        RGBAColor *surfaceColour, RGBAColor *colour, double attenuation);
    static void reflect(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *colour);
    static void refract(Texture *texture, Vector3Dd *intersectionPoint,
        RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *colour);
    static void computeReflectedColour(RayWithSegments *ray, Texture *texture,
        Intersection *rayIntersection, RGBAColor *surfaceColour,
        RGBAColor *filterColour, RGBAColor *colour);
    static void determineSurfaceColour(Intersection *rayIntersection,
        RGBAColor *colour, RayWithSegments *ray, int shadowRay);

  private:
    static void doLight(Light *lightSource, double *lightSourceDepth,
        RayWithSegments *lightSourceRay, Vector3Dd *intersectionPoint,
        RGBAColor *lightColour);
    static int doBlocking(Intersection *localIntersection,
        RGBAColor *lightColour, PriorityQueueNode *localQueue);
    static void doPhong(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd eye, Vector3Dd *surfaceNormal, RGBAColor *colour,
        RGBAColor *lightColour, RGBAColor *surfaceColour);
    static void doSpecular(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd rEye, Vector3Dd *surfaceNormal, RGBAColor *colour,
        RGBAColor *lightColour, RGBAColor *surfaceColour);
    static void doDiffuse(Texture *texture, RayWithSegments *lightSourceRay,
        Vector3Dd *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
        RGBAColor *surfaceColour, double attenuation);
};

#endif
