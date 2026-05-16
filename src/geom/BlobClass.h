#ifndef __BLOB_CLASS_H__
#define __BLOB_CLASS_H__

#include "common/frame.h"
#include "geom/BlobElement.h"
#include "geom/BlobInterval.h"
#include "geom/geometry.h"

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

#endif
