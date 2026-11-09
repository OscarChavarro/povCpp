#ifndef __TORUS__
#define __TORUS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class Ray;
class RayHit;

class Torus : public Solid {
private:
    double majorRadius;
    double minorRadius;

    /**
    Finds the real roots of a polynomial, in ascending order, isolating them
    with the roots of its derivative (that separate the intervals where the
    polynomial is monotonic) and bisecting the intervals where it changes its
    sign. Roots of even multiplicity (tangencies) are found only if they are
    a root of the derivative too.
    @param polynomial coefficients, in ascending order of the powers
    @return the real roots
    */
    static java::ArrayList<double> findRealRoots(
        const java::ArrayList<double>& polynomial);
    static double evaluate(const java::ArrayList<double>& polynomial,
                           int degree, double x);

    /**
    Checks a root of the intersection equation: the solver of the equation can
    give real roots that do not correspond to any point of the torus.
    @param origin origin of the ray
    @param direction unit direction of the ray
    @param t distance along the ray to check
    @return true if the point of the ray at the distance is on the surface of
    the torus
    */
    bool isOnSurface(const Vector3Dd& origin, const Vector3Dd& direction,
                     double t) const;

public:
    Torus(double inMajorRadius, double inMinorRadius);
    virtual ~Torus() {}

    double getMajorRadius() const;
    void setMajorRadius(double rMajor);
    double getMinorRadius() const;
    void setMinorRadius(double rMinor);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
    virtual double* getMinMax();
};

#endif
