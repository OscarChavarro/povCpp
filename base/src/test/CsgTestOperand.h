#ifndef VSDK_CSG_TEST_OPERAND_H
#define VSDK_CSG_TEST_OPERAND_H
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class CsgTestOperand : public CsgOperandView {
private:
    GeometryContainment result;
    GeometryContainment smallerResult;
    double switchLimit;
    bool malformedRay;
public:
    explicit CsgTestOperand(GeometryContainment value);
    CsgTestOperand(GeometryContainment value, GeometryContainment smaller, double limit);
    void setMalformedRay(bool value);
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    GeometryContainment classifyPointCertified(const Vector3Dd&, double) const override;
};
#endif
