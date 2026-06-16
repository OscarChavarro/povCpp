#ifndef __BLOB_H__
#define __BLOB_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"
#include "environment/geometry/volume/BlobList.h"

class Blob : public Geometry {
  public:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    int count;
    double threshold;
    BlobElement *list;
    BlobInterval *intervals;
    int sturmFlag;

    int getSturmFlag() const { return sturmFlag; }
    void setSturmFlag(int flag) { sturmFlag = flag; }
    Matrix4x4d* getTransformation() const { return transformation; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }

    static void makeBlob(SimpleBody *obj, double threshold, BlobList *bloblist,
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
    static int determineInfluences(const Vector3Dd *p, const Vector3Dd *d,
        const Blob *blob, double mindist);
    static double calculateFieldValue(SimpleBody *obj, const Vector3Dd *pos);
    static int validateHit(const Blob *blob, const Vector3Dd *p);
};

#endif
