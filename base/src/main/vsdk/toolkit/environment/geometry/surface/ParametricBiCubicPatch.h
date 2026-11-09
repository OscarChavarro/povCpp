#ifndef __PARAMETRIC_BI_CUBIC_PATCH__
#define __PARAMETRIC_BI_CUBIC_PATCH__


#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/common/memoryManagement/SharedPointer.h"
#include "vsdk/toolkit/environment/geometry/surface/_ParametricBiCubicPatchIntersector.h"
class ParametricCurve;
class Ray;
class RayHit;

class ParametricBiCubicPatch : public Surface {
public:
    Matrix4x4d geometryMatrixX;
    Matrix4x4d geometryMatrixY;
    Matrix4x4d geometryMatrixZ;

    static const int FERGUSON = 7;

    /// Note that the contourCurve must have 4 points with its respective
    /// control parameters.
    ParametricCurve* contourCurve;

private:
    Matrix4x4d sParameterMatrix;
    Matrix4x4d tParameterMatrix;
    Matrix4x4d sDerivativeParameterMatrix;
    Matrix4x4d tDerivativeParameterMatrix;
    Matrix4x4d basisMatrix;
    Matrix4x4d transposedBasisMatrix;
    Matrix4x4d coefficientMatrixX;
    Matrix4x4d coefficientMatrixY;
    Matrix4x4d coefficientMatrixZ;

    Vector3Dd controlMeshPoints[4][4];
    bool hasControlMeshPoints;
    /// C++ port note: the coefficient matrices are values, so this tells
    /// when they were built (Java checks for null matrices)
    bool hasCoefficientMatrices;

    int approximationSteps;

    // Lazily built ray intersection structures, discarded each time the
    // patch matrices are rebuilt. Shared by the copies of the patch, as it
    // is immutable
    mutable SharedPointer<const _ParametricBiCubicPatchIntersector> rayIntersector;

    SharedPointer<const _ParametricBiCubicPatchIntersector> getRayIntersector() const;
    static void fillHitInformation(
        const _ParametricBiCubicPatchIntersector& intersector,
        const _ParametricBiCubicPatchIntersector::PatchHit& hit,
        const Ray& inRay, RayHit* outData);

public:
    int type;

    ParametricBiCubicPatch();

    void buildFergusonPatch(ParametricCurve* curve);
    void buildBezierPatch(const Vector3Dd controlMeshPoints[4][4]);

    /**
    Returns the control point of the Bezier patch equivalent to current
    patch (for Bezier patches, this is the original control mesh point).
    PRE: Some of the "build*Patch" methods should be called before calling
    this method.
    @param i control point index on the s direction, in [0, 3]
    @param j control point index on the t direction, in [0, 3]
    @param outPoint receives the Bezier control point (i, j)
    @return false if the patch has not been built (Java returns null)
    */
    bool getBezierControlPoint(int i, int j, Vector3Dd& outPoint) const;

    int getApproximationSteps() const;
    void setApproximationSteps(int n);

    int getType() const;
    void setType(int type);

    void printGeometryMatrices() const;

    void evaluate(Vector3Dd& p, double s, double t);
    Vector3Dd evaluateTangent(double s, double t);
    Vector3Dd evaluateBinormal(double s, double t);
    Vector3Dd evaluateNormal(double s, double t);

    Ray* doIntersectionFirstHit(const Ray& r);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double intT, RayHit* outData);
    virtual double* getMinMax();

private:
    static const int INITIAL_APPROXIMATION_STEPS = 12;

    void calculateMatrices();
    void buildGeometryMatricesXYZ_Bezier();
    void buildGeometryMatricesXYZ_Hermite();
    void buildGeometryMatricesXYZ_Ferguson();
};

#endif
