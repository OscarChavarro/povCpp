#include "java/lang/Double.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
static Matrix4x4d buildLinearMatrix()
{
    const double m[4][4] = {
        { 0.0,  0.0,  0.0,  1.0},
        { 0.0,  0.0,  1.0,  1.0},
        {-1.0,  1.0,  0.0,  0.0},
        { 1.0,  0.0,  0.0,  0.0}
    };
    return Matrix4x4d::copyOf(m);
}

static Matrix4x4d buildHermiteMatrix()
{
    const double m[4][4] = {
        { 2.0, -2.0,  1.0,  1.0},
        {-3.0,  3.0, -2.0, -1.0},
        { 0.0,  0.0,  1.0,  0.0},
        { 1.0,  0.0,  0.0,  0.0}
    };
    return Matrix4x4d::copyOf(m);
}

static Matrix4x4d buildBezierMatrix()
{
    const double m[4][4] = {
        {-1.0,  3.0, -3.0, 1.0},
        { 3.0, -6.0,  3.0, 0.0},
        {-3.0,  3.0,  0.0, 0.0},
        { 1.0,  0.0,  0.0, 0.0}
    };
    return Matrix4x4d::copyOf(m);
}

static Matrix4x4d buildUnrbSplineMatrix()
{
    const double m[4][4] = {
        {-1.0,  3.0, -3.0, 1.0},
        { 3.0, -6.0,  3.0, 0.0},
        {-3.0,  0.0,  3.0, 0.0},
        { 1.0,  4.0,  1.0, 0.0}
    };
    return Matrix4x4d::copyOf(m);
}

static Matrix4x4d buildCatmullRomMatrix()
{
    const double m[4][4] = {
        {-0.5,  1.5, -1.5,  0.5},
        { 1.0, -2.5,  2.0, -0.5},
        {-0.5,  0.0,  0.5,  0.0},
        { 0.0,  1.0,  0.0,  0.0}
    };
    return Matrix4x4d::copyOf(m);
}

// Out-of-class definitions, needed when the constants are bound to
// references (e.g. by gtest assertions) under C++11
const int ParametricCurve::BREAK;
const int ParametricCurve::CORNER;
const int ParametricCurve::QUAD;
const int ParametricCurve::HERMITE;
const int ParametricCurve::BEZIER;
const int ParametricCurve::UNRBSPLINE;
const int ParametricCurve::NUNRBSPLINE;
const int ParametricCurve::CATMULLROM;

const Matrix4x4d ParametricCurve::LINEAR_MATRIX = buildLinearMatrix();
const Matrix4x4d ParametricCurve::HERMITE_MATRIX = buildHermiteMatrix();
const Matrix4x4d ParametricCurve::BEZIER_MATRIX = buildBezierMatrix();
const Matrix4x4d ParametricCurve::UNRBSPLINE_MATRIX = buildUnrbSplineMatrix();
const Matrix4x4d ParametricCurve::CATMULL_ROM_MATRIX = buildCatmullRomMatrix();

ParametricCurve::ParametricCurve()
    : approximationSteps(INITIAL_APPROXIMATION_STEPS)
{
}

int ParametricCurve::getApproximationSteps() const
{
    return approximationSteps;
}

void ParametricCurve::setApproximationSteps(int n)
{
    approximationSteps = n;
}

int ParametricCurve::getPointType(int pos) const
{
    return types.get(pos);
}

void ParametricCurve::addPoint(const java::ArrayList<Vector3Dd>& point, int type)
{
    if ( type == BREAK && points.size() == 0 ) {
        return;
    }
    points.add(point);
    types.add(type);
}

void ParametricCurve::addPointAt(const java::ArrayList<Vector3Dd>& point, int type, int position)
{
    if ( type == BREAK && position < 1 ) {
        return;
    }
    points.add((long int)position, point);
    types.add((long int)position, type);
}

const java::ArrayList<Vector3Dd>& ParametricCurve::getPointVector(int pos) const
{
    return const_cast<java::ArrayList< java::ArrayList<Vector3Dd> >&>(points)[(long int)pos];
}

const Vector3Dd* ParametricCurve::getPoint(int idx) const
{
    if ( idx < 0 || idx >= (int)points.size() ) {
        return nullptr;
    }
    java::ArrayList<Vector3Dd>& v = const_cast<java::ArrayList< java::ArrayList<Vector3Dd> >&>(points)[(long int)idx];
    if ( v.size() == 0 ) {
        return nullptr;
    }
    return &v[0];
}

int ParametricCurve::getPointSize() const
{
    return (int)points.size();
}

void ParametricCurve::removePoint(int pos)
{
    points.remove((long int)pos);
    types.remove((long int)pos);
}

void ParametricCurve::setPointAt(const java::ArrayList<Vector3Dd>& p, int pos)
{
    points[(long int)pos] = p;
}

int ParametricCurve::getNumPieces() const
{
    int sum = 1;
    for (long int i = 1; i < types.size(); ++i) {
        if ( types.get(i) == BREAK ) {
            sum++;
        }
    }
    return sum;
}

double ParametricCurve::matrixTerm(const Matrix4x4d& m, int col, double t)
{
    double vt = 0.0;
    for ( int i = 0; i < 4; ++i ) {
        vt += m.get(i, col) * std::pow(t, 3 - i);
    }
    return vt;
}

bool ParametricCurve::evaluate(int endingSegment, double t, Vector3Dd& outPoint) const
{
    if ( endingSegment < 0 || endingSegment >= (int)types.size() ) {
        return false;
    }

    if ( types.get(endingSegment) == CORNER ) return evaluateLinear(endingSegment, t, outPoint);
    if ( types.get(endingSegment) == QUAD ) return evaluateQuadratic(endingSegment, t, outPoint);
    if ( types.get(endingSegment) == HERMITE ) return evaluateHermite(endingSegment, t, outPoint);
    if ( types.get(endingSegment) == BEZIER ) return evaluateBezier(endingSegment, t, outPoint);
    if ( types.get(endingSegment) == UNRBSPLINE ) return evaluateBspline(endingSegment, t, outPoint);

    return false;
}

bool ParametricCurve::evaluateLinear(int nseg, double t, Vector3Dd& outPoint) const
{
    if ( nseg <= 0 || nseg >= (int)points.size()
        || points.get(nseg - 1).size() == 0
        || points.get(nseg).size() == 0 ) {
        return false;
    }

    Vector3Dd p1 = points.get(nseg - 1).get(0);
    Vector3Dd p2 = points.get(nseg).get(0);

    Vector3Dd result = p1.multiply(matrixTerm(LINEAR_MATRIX, 0, t));
    result = result.add(p2.multiply(matrixTerm(LINEAR_MATRIX, 1, t)));
    outPoint = result;
    return true;
}

bool ParametricCurve::evaluateQuadratic(int nseg, double t, Vector3Dd& outPoint) const
{
    if ( nseg <= 0 || nseg >= (int)points.size()
        || points.get(nseg - 1).size() == 0
        || points.get(nseg).size() < 2 ) {
        return false;
    }

    Vector3Dd qp0 = points.get(nseg - 1).get(0);
    Vector3Dd qp1 = points.get(nseg).get(1);
    Vector3Dd qp2 = points.get(nseg).get(0);

    Vector3Dd result = qp0.multiply(matrixTerm(BEZIER_MATRIX, 0, t));
    Vector3Dd p1 = qp0.add(qp1.subtract(qp0).multiply(2.0 / 3.0));
    result = result.add(p1.multiply(matrixTerm(BEZIER_MATRIX, 1, t)));

    Vector3Dd p2 = qp1.add(qp2.subtract(qp0).multiply(1.0 / 3.0));
    result = result.add(p2.multiply(matrixTerm(BEZIER_MATRIX, 2, t)));
    result = result.add(qp2.multiply(matrixTerm(BEZIER_MATRIX, 3, t)));

    outPoint = result;
    return true;
}

bool ParametricCurve::evaluateHermite(int nseg, double t, Vector3Dd& outPoint) const
{
    if ( nseg <= 0 || nseg >= (int)points.size()
        || points.get(nseg - 1).size() < 3
        || points.get(nseg).size() < 2 ) {
        return false;
    }

    java::ArrayList<Vector3Dd> start = points.get(nseg - 1);
    java::ArrayList<Vector3Dd> end = points.get(nseg);

    Vector3Dd result = start.get(0).multiply(matrixTerm(HERMITE_MATRIX, 0, t));
    result = result.add(end.get(0).multiply(matrixTerm(HERMITE_MATRIX, 1, t)));
    result = result.add(start.get(2).multiply(matrixTerm(HERMITE_MATRIX, 2, t)));
    result = result.add(end.get(1).multiply(matrixTerm(HERMITE_MATRIX, 3, t)));

    outPoint = result;
    return true;
}

bool ParametricCurve::evaluateBezier(int nseg, double t, Vector3Dd& outPoint) const
{
    if ( nseg <= 0 || nseg >= (int)points.size()
        || points.get(nseg - 1).size() < 3
        || points.get(nseg).size() < 2 ) {
        return false;
    }

    java::ArrayList<Vector3Dd> start = points.get(nseg - 1);
    java::ArrayList<Vector3Dd> end = points.get(nseg);

    Vector3Dd result = start.get(0).multiply(matrixTerm(BEZIER_MATRIX, 0, t));
    result = result.add(start.get(2).multiply(matrixTerm(BEZIER_MATRIX, 1, t)));
    result = result.add(end.get(1).multiply(matrixTerm(BEZIER_MATRIX, 2, t)));
    result = result.add(end.get(0).multiply(matrixTerm(BEZIER_MATRIX, 3, t)));

    outPoint = result;
    return true;
}

bool ParametricCurve::evaluateBspline(int nseg, double t, Vector3Dd& outPoint) const
{
    if ( points.size() < 4 ) {
        return false;
    }

    Vector3Dd result(0.0, 0.0, 0.0);
    for ( int np = 0; np < 4; ++np ) {
        int idx = nseg - np;
        if ( idx < 0 || idx >= (int)points.size() || points.get(idx).size() == 0 ) {
            return false;
        }
        double vt = matrixTerm(UNRBSPLINE_MATRIX, np, t);
        result = result.add(points.get(idx).get(0).multiply(vt / 6.0));
    }

    outPoint = result;
    return true;
}

int ParametricCurve::calculatePointPosition(int pin) const
{
    int pout = 0;
    for ( int i = 0; i < (int)types.size() && i < pin; ++i ) {
        if ( types.get(i) == BREAK ) {
            pout = -1;
        }
        else {
            pout++;
        }
    }
    return pout;
}

java::ArrayList<Vector3Dd> ParametricCurve::calculatePoints(int endingPointForSegment, bool withBrokenRects) const
{
    java::ArrayList<Vector3Dd> pol;

    if ( endingPointForSegment <= 0 || endingPointForSegment >= (int)types.size() ) {
        return pol;
    }

    int relativePoint = calculatePointPosition(endingPointForSegment);
    int t = types.get(endingPointForSegment);

    if ( ((relativePoint <= 2) && t == UNRBSPLINE) || relativePoint < 0 ) {
        return pol;
    }

    if ( ((t == CORNER) && !withBrokenRects) ||
         relativePoint <= 0 ||
         (t == UNRBSPLINE && relativePoint < 3) ) {
        if ( points.get(endingPointForSegment - 1).size() != 0 ) pol.add(points.get(endingPointForSegment - 1).get(0));
        if ( points.get(endingPointForSegment).size() != 0 ) pol.add(points.get(endingPointForSegment).get(0));
        return pol;
    }

    Vector3Dd q;
    if ( !evaluate(endingPointForSegment, 0.0, q) ) {
        return pol;
    }
    pol.add(q);

    for ( int i = 1; i <= approximationSteps; ++i ) {
        if ( evaluate(endingPointForSegment, (double)i / (double)approximationSteps, q) ) {
            pol.add(q);
        }
    }

    return pol;
}

double* ParametricCurve::getMinMax()
{
    double* minmax = new double[6];
    minmax[0] = minmax[1] = minmax[2] = java::Double::MAX_VALUE;
    minmax[3] = minmax[4] = minmax[5] = (-java::Double::MAX_VALUE);

    for ( int i = 1; i < (int)types.size(); ++i ) {
        java::ArrayList<Vector3Dd> polyline = calculatePoints(i, false);
        for ( long int j = 0; j < polyline.size(); ++j ) {
            Vector3Dd p = polyline.get(j);
            if ( p.x() < minmax[0] ) minmax[0] = p.x();
            if ( p.y() < minmax[1] ) minmax[1] = p.y();
            if ( p.z() < minmax[2] ) minmax[2] = p.z();
            if ( p.x() > minmax[3] ) minmax[3] = p.x();
            if ( p.y() > minmax[4] ) minmax[4] = p.y();
            if ( p.z() > minmax[5] ) minmax[5] = p.z();
        }
    }

    return minmax;
}

int ParametricCurve::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    for ( int i = 1; i < (int)types.size(); ++i ) {
        if ( types[(long int)i] == BREAK ) {
            i++;
            continue;
        }

        java::ArrayList<Vector3Dd> polyline = calculatePoints(i, false);
        for ( long int j = 0; j < polyline.size(); ++j ) {
            if ( polyline[(long int)j].subtract(p).length() < distanceTolerance ) {
                return LIMIT;
            }
        }
    }

    return OUTSIDE;
}
