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

#endif
