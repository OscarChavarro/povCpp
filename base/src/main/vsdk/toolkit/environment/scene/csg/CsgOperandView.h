#ifndef VSDK_CSG_OPERAND_VIEW_H
#define VSDK_CSG_OPERAND_VIEW_H

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/AffinePlacementStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
class Geometry;
class GeometryBounds;
class CsgExpression;
class Ray;
class RayCrossingQuery;
class RaySolidClassification;

// A borrowed operand view; its geometry, child expression and placement live
// at least as long as the expression that references this view.
class CsgOperandView {
public:
    virtual ~CsgOperandView();
    virtual const Geometry* getGeometry() const = 0;
    virtual Matrix4x4d getAffinePlacement() const = 0;
    // Output matrices are usable only when the returned status is Valid.
    virtual AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const;
    virtual const CsgExpression* getChildExpression() const;
    // Opt in only when child and placement fully describe this view's solid.
    virtual bool isStructuralTraversalSafe() const;
    // True only when both views denote the very same placed solid.
    virtual bool isSameSolid(const CsgOperandView& other) const;
    // World-space certified query; unknown operand implementations stay explicit.
    virtual GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const;
    // Conservative parent-space bounds of a leaf's solid; Unsupported by default.
    virtual void getSolidBounds(GeometryBounds& result) const;
    // Solid segments of a leaf in this operand's parent coordinates.
    virtual RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const;
};
#endif
