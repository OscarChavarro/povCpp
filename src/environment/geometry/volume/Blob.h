#ifndef __BLOB__
#define __BLOB__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"
#include "environment/geometry/volume/BlobList.h"

class Blob : public Geometry {
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

    int getSturmFlag() const { return sturmFlag; }
    void setSturmFlag(int flag) { sturmFlag = flag; }
    Matrix4x4d* getTransformation() const { return transformation; }
    void setTransformation(Matrix4x4d *value) { transformation = value; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }
    void setTransformationInverse(Matrix4x4d *value) { transformationInverse = value; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }
    int getCount() const { return count; }
    void setCount(int value) { count = value; }
    double getThreshold() const { return threshold; }
    void setThreshold(double value) { threshold = value; }
    BlobElement *getList() const { return list; }
    void setList(BlobElement *value) { list = value; }
    BlobInterval *getIntervals() const { return intervals; }
    void setIntervals(BlobInterval *value) { intervals = value; }

    static void makeBlob(BoundedGeometry *obj, double threshold, BlobList *bloblist,
        int npoints, int sflag);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    int count;
    double threshold;
    BlobElement *list;
    BlobInterval *intervals;
    int sturmFlag;

    static int determineInfluences(const Vector3Dd *p, const Vector3Dd *d,
        const Blob *blob, double mindist);
    static double calculateFieldValue(BoundedGeometry *obj, const Vector3Dd *pos);
    static int validateHit(const Blob *blob, const Vector3Dd *p);
};

#endif
