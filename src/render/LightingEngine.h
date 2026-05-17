#ifndef __LIGHTING_ENGINE_H__
#define __LIGHTING_ENGINE_H__

#include "common/Vector.h"
#include "media/Texture.h"

class Light;
class Ray;
class RGBAColor;
class PriorityQueueNode;
class Intersection;

class LightingEngine {
  public:
    static void fog(double distance, RGBAColor *fogColour, double fogDistance, RGBAColor *colour);
  private:
    static void doLight(Light *lightSource, double *lightSourceDepth,
        Ray *lightSourceRay, Vector3D *intersectionPoint, RGBAColor *lightColour);
    static int doBlocking(Intersection *localIntersection, RGBAColor *lightColour,
        PriorityQueueNode *localQueue);
    static void doPhong(Texture *texture, Ray *lightSourceRay, Vector3D eye,
        Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
        RGBAColor *surfaceColour);
    static void doSpecular(Texture *texture, Ray *lightSourceRay, Vector3D rEye,
        Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
        RGBAColor *surfaceColour);
    static void doDiffuse(Texture *texture, Ray *lightSourceRay,
        Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
        RGBAColor *surfaceColour, double attenuation);

    friend void Diffuse(Texture *texture, Vector3D *intersectionPoint, Ray *eye,
        Vector3D *surfaceNormal, RGBAColor *surfaceColour, RGBAColor *colour,
        double attenuation);
};

#endif
