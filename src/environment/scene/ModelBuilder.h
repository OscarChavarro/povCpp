#ifndef __MODEL_BUILDER_H__
#define __MODEL_BUILDER_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

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

class ModelBuilder {
  public:
    static Composite *getCompositeObject();
    static Sphere *getSphereShape();
    static Light *getLightSourceShape();
    static Quadric *getQuadricShape();
    static PolynomialShape *getPolyShape(int order, const int *termCounts);
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
    static RGBAColor *getColor();
    static Vector3Dd *getVector();
    static double *getFloat();
    static ParametricBiCubicPatch *getBicubicPatchShape();
};

#endif
