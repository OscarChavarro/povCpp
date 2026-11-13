#ifndef VSDK_RAY_CASTING_CORE_OPERAND_H
#define VSDK_RAY_CASTING_CORE_OPERAND_H
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class RayCastingBody;
// A borrowed CSG view of a POV body's placed core solid, without its
// bounding or clipping regions. It lets region policies be expressed as
// shared CSG compositions instead of a second containment algorithm.
class RayCastingCoreOperand : public CsgOperandView {
private:
    const RayCastingBody& body;
public:
    explicit RayCastingCoreOperand(const RayCastingBody& body);
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const override;
    void getSolidBounds(GeometryBounds& result) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool queryModificationVersion(long long& version) const override;
};
#endif
