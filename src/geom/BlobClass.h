#ifndef __BLOB_CLASS_H__
#define __BLOB_CLASS_H__

#include "common/Frame.h"
#include "geom/BlobElement.h"
#include "geom/BlobInterval.h"
#include "geom/Geometry.h"

class Blob : public Geometry {
  public:
    Transformation *Transform;
    short Inverted;
    int count;
    DBL threshold;
    BlobElement *list;
    BlobInterval *intervals;
    int Sturm_Flag;

    static int allBlobIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insideBlob(Vector3D *point, SimpleBody *object);
    static void blobNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyBlob(SimpleBody *object);
    static void translateBlob(SimpleBody *object, Vector3D *vector);
    static void rotateBlob(SimpleBody *object, Vector3D *vector);
    static void scaleBlob(SimpleBody *object, Vector3D *vector);
    static void invertBlob(SimpleBody *object);

  private:
    static int determineInfluences(
        Vector3D *p, Vector3D *d, Blob *blob, DBL mindist);
    static DBL calculateFieldValue(SimpleBody *obj, Vector3D *pos);
    static int validateHit(Blob *blob, Vector3D *p);
};

#endif
