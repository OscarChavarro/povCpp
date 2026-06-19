#ifndef __MODEL_BUILDER__
#define __MODEL_BUILDER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/camera/Camera.h"
#include "environment/light/Light.h"
#include "environment/light/PointLight.h"
#include "environment/light/SpotLight.h"
#include "environment/geometry/element/Triangle.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"

class Geometry;
class SimpleBody;

class ModelBuilder {
  public:
    static SimpleBody *wrap(Geometry *geometry);
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
    static ColorRgba *getColor();
    static Vector3Dd *getVector();
    static double *getFloat();
};

#endif
