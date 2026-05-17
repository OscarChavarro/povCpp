#ifndef __BLOB_ELEMENT_H__
#define __BLOB_ELEMENT_H__

#include "common/FrameConfig.h"
#include "common/Vector3D.h"

class BlobElement {
  public:
    Vector3D pos;
    double radius2;
    double coeffs[3];
    double tcoeffs[5];
};

#endif
