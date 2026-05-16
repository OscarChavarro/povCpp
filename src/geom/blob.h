#ifndef __BLOB_H__
#define __BLOB_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

/* Blob types */
class BlobElement {
  public:
    Vector3D pos;
    DBL radius2;
    DBL coeffs[3];
    DBL tcoeffs[5];
};

class BlobList {
  public:
    BlobElement elem;
    BlobList *next;
};

class BlobInterval {
  public:
    int type, index;
    DBL bound;
};

class Blob : public Geometry {
  public:
    Transformation *Transform;
    short Inverted;
    int count;
    DBL threshold;
    BlobElement *list;
    BlobInterval *intervals;
    int Sturm_Flag;
};

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
