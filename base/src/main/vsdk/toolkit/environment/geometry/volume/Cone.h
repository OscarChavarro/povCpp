#ifndef __CONE__
#define __CONE__

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class PolyhedralBoundedSolid;
class Ray;
class RayHit;

class Cone : public Solid {
private:
    double bottomRadius; // Radius at the base
    double topRadius;    // Radius at the top
    double height;       // Height

    static const int DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    static const int DEFAULT_HEIGHT_DIVISIONS = 1;
    static const int MIN_CIRCUMFERENCE_DIVISIONS = 3;
    static const int MIN_HEIGHT_DIVISIONS = 1;

    static double sq(double v);
    static bool approxEq(double a, double b);

    Ray* doIntersectionCylinder(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);
    Ray* doIntersectionCone(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);
    Ray* doIntersectionTap(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);

    static void closeTopFaceToApex(PolyhedralBoundedSolid* solid, double apexZ);
    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(int nsides,
        int heightDivisions);

public:
    Cone(double bottomRadius, double topRadius, double height);
    virtual ~Cone() {}

    double getBottomRadius() const;
    double getTopRadius() const;
    double getHeight() const;
    void setBottomRadius(double value);
    void setTopRadius(double value);
    void setHeight(double value);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;
    virtual double* getMinMax() override;

    /**
    @return a new boundary representation of the cone with the default
    divisions, owned by the caller
    */
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;

    /**
    @param circumferenceDivisions sides of the cone base (at least 3)
    @param heightDivisions vertical subdivisions (at least 1)
    @return a new boundary representation of the cone, owned by the caller
    */
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(
        int circumferenceDivisions, int heightDivisions);

};

#endif
