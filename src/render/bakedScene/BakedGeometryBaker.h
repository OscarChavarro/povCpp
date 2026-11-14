#ifndef __BAKED_GEOMETRY_BAKER__
#define __BAKED_GEOMETRY_BAKER__

#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/geometry/element/TransformStep.h"

class BakedGeometryBaker {
  public:
    static QuadricSurface bakeQuadric(
        const QuadricSurface &original, const java::ArrayList<TransformStep> &steps);
    static PlanarHalfSpace bakePlane(
        const PlanarHalfSpace &original, const java::ArrayList<TransformStep> &steps);

  private:
    static void quadricToMatrix(const QuadricSurface &quadric, Matrix4x4d *matrix);
    static QuadricSurface matrixToQuadric(const Matrix4x4d &matrix);
    static QuadricSurface transformQuadric(
        const QuadricSurface &shape, const Matrix4x4d &transformationInverse);
};

#endif
