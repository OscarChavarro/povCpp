#ifndef __BLOB_ELEMENT__
#define __BLOB_ELEMENT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class BlobElement {
  public:
    Vector3Dd &getPos() { return pos; }
    const Vector3Dd &getPos() const { return pos; }
    double getRadius2() const { return radius2; }
    void setRadius2(double value) { radius2 = value; }
    double *getCoeffs() { return coeffs; }
    const double *getCoeffs() const { return coeffs; }

  private:
    Vector3Dd pos;
    double radius2;
    double coeffs[3];
    // No tcoeffs[5] scratch field here on purpose: it used to cache the
    // per-ray quartic-term contribution of this element between the two
    // places Blob::doIntersectionForAllRayCrossings needs it (computed when the ray enters
    // this element's influence, consumed when it leaves), but BlobElement is
    // shared scene geometry visited by every rendering thread - under
    // `-parallel`, two threads hitting the same blob concurrently tore each
    // other's writes (the speckled/missing-pixel corruption on blob.pov).
    // Blob::doIntersectionForAllRayCrossings now keeps that scratch in a local array instead
    // (see doc/CSGPerformance.md's sibling investigation for the analogous
    // ParametricBiCubicPatch bug).
};

#endif
