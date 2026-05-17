#ifndef __LIGHTING_H__
#define __LIGHTING_H__

#include "common/Frame.h"
#include "common/Ray.h"
#include "common/Vector.h"
#include "geom/Geometry.h"
#include "media/Texture.h"
class Light;
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

extern void perturbNormal(Vector3D *New_Normal, Texture *Texture,
    Vector3D *Intersection_Point, Vector3D *Surface_Normal);
extern void Ambient(Texture *Texture, RGBAColor *Surface_Colour,
    RGBAColor *Colour, double Attenuation);
extern void Diffuse(Texture *Texture, Vector3D *Intersection_Point, Ray *Eye,
    Vector3D *Surface_Normal, RGBAColor *Surface_Colour, RGBAColor *Colour,
    double Attenuation);
extern void Reflect(Texture *Texture, Vector3D *Intersection_Point, Ray *Ray,
    Vector3D *Surface_Normal, RGBAColor *Colour);
extern void Refract(Texture *Texture, Vector3D *Intersection_Point, Ray *Ray,
    Vector3D *Surface_Normal, RGBAColor *Colour);
extern void computeReflectedColour(Ray *Ray, Texture *Texture,
    Intersection *Ray_Intersection, RGBAColor *Surface_Colour,
    RGBAColor *Filter_Colour, RGBAColor *Colour);
extern void determineSurfaceColour(Intersection *Ray_Intersection,
    RGBAColor *Colour, Ray *Ray, int Shadow_Ray);

#endif
