#ifndef __BLOB__
#define __BLOB__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"
#include "environment/geometry/volume/BlobList.h"

class Blob : public Geometry {
  private:
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
    static int validateHit(const Blob *blob, const Vector3Dd *p);

  public:
    Blob() :
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

    Blob(double thresholdValue, BlobList *blobList, int numberOfPoints, int sturmFlagValue);
    Blob(const Matrix4x4d *transformationValue,
        const Matrix4x4d *transformationInverseValue, bool invertedValue,
        int countValue, double thresholdValue, const BlobElement *listValue,
        int sturmFlagValue);
    Blob(const Blob &other);

    int getSturmFlag() const { return sturmFlag; }
    Matrix4x4d* getTransformation() const { return transformation; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }
    bool isInverted() const { return inverted; }
    int getCount() const { return count; }
    double getThreshold() const { return threshold; }
    BlobElement *getList() const { return list; }
    BlobInterval *getIntervals() const { return intervals; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    Blob *copyWithSturmFlag(int flag) const;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;
};

#endif
