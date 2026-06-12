#ifndef __BLOB_H__
#define __BLOB_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/BlobElement.h"
#include "environment/geometry/volume/BlobInterval.h"
#include "environment/geometry/volume/BlobList.h"

class Blob : public Geometry {
  public:
    static Methods methodTable;
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool Inverted;
    int count;
    double threshold;
    BlobElement *list;
    BlobInterval *intervals;
    int sturmFlag;

    static int allBlobIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int insideBlob(Vector3Dd *point, SimpleBody *object);
    static void blobNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyBlob(SimpleBody *object);
    static void translateBlob(SimpleBody *object, Vector3Dd *vector);
    static void rotateBlob(SimpleBody *object, Vector3Dd *vector);
    static void scaleBlob(SimpleBody *object, Vector3Dd *vector);
    static void invertBlob(SimpleBody *object);
    static void makeBlob(SimpleBody *obj, double threshold, BlobList *bloblist,
        int npoints, int sflag);

  private:
    static int determineInfluences(const Vector3Dd *p, const Vector3Dd *d,
        const Blob *blob, double mindist);
    static double calculateFieldValue(SimpleBody *obj, const Vector3Dd *pos);
    static int validateHit(const Blob *blob, const Vector3Dd *p);
};

#endif
