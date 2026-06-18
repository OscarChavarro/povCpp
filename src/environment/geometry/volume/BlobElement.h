#ifndef __BLOB_ELEMENT_H__
#define __BLOB_ELEMENT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class BlobElement {
  public:
    Vector3Dd &getPos() { return pos; }
    const Vector3Dd &getPos() const { return pos; }
    double getRadius2() const { return radius2; }
    void setRadius2(double value) { radius2 = value; }
    double *getCoeffs() { return coeffs; }
    const double *getCoeffs() const { return coeffs; }
    double *getTCoeffs() { return tcoeffs; }
    const double *getTCoeffs() const { return tcoeffs; }

  private:
    Vector3Dd pos;
    double radius2;
    double coeffs[3];
    double tcoeffs[5];
};

#endif
