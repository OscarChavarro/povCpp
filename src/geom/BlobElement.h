#ifndef __BLOB_ELEMENT_H__
#define __BLOB_ELEMENT_H__

#include "common/frame.h"
#include "common/vector.h"

class BlobElement {
  public:
    Vector3D pos;
    DBL radius2;
    DBL coeffs[3];
    DBL tcoeffs[5];
};

#endif
