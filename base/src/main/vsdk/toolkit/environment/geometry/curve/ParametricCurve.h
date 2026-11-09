#ifndef __PARAMETRIC_CURVE__
#define __PARAMETRIC_CURVE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/curve/Curve.h"
class ParametricCurve : public Curve {
public:
    static const int BREAK = 1;
    static const int CORNER = 2;
    static const int QUAD = 3;
    static const int HERMITE = 4;
    static const int BEZIER = 5;
    static const int UNRBSPLINE = 6;
    static const int NUNRBSPLINE = 7;
    static const int CATMULLROM = 8;

    static const Matrix4x4d LINEAR_MATRIX;
    static const Matrix4x4d HERMITE_MATRIX;
    static const Matrix4x4d BEZIER_MATRIX;
    static const Matrix4x4d UNRBSPLINE_MATRIX;
    static const Matrix4x4d CATMULL_ROM_MATRIX;

    ParametricCurve();
    virtual ~ParametricCurve() override {}

    int getApproximationSteps() const;
    void setApproximationSteps(int n);

    void addPoint(const java::ArrayList<Vector3Dd>& point, int type);
    void addPointAt(const java::ArrayList<Vector3Dd>& point, int type, int position);
    const java::ArrayList<Vector3Dd>& getPointVector(int pos) const;
    virtual const Vector3Dd* getPoint(int idx) const;
    int getPointSize() const;
    /**
    Returns the segment type (`BREAK`, `CORNER`, ...) of a control point
    (Java code accesses the public `types` list directly).
    @param pos index of the control point
    @return type associated with the control point
    */
    int getPointType(int pos) const;
    void removePoint(int pos);
    void setPointAt(const java::ArrayList<Vector3Dd>& p, int pos);

    int getNumPieces() const;

    bool evaluate(int endingSegment, double t, Vector3Dd& outPoint) const;
    java::ArrayList<Vector3Dd> calculatePoints(int endingPointForSegment, bool withBrokenRects) const;

    virtual double* getMinMax() override;
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;

private:
    static const int INITIAL_APPROXIMATION_STEPS = 50;

    java::ArrayList< java::ArrayList<Vector3Dd> > points;
    java::ArrayList<int> types;
    int approximationSteps;

    bool evaluateLinear(int nseg, double t, Vector3Dd& outPoint) const;
    bool evaluateQuadratic(int nseg, double t, Vector3Dd& outPoint) const;
    bool evaluateHermite(int nseg, double t, Vector3Dd& outPoint) const;
    bool evaluateBezier(int nseg, double t, Vector3Dd& outPoint) const;
    bool evaluateBspline(int nseg, double t, Vector3Dd& outPoint) const;

    int calculatePointPosition(int pin) const;
    static double matrixTerm(const Matrix4x4d& m, int col, double t);
};

#endif
