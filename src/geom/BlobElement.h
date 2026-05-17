#ifndef __BLOB_ELEMENT_H__
#define __BLOB_ELEMENT_H__

#include "common/Frame.h"
#include "common/Vector.h"

class BlobElement {
  public:
    Vector3D pos;
    double radius2;
    double coeffs[3];
    double tcoeffs[5];
};

#endif
