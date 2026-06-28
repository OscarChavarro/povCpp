#ifndef __GEOMETRY_TRANSFORM_MUTATOR__
#define __GEOMETRY_TRANSFORM_MUTATOR__

class Geometry;
class Vector3Dd;

class GeometryTransformMutator {
  public:
    static bool translateIfSupported(Geometry *geometry, Vector3Dd *vector);
    static bool rotateIfSupported(Geometry *geometry, Vector3Dd *vector);
    static bool scaleIfSupported(Geometry *geometry, Vector3Dd *vector);
};

#endif
