#ifndef ___POLYHEDRAL_BOUNDED_SOLID_OFFSET_CYLINDER_FALLBACK__
#define ___POLYHEDRAL_BOUNDED_SOLID_OFFSET_CYLINDER_FALLBACK__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"

class PolyhedralBoundedSolid;

/**
Structural-shape boolean fallback for offset vertical cylinders: detects two
vertical cylinders in a subtract and rebuilds the difference from analytic
cylinder descriptions.

C++ counterpart of Java's `_PolyhedralBoundedSolidOffsetCylinderFallback`.
*/
class _PolyhedralBoundedSolidOffsetCylinderFallback
    : public _PolyhedralBoundedSolidOperator {
public:
    class VerticalCylinderOperandSpec {
    public:
        double centerX;
        double centerY;
        double zMin;
        double radius;
        double height;
        int radialDivisions;
        int heightDivisions;
    };

    class OffsetCylinderDifferenceFallbackSpec {
    public:
        VerticalCylinderOperandSpec operandA;
        VerticalCylinderOperandSpec operandB;
    };

    /**
    @return a new spec owned by the caller, or null when the case does not
    apply
    */
    static OffsetCylinderDifferenceFallbackSpec*
    prepareOffsetCylinderDifferenceFallbackSpec(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op);

    /**
    @param spec fallback spec, can be null
    @return a new solid owned by the caller, or null
    */
    static PolyhedralBoundedSolid* buildOffsetCylinderDifferenceFallback(
        OffsetCylinderDifferenceFallbackSpec* spec);

private:
    _PolyhedralBoundedSolidOffsetCylinderFallback();
    static bool describeVerticalCylinder(PolyhedralBoundedSolid* solid,
        VerticalCylinderOperandSpec& outSpec);
    static PolyhedralBoundedSolid* createFallbackCylinder(
        const VerticalCylinderOperandSpec& spec,
        int radialDivisions,
        int heightDivisions);
};

#endif
