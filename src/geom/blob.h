#ifndef __BLOB_H__
#define __BLOB_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

/* Blob types */
#include "geom/BlobElement.h"
#include "geom/BlobList.h"
#include "geom/BlobInterval.h"
#include "geom/BlobClass.h"

extern Methods Blob_Methods;
extern Blob *Get_Blob_Shape(void);
extern void MakeBlob(
    SimpleBody *obj, DBL threshold, BlobList *bloblist, int npoints, int sflag);
extern int All_Blob_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Blob(Vector3D *point, SimpleBody *Object);
extern void Blob_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Blob(SimpleBody *Object);
extern void Translate_Blob(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Blob(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Blob(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Blob(SimpleBody *Object);

#endif
