#ifndef VSDK_QUADRIC_SURFACE_H
#define VSDK_QUADRIC_SURFACE_H

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "java/lang/Math.h"

// F(x,y,z) = square dot (x*x,y*y,z*z) + mixed dot (xy,xz,yz)
//              + linear dot (x,y,z) + constant; the solid is F <= 0.
class QuadricSurface : public Solid {
private:
    Vector3Dd square;
    Vector3Dd mixed;
    Vector3Dd linear;
    double constant;
    RayFirstHitStatus roots(const Ray& ray, const RayCrossingQuery& query,
        double& first, double& second) const;
public:
    QuadricSurface(const Vector3Dd& square, const Vector3Dd& mixed,
        const Vector3Dd& linear, double constant);
    // The evaluation order matches the historical POV implementation.
    double evaluate(const Vector3Dd& point) const {
        double result = point.dotProduct(linear) + constant;
        result += point.multiply(point).dotProduct(square);
        result += mixed.x()*point.x()*point.y() + mixed.y()*point.x()*point.z() +
            mixed.z()*point.y()*point.z();
        return result;
    }
    Vector3Dd gradient(const Vector3Dd& point) const {
        const Vector3Dd base = square.multiply(2.0).multiply(point).add(linear);
        return Vector3Dd(base.x()+mixed.x()*point.y()+mixed.y()*point.z(),
            base.y()+mixed.x()*point.x()+mixed.z()*point.z(),
            base.z()+mixed.y()*point.x()+mixed.z()*point.y());
    }
    Geometry* copyGeometry() const override;
    Vector3Dd& squareCoefficients() { return square; }
    const Vector3Dd& squareCoefficients() const { return square; }
    Vector3Dd& mixedCoefficients() { return mixed; }
    const Vector3Dd& mixedCoefficients() const { return mixed; }
    Vector3Dd& linearCoefficients() { return linear; }
    const Vector3Dd& linearCoefficients() const { return linear; }
    double& constantCoefficient() { return constant; }
    double constantCoefficient() const { return constant; }
    static bool solve(double a, double b, double c, double& first, double& second) {
        if (a != 0.0) {
            const double discriminant = b*b - 4.0*a*c;
            if (discriminant < 0.0) return false;
            const double root = java::Math::sqrt(discriminant);
            first = (-b + root)/(2.0*a);
            second = (-b - root)/(2.0*a);
        } else {
            if (b == 0.0) return false;
            first = second = -c/b;
        }
        return true;
    }
    void getBounds(GeometryBounds& result) const override;
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    int doContainmentTest(const Vector3Dd& point, double tolerance) override;
    double* getMinMax() override;
};
#endif
