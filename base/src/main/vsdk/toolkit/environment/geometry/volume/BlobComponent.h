#ifndef VSDK_BLOB_COMPONENT_H
#define VSDK_BLOB_COMPONENT_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Spherical field component of a BlobSolid: strength * (1 - d^2/radius^2)^2
// at distance d < radius from the center, and zero beyond. The density is
// evaluated as c0*d^4 + c1*d^2 + c2 with the PovRay coefficients.
class BlobComponent {
private:
    Vector3Dd center;
    double radius;
    double strength;
    double radiusSquared;
    double coefficients[3];
public:
    BlobComponent();
    BlobComponent(const Vector3Dd& center, double radius, double strength);
    const Vector3Dd& getCenter() const { return center; }
    double getRadius() const { return radius; }
    double getStrength() const { return strength; }
    double getRadiusSquared() const { return radiusSquared; }
    // c0, c1 and c2 of the density polynomial in d^2.
    const double* getCoefficients() const { return coefficients; }
    // PovRay rejects radii or strengths below 1e-5 in magnitude.
    bool isDegenerate() const;
};
#endif
