#ifndef __SCENE_FACTORY_H__
#define __SCENE_FACTORY_H__

#include "common/Vector.h"
#include "geom/Objects.h"

class Composite;
class Sphere;
class Light;
class Quadric;
class Poly;
class Box;
class Blob;
class HeightField;
class InfinitePlane;
class Triangle;
class SmoothTriangle;
class CSG;
class Viewpoint;
class RGBAColor;
class BicubicPatch;

class SceneFactory {
  public:
    static Composite *getCompositeObject();
    static Sphere *getSphereShape();
    static Light *getLightSourceShape();
    static Quadric *getQuadricShape();
    static Poly *getPolyShape(int order);
    static Box *getBoxShape();
    static Blob *getBlobShape();
    static HeightField *getHeightFieldShape();
    static InfinitePlane *getPlaneShape();
    static Triangle *getTriangleShape();
    static SmoothTriangle *getSmoothTriangleShape();
    static CSG *getCsgShape();
    static CSG *getCsgUnion();
    static CSG *getCsgIntersection();
    static Viewpoint *getViewpoint();
    static RGBAColor *getColour();
    static Vector3D *getVector();
    static double *getFloat();
    static BicubicPatch *getBicubicPatchShape();
};

#endif
