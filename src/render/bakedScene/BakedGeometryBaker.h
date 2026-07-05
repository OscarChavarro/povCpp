#ifndef __BAKED_GEOMETRY_BAKER__
#define __BAKED_GEOMETRY_BAKER__

#include "java/util/ArrayList.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/TransformStep.h"

class BakedGeometryBaker {
  public:
    static Quadric bakeQuadric(
        const Quadric &original, const java::ArrayList<TransformStep> &steps);
    static InfinitePlane bakePlane(
        const InfinitePlane &original, const java::ArrayList<TransformStep> &steps);

  private:
    static void quadricToMatrix(const Quadric &quadric, Matrix4x4d *matrix);
    static Quadric matrixToQuadric(const Matrix4x4d &matrix);
    static Quadric transformQuadric(
        const Quadric &shape, const Matrix4x4d &transformationInverse);
};

#endif
