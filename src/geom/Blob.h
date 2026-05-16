#ifndef __BLOB_H__
#define __BLOB_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

/* Blob types */
#include "geom/BlobClass.h"
#include "geom/BlobElement.h"
#include "geom/BlobInterval.h"
#include "geom/BlobList.h"

extern Methods Blob_Methods;
extern Blob *getBlobShape(void);
extern void MakeBlob(
    SimpleBody *obj, DBL threshold, BlobList *bloblist, int npoints, int sflag);
extern int allBlobIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insideBlob(Vector3D *point, SimpleBody *Object);
extern void blobNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyBlob(SimpleBody *Object);
extern void translateBlob(SimpleBody *Object, Vector3D *Vector);
extern void rotateBlob(SimpleBody *Object, Vector3D *Vector);
extern void scaleBlob(SimpleBody *Object, Vector3D *Vector);
extern void invertBlob(SimpleBody *Object);

#endif
