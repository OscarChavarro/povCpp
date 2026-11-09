#include "java/lang/Math.h"
#include <cmath>

#include "java/util/ArrayList.txx"

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
Torus::Torus(double inMajorRadius, double inMinorRadius)
    : majorRadius(inMajorRadius), minorRadius(inMinorRadius) {}

double Torus::getMajorRadius() const { return majorRadius; }
void Torus::setMajorRadius(double rMajor) { majorRadius = rMajor; }
double Torus::getMinorRadius() const { return minorRadius; }
void Torus::setMinorRadius(double rMinor) { minorRadius = rMinor; }

namespace {
/// The roots the quartic solver gives for a ray are accepted only if the
/// point they define is on the surface of the torus, within this distance
/// (relative to the major radius): verified roots have errors around 1e-11,
/// and spurious ones (the solver gives some) errors of the order of 0.1
const double ROOT_VALIDATION_TOLERANCE = 1.0e-6;
}

Ray* Torus::doIntersectionFirstHit(const Ray& inOutRay) {
    Vector3Dd origin = inOutRay.getOrigin();

    Ray ray = inOutRay.withDirection(inOutRay.getDirection().normalized());
    Vector3Dd d = ray.getDirection();

    // Distance from the original origin to the point nearest to the center
    double shift = -origin.dotProduct(d);
    Vector3Dd p = origin.add(d.multiply(shift));

    double alpha, beta, gama;

    alpha = d.dotProduct(d);
    beta = 2 * p.dotProduct(d);
    gama = p.dotProduct(p) - (minorRadius * minorRadius) - (majorRadius * majorRadius);

    double a4, a3, a2, a1, a0;

    a4 = alpha * alpha;
    a3 = 2 * alpha * beta;
    a2 = (beta * beta) + 2 * alpha * gama + 4 * (majorRadius * majorRadius) * (d.z() * d.z());
    a1 = 2 * beta * gama + 8 * (majorRadius * majorRadius) * p.z() * d.z();
    a0 = (gama * gama) + 4 * (majorRadius * majorRadius) * (p.z() * p.z()) - (4 * (majorRadius * majorRadius) * (minorRadius * minorRadius));

    java::ArrayList<double> polynomial;
    polynomial.add(a0);
    polynomial.add(a1);
    polynomial.add(a2);
    polynomial.add(a3);
    polynomial.add(a4);
    java::ArrayList<double> roots = findRealRoots(polynomial);
    double mRoot = 0;
    int count = 0;

    for ( long i = 0; i < roots.size(); i++ ) {
        // Distance along the ray that was given
        double t = shift + roots.get(i);

        if ( t > 0 && isOnSurface(origin, d, t) ) {
            if ( count == 0 || t < mRoot ) {
                mRoot = t;
                count++;
            }
        }
    }

    if ( count == 0 ) {
        return nullptr;
    }
    return new Ray(ray.withT(mRoot));
}

java::ArrayList<double> Torus::findRealRoots(
    const java::ArrayList<double>& polynomial)
{
    int degree = (int)polynomial.size() - 1;
    java::ArrayList<double> found;

    while ( degree > 0 && polynomial.get(degree) == 0.0 ) {
        degree--;
    }
    if ( degree <= 0 ) {
        return found;
    }
    if ( degree == 1 ) {
        found.add(-polynomial.get(0) / polynomial.get(1));
        return found;
    }

    java::ArrayList<double> derivative;

    for ( int k = 1; k <= degree; k++ ) {
        derivative.add(k * polynomial.get(k));
    }
    java::ArrayList<double> critical = findRealRoots(derivative);

    // Cauchy bound: every root is inside it
    double bound = 0;

    for ( int k = 0; k < degree; k++ ) {
        bound = java::Math::max(bound, std::fabs(polynomial.get(k) / polynomial.get(degree)));
    }
    bound += 1;

    java::ArrayList<double> limits;

    limits.add(-bound);
    for ( long k = 0; k < critical.size(); k++ ) {
        limits.add(java::Math::max(-bound, java::Math::min(bound, critical.get(k))));
    }
    limits.add(bound);

    for ( long k = 0; k < limits.size() - 1 && found.size() < degree; k++ ) {
        double low = limits.get(k);
        double high = limits.get(k + 1);
        double valueAtLow = evaluate(polynomial, degree, low);
        double valueAtHigh = evaluate(polynomial, degree, high);

        if ( valueAtLow == 0.0 ) {
            if ( found.size() == 0 || found.get(found.size() - 1) != low ) {
                found.add(low);
            }
        }
        else if ( valueAtHigh != 0.0 && (valueAtLow < 0.0) != (valueAtHigh < 0.0) ) {
            for ( int iteration = 0; iteration < 200; iteration++ ) {
                double middle = 0.5 * (low + high);

                if ( middle <= low || middle >= high ) {
                    break;
                }
                if ( (evaluate(polynomial, degree, middle) < 0.0) == (valueAtLow < 0.0) ) {
                    low = middle;
                }
                else {
                    high = middle;
                }
            }
            found.add(0.5 * (low + high));
        }
    }
    if ( found.size() < degree && evaluate(polynomial, degree, bound) == 0.0 ) {
        found.add(bound);
    }
    return found;
}

double Torus::evaluate(const java::ArrayList<double>& polynomial, int degree,
                       double x)
{
    double value = 0;

    for ( int k = degree; k >= 0; k-- ) {
        value = value * x + polynomial.get(k);
    }
    return value;
}

bool Torus::isOnSurface(const Vector3Dd& origin, const Vector3Dd& direction,
                        double t) const
{
    Vector3Dd hit = origin.add(direction.multiply(t));
    double distanceToCircle = std::hypot(std::hypot(hit.x(), hit.y()) - majorRadius, hit.z());
    double tolerance = ROOT_VALIDATION_TOLERANCE * java::Math::max(majorRadius, minorRadius);

    return std::fabs(distanceToCircle - minorRadius) <= tolerance;
}

bool Torus::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    Ray* hit = doIntersectionFirstHit(inRay);
    if (hit == nullptr) return false;

    if (outHit != nullptr) {
        outHit->setRay(*hit);
        doExtraInformation(*hit, hit->getT(), outHit);
        outHit->setRay(*hit);
    }
    delete hit;
    return true;
}

void Torus::doExtraInformation(const Ray& inRay, double inT, RayHit* outHit) {
    if (outHit == nullptr) return;

    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
    if (outHit->needsPoint()) {
        outHit->point = p;
    }

    if (outHit->needsNormal() || outHit->needsTextureCoordinates() || outHit->needsTangent()) {
        double x = p.x(), y = p.y(), z = p.z();
        double sum = x*x + y*y + z*z + majorRadius*majorRadius - minorRadius*minorRadius;
        Vector3Dd grad(
            4*x*sum - 8*majorRadius*majorRadius*x,
            4*y*sum - 8*majorRadius*majorRadius*y,
            4*z*sum);
        Vector3Dd n = grad.normalized();
        if (outHit->needsNormal()) outHit->normal = n;

        if (outHit->needsTextureCoordinates()) {
            double theta = std::atan2(y, x);
            double ring = std::sqrt(x*x + y*y);
            double phi = std::atan2(z, ring - majorRadius);
            outHit->u = (theta + M_PI) / (2.0 * M_PI);
            outHit->v = (phi + M_PI) / (2.0 * M_PI);
        }

        if (outHit->needsTangent()) {
            double theta = std::atan2(y, x);
            outHit->tangent = Vector3Dd(-std::sin(theta), std::cos(theta), 0);
        }
    }
}

double* Torus::getMinMax() {
    double* m = new double[6];
    double ext = majorRadius + minorRadius;
    m[0] = -ext; m[1] = -ext; m[2] = -minorRadius;
    m[3] =  ext; m[4] =  ext; m[5] =  minorRadius;
    return m;
}
