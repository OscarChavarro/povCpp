#ifndef __BLOB__
#define __BLOB__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"

class Blob : public Geometry {
  private:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr double INSIDE_TOLERANCE = 1.0e-6;
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

    static BlobElement *allocateBlobElements(int count);
    static BlobInterval *allocateBlobIntervals(int count);

    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    const int count;
    const double threshold;
    BlobElement *const list;
    BlobInterval *const intervals;
    const int sturmFlag;

    static int determineInfluences(const Vector3Dd *p, const Vector3Dd *d,
        const Blob *blob, double minimumDistance);
    static double calculateFieldValue(BoundedGeometry *obj, const Vector3Dd *pos);
    static bool validateHit(const Blob *blob, const Vector3Dd *p);

  public:
    Blob();
    Blob(double thresholdValue,
        java::ArrayList<BlobElement *> *blobElements, int numberOfPoints,
        int sturmFlagValue);
    Blob(const Matrix4x4d *transformationValue,
        const Matrix4x4d *transformationInverseValue, bool invertedValue,
        int countValue, double thresholdValue, const BlobElement *listValue,
        int sturmFlagValue);
    Blob(const Blob &other);

    int getSturmFlag() const { return sturmFlag; }
    Blob *copyWithSturmFlag(int flag) const;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;
};

inline
Blob::Blob() :
    transformation(nullptr),
    transformationInverse(nullptr),
    inverted(false),
    count(0),
    threshold(0.0),
    list(nullptr),
    intervals(nullptr),
    sturmFlag(0)
{
}

#endif
