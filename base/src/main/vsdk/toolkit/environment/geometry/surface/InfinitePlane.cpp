#include <cmath>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
InfinitePlane::InfinitePlane(const InfinitePlane& other) { clone(other); }

InfinitePlane::InfinitePlane(double a_, double b_, double c_, double d_)
    : a(a_), b(b_), c(c_), d(d_) {}

InfinitePlane::InfinitePlane(const Vector3Dd& normal, const Vector3Dd& pointInPlane)
{
    Vector3Dd n = normal.normalized();
    a = n.x(); b = n.y(); c = n.z();
    d = -n.dotProduct(pointInPlane);
}

InfinitePlane::InfinitePlane(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2)
{
    Vector3Dd aa = p1.subtract(p0).normalized();
    Vector3Dd bb = p2.subtract(p0).normalized();
    Vector3Dd normal = aa.crossProduct(bb).normalized();
    a = normal.x(); b = normal.y(); c = normal.z();
    d = -normal.dotProduct(p0);
}

void InfinitePlane::clone(const InfinitePlane& other)
{
    a = other.a; b = other.b; c = other.c; d = other.d;
}

Ray* InfinitePlane::doIntersectionFirstHit(const Ray& inout_rayo)
{
    double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
    if (std::abs(denominator) < VSDK::EPSILON) return nullptr;
    double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;
    if (t < 0) return nullptr;
    return new Ray(inout_rayo.withT(t));
}

bool InfinitePlane::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    Ray* hit = doIntersectionFirstHit(inRay);
    if (hit == nullptr) return false;
    if (outHit != nullptr) {
        outHit->setRay(*hit);
        doExtraInformation(*hit, hit->getT(), outHit);
    }
    delete hit;
    return true;
}

Ray* InfinitePlane::doIntersectionWithNegative(const Ray& inout_rayo)
{
    double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
    if (std::abs(denominator) < VSDK::EPSILON) {
        Ray r(inout_rayo.getOrigin(), inout_rayo.getDirection().multiply(-1));
        Ray* hit = doIntersectionFirstHit(r);
        if (hit != nullptr) {
            Ray* out = new Ray(inout_rayo.withT(-hit->getT()));
            delete hit;
            return out;
        }
        return nullptr;
    }
    double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;
    return new Ray(inout_rayo.withT(t));
}

int InfinitePlane::doContainmentTestHalfSpace(const Vector3Dd& p, double distanceTolerance)
{
    double num = a*p.x() + b*p.y() + c*p.z() + d;
    int op = LIMIT;
    if (num > distanceTolerance) op = OUTSIDE;
    else if (num < -distanceTolerance) op = INSIDE;
    return op;
}

int InfinitePlane::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    double num = a*p.x() + b*p.y() + c*p.z() + d;
    int op = LIMIT;
    if (num > distanceTolerance) op = OUTSIDE;
    else if (num < -distanceTolerance) op = -INSIDE;
    return op;
}

void InfinitePlane::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if (outData == nullptr) return;
    outData->point = Vector3Dd(
        inRay.getOrigin().x() + inT*inRay.getDirection().x(),
        inRay.getOrigin().y() + inT*inRay.getDirection().y(),
        inRay.getOrigin().z() + inT*inRay.getDirection().z());
    outData->normal = getNormal();
}

double* InfinitePlane::getMinMax()
{
    double* minmax = new double[6];
    for (int i = 0; i < 3; i++) minmax[i] = -1e308;
    for (int i = 3; i < 6; i++) minmax[i] = 1e308;
    return minmax;
}

Vector3Dd InfinitePlane::getNormal() const { return Vector3Dd(a, b, c).normalized(); }
double InfinitePlane::getD() const { return d; }

void InfinitePlane::setNormal(const Vector3Dd& n)
{
    Vector3Dd nn = n.normalized();
    a = nn.x(); b = nn.y(); c = nn.z();
}

void InfinitePlane::setD(double d_) { d = d_; }

void InfinitePlane::setFromPointNormal(const Vector3Dd& p, const Vector3Dd& n)
{
    setNormal(n);
    d = -(n.x()*p.x() + n.y()*p.y() + n.z()*p.z());
}

double InfinitePlane::pointDistance(const Vector3Dd& p) const { return a*p.x()+b*p.y()+c*p.z()+d; }

Vector3Dd InfinitePlane::projectPoint(const Vector3Dd& p) const
{
    double distance = pointDistance(p);
    Vector3Dd n(a, b, c);
    n = n.normalized().multiply(distance);
    return p.subtract(n);
}

Vector3Dd InfinitePlane::mirrorPoint(const Vector3Dd& p) const
{
    double distance = pointDistance(p);
    Vector3Dd n(a, b, c);
    n = n.normalized().multiply(2*distance);
    return p.subtract(n);
}

bool InfinitePlane::overlapsWith(const InfinitePlane& other, double tolerance) const
{
    double a1=a,b1=b,c1=c,d1=d;
    double a2=other.a,b2=other.b,c2=other.c,d2=other.d;
    Vector3Dd n1(a1,b1,c1), n2(a2,b2,c2);
    double l1=n1.length(), l2=n2.length();
    a1/=l1; b1/=l1; c1/=l1; d1/=l1;
    a2/=l2; b2/=l2; c2/=l2; d2/=l2;
    return std::abs(a2-a1)<=tolerance && std::abs(b2-b1)<=tolerance && std::abs(c2-c1)<=tolerance && std::abs(d2-d1)<=tolerance;
}

java::String InfinitePlane::toString() const
{
    return "InfinitePlane: N=<" + VSDK::formatDouble(a) + ", " + VSDK::formatDouble(b) + ", " +
        VSDK::formatDouble(c) + ">, D=" + VSDK::formatDouble(d);
}

double InfinitePlane::getA() const { return a; }
void InfinitePlane::setA(double a_) { a = a_; }
double InfinitePlane::getB() const { return b; }
void InfinitePlane::setB(double b_) { b = b_; }
double InfinitePlane::getC() const { return c; }
void InfinitePlane::setC(double c_) { c = c_; }
