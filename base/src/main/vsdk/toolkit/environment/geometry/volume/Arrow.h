#ifndef __ARROW__
#define __ARROW__

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class Cone;
class PolyhedralBoundedSolid;
class Ray;
class RayHit;

class Arrow : public Solid {
private:
    static const double NO_HIT;
    static const int DEFAULT_CIRCUMFERENCE_DIVISIONS = 36 / 4;
    static const int DEFAULT_HEIGHT_DIVISIONS = 1;
    static const int MIN_CIRCUMFERENCE_DIVISIONS = 3;
    static const int MIN_HEIGHT_DIVISIONS = 1;

    double baseLength;
    double headLength;
    double baseRadius;
    double headRadius;

    Cone* baseCylinder;
    Cone* headCone;
    Cone* lastElement;

    bool doIntersectionDistanceOnly(const Ray& inRay, RayHit* outHit);
    static void closeTopFaceToApex(
        PolyhedralBoundedSolid* solid, int nsides, double apexZ);
    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(
        int nsides, int heightDivisions);

public:
    Arrow(double baseLength, double headLength, double baseRadius, double headRadius);
    virtual ~Arrow();

    double getBaseLength() const;
    void setBaseLength(double val);
    double getHeadLength() const;
    void setHeadLength(double val);
    double getBaseRadius() const;
    void setBaseRadius(double val);
    double getHeadRadius() const;
    void setHeadRadius(double val);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
    virtual double* getMinMax() override;
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(
        int circumferenceDivisions, int heightDivisions);
};

#endif
