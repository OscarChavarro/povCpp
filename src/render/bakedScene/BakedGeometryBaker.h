#ifndef __BAKED_GEOMETRY_BAKER__
#define __BAKED_GEOMETRY_BAKER__

#include "java/util/ArrayList.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/TransformStep.h"

// Scene-compile-time engine that takes a read-only Quadric/InfinitePlane plus
// its recorded elementary TransformStep list (Phase 1) and produces a baked,
// world-space copy whose coefficients have been rewritten with the exact
// baseline formulas and the exact baseline operation order (one elementary
// congruence per step, replayed in recorded order) - see
// doc/performanceReviewPlan5.md Phase 0 appendix and Phase 2. Nothing in
// environment/geometry is touched; these are static functions operating on
// copies only.
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
