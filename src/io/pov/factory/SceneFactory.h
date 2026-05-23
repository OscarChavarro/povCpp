#ifndef __SCENE_FACTORY_H__
#define __SCENE_FACTORY_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/Composite.h"

class Composite;
class Sphere;
class Light;
class Quadric;
class PolynomialShape;
class Box;
class Blob;
class HeightField;
class InfinitePlane;
class Triangle;
class SmoothTriangle;
class CSG;
class Camera;
class RGBAColor;
class ParametricBiCubicPatch;

class SceneFactory {
  public:
    static Composite *getCompositeObject();
    static Sphere *getSphereShape();
    static Light *getLightSourceShape();
    static Quadric *getQuadricShape();
    static PolynomialShape *getPolyShape(int order);
    static Box *getBoxShape();
    static Blob *getBlobShape();
    static HeightField *getHeightFieldShape();
    static InfinitePlane *getPlaneShape();
    static Triangle *getTriangleShape();
    static SmoothTriangle *getSmoothTriangleShape();
    static CSG *getCsgShape();
    static CSG *getCsgUnion();
    static CSG *getCsgIntersection();
    static Camera *getCamera();
    static RGBAColor *getColour();
    static Vector3Dd *getVector();
    static double *getFloat();
    static ParametricBiCubicPatch *getBicubicPatchShape();
};

#endif
