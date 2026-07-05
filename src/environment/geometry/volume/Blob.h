#ifndef __BLOB__
#define __BLOB__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"

class Blob : public Geometry {
  private:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr double INSIDE_TOLERANCE = 1.0e-6;
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

    static BlobElement *allocateBlobElements(int count);

    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    const int count;
    const double threshold;
    BlobElement *const list;
    const int sturmFlag;

    // intervals is supplied by the caller (doIntersectionForAllRayCrossings), sized to
    // 2*blob->count, rather than read from a shared instance field: see
    // BlobElement.h's comment on the analogous tcoeffs removal.
    static int determineInfluences(const Vector3Dd *p, const Vector3Dd *d,
        const Blob *blob, double minimumDistance, BlobInterval *intervals);
    static double calculateFieldValue(const Blob *blob, const Vector3Dd *pos);
    static bool validateHit(const Blob *blob, const Vector3Dd *p);
    int traceCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        const GeometryIntersectionEmissionContext *context);

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
    ~Blob() override;

    int getSturmFlag() const { return sturmFlag; }
    Blob *copyWithSturmFlag(int flag) const;

    AxisAlignedBoundingBox getMinMax() const override;

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
};

inline
Blob::Blob() :
    transformation(nullptr),
    transformationInverse(nullptr),
    inverted(false),
    count(0),
    threshold(0.0),
    list(nullptr),
    sturmFlag(0)
{
}

#endif
