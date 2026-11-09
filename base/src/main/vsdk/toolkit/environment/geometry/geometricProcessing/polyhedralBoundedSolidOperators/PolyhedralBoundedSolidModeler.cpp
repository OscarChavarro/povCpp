#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_PolyhedralBoundedSolidSetOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/slicing/_PolyhedralBoundedSolidSplitter.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

// Weld tolerance for glyph/poly-line simplification, expressed as a
// fraction of the contour bounding-box diagonal.
const double GLYPH_WELD_RELATIVE_FACTOR = 1.0e-2;

class BoundaryRepresentationFromCurveBuildState {
public:
    PolyhedralBoundedSolid* solid;
    bool firstLoop;
    bool beginningOfLoop;
    int nextVertexId;
    int lastLoopStartVertexId;
    int nextFaceId;
    Vector3Dd firstPointInLoop;
    Vector3Dd lastAcceptedPoint;
    bool hasLastAcceptedPoint;
    double weldEpsilon;
    int verticesInCurrentLoop;

    BoundaryRepresentationFromCurveBuildState()
        : solid(new PolyhedralBoundedSolid()), firstLoop(true),
          beginningOfLoop(true), nextVertexId(1), lastLoopStartVertexId(1),
          nextFaceId(1), hasLastAcceptedPoint(false),
          weldEpsilon(PolyhedralBoundedSolidNumericPolicy::BREP_BIG_EPSILON),
          verticesInCurrentLoop(0)
    {
    }
};

double distance(const Vector3Dd& a, const Vector3Dd& b)
{
    return a.subtract(b).length();
}

void findWireSweepEnds(PolyhedralBoundedSolid* solid,
                       _PolyhedralBoundedSolidHalfEdge** outFirst,
                       _PolyhedralBoundedSolidHalfEdge** outLast)
{
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* last;

    first = solid->getPolygonsList().get(0)->boundariesList.get(0)
        ->boundaryStartHalfEdge;
    while ( first->parentEdge != first->next()->parentEdge ) {
        first = first->next();
    }
    last = first->next();
    while ( last->parentEdge != last->next()->parentEdge ) {
        last = last->next();
    }
    *outFirst = first;
    *outLast = last;
}

bool isOnXAxis(const Vector3Dd& p, double tolerance)
{
    return std::fabs(p.y()) <= tolerance && std::fabs(p.z()) <= tolerance;
}

void collapseFaceToAxisVertex(_PolyhedralBoundedSolidFace* face, double x)
{
    if ( face == nullptr ) {
        return;
    }

    int i;
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* he = start;
        if ( he == nullptr ) {
            continue;
        }
        do {
            he->startingVertex->position = Vector3Dd(x, 0.0, 0.0);
            he = he->next();
        } while ( he != nullptr && he != start );
    }
}

bool isBreakMarker(ParametricCurve* curve, int segmentIndex)
{
    return curve->getPointType(segmentIndex) == ParametricCurve::BREAK;
}

java::ArrayList<Vector3Dd> sampleCurveSegment(ParametricCurve* curve,
                                              int segmentIndex)
{
    // Approximate one parametric segment as a polyLine.
    return curve->calculatePoints(segmentIndex, false);
}

void startLoopWithSeedPoint(BoundaryRepresentationFromCurveBuildState& state,
                            const Vector3Dd& point)
{
    state.beginningOfLoop = false;
    if ( state.firstLoop ) {
        // [MANT1988] 12.2: first contour starts with MVFS.
        PolyhedralBoundedSolidEulerOperators::mvfs(state.solid, point,
            state.nextVertexId, state.nextFaceId);
        state.nextVertexId++;
        state.nextFaceId++;
    }
    else {
        // Additional contours are connected and converted into rings.
        PolyhedralBoundedSolidEulerOperators::smev(state.solid, 1,
            state.nextVertexId-1, state.nextVertexId, point);
        state.nextVertexId++;
        PolyhedralBoundedSolidEulerOperators::kemr(state.solid, 1, 1,
            state.nextVertexId-2, state.nextVertexId-1,
            state.nextVertexId-1, state.nextVertexId-2);
        state.lastLoopStartVertexId = state.nextVertexId-1;
    }

    state.firstPointInLoop = point;
    state.lastAcceptedPoint = point;
    state.hasLastAcceptedPoint = true;
    state.verticesInCurrentLoop = 1;
}

bool shouldAcceptPolyLinePoint(
    const BoundaryRepresentationFromCurveBuildState& state,
    const Vector3Dd& point)
{
    return distance(point, state.lastAcceptedPoint) > state.weldEpsilon &&
        distance(point, state.firstPointInLoop) > state.weldEpsilon;
}

void appendPointToCurrentLoop(BoundaryRepresentationFromCurveBuildState& state,
                              const Vector3Dd& point)
{
    PolyhedralBoundedSolidEulerOperators::smev(state.solid, 1,
        state.nextVertexId-1, state.nextVertexId, point);
    state.nextVertexId++;
    state.lastAcceptedPoint = point;
    state.hasLastAcceptedPoint = true;
    state.verticesInCurrentLoop++;
}

void processSampledSegment(BoundaryRepresentationFromCurveBuildState& state,
                           const java::ArrayList<Vector3Dd>& polyline)
{
    int j;
    for ( j = 0; j < polyline.size(); j++ ) {
        Vector3Dd point = polyline.get(j);
        if ( state.beginningOfLoop ) {
            startLoopWithSeedPoint(state, point);
        }
        else if ( shouldAcceptPolyLinePoint(state, point) ) {
            appendPointToCurrentLoop(state, point);
        }
    }
}

void closeLoopWithMef(BoundaryRepresentationFromCurveBuildState& state)
{
    if ( state.verticesInCurrentLoop < 3 ) {
        Logger::reportMessage(java::String(), Logger::WARNING,
            "closeLoopWithMef",
            java::String("Degenerate glyph loop with ") +
            java::String::valueOf(state.verticesInCurrentLoop) +
            " distinct vertices after welding; result may be invalid.");
    }

    // [MANT1988] 12.2: close current wire by creating the face boundary.
    PolyhedralBoundedSolidEulerOperators::mef(state.solid, 1, 1,
        state.lastLoopStartVertexId, state.lastLoopStartVertexId+1,
        state.nextVertexId-1, state.nextVertexId-2, state.nextFaceId);
    state.nextFaceId++;

    if ( !state.firstLoop ) {
        // For inner contours, merge ring into the first face.
        PolyhedralBoundedSolidEulerOperators::kfmrh(state.solid, 2,
            state.nextFaceId-1);
    }
    state.firstLoop = false;
    state.beginningOfLoop = true;
    state.hasLastAcceptedPoint = false;
    state.verticesInCurrentLoop = 0;
}

}

void PolyhedralBoundedSolidModeler::applyTransformation(
    PolyhedralBoundedSolid* solid, const Matrix4x4d& transformation)
{
    if ( solid == nullptr ) {
        Logger::reportMessage("PolyhedralBoundedSolidModeler",
            Logger::WARNING, "applyTransformation",
            "Null solid given, transformation ignored.");
        return;
    }
    int i;
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* vertex =
            solid->getVerticesList().get(i);
        vertex->position = transformation.multiply(vertex->position);
    }
}

void PolyhedralBoundedSolidModeler::addArcToExistingFace(
    PolyhedralBoundedSolid* solid, int faceId, int vertexId, double cx,
    double cy, double radius, double height, double phi1, double phi2, int n)
{
    double x;
    double y;
    double angle;
    double inc;
    int prev;
    int i;
    int nextVertexId;

    angle = phi1 * M_PI / 180.0;
    inc = ((phi2 - phi1) / (n)) * M_PI / 180.0;
    prev = vertexId;
    for ( i = 0; i < n; i++ ) {
        angle += inc;
        // Snap trig values to 1e-10 grid so that equal angular positions on
        // two separately constructed circles always produce bit-identical
        // coordinates, preventing near-coincident vertices downstream.
        // std::floor(v + 0.5) mimics Java's Math.round.
        x = std::floor((cx + radius * std::cos(angle)) * 1.0e10 + 0.5) / 1.0e10;
        y = std::floor((cy + radius * std::sin(angle)) * 1.0e10 + 0.5) / 1.0e10;
        nextVertexId = solid->getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators::smev(solid, faceId, prev,
            nextVertexId, Vector3Dd(x, y, height));
        prev = nextVertexId;
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

PolyhedralBoundedSolid* PolyhedralBoundedSolidModeler::createCircularLamina(
    double cx, double cy, double rad, double h, int n)
{
    PolyhedralBoundedSolid* solid;

    solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(solid,
        Vector3Dd(cx + rad, cy, h), 1, 1);
    addArcToExistingFace(solid, 1, 1, cx, cy, rad, h, 0,
        (n - 1) * 360.0 / n, n-1);
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, n, 1, 2);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

void PolyhedralBoundedSolidModeler::translationalSweepExtrudeFace(
    PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face,
    const Matrix4x4d& transformationMatrix)
{
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* scan;
    _PolyhedralBoundedSolidVertex* v;
    Vector3Dd newPos;
    int i;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        l = face->boundariesList.get(i);
        first = l->boundaryStartHalfEdge;
        scan = first->next();
        v = scan->startingVertex;
        newPos = transformationMatrix.multiply(v->position);
        PolyhedralBoundedSolidEulerOperators::lmev(solid, scan, scan,
            solid->getMaxVertexId()+1, newPos);
        while ( scan != first ) {
            v = scan->next()->startingVertex;
            newPos = transformationMatrix.multiply(v->position);
            PolyhedralBoundedSolidEulerOperators::lmev(solid, scan->next(),
                scan->next(), solid->getMaxVertexId()+1, newPos);
            PolyhedralBoundedSolidEulerOperators::lmef(solid,
                scan->previous(), scan->next()->next(),
                solid->getMaxFaceId()+1);
            scan = (scan->next()->mirrorHalfEdge())->next();
        }
        PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->previous(),
            scan->next()->next(), solid->getMaxFaceId()+1);
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

void PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
    PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face,
    const Matrix4x4d& transformationMatrix)
{
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* scan;
    _PolyhedralBoundedSolidVertex* v;
    Vector3Dd newPos;
    java::ArrayList<int> newFaces;
    int i;
    int newFaceId;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        l = face->boundariesList.get(i);
        first = l->boundaryStartHalfEdge;
        scan = first->next();
        v = scan->startingVertex;
        newPos = transformationMatrix.multiply(v->position);
        PolyhedralBoundedSolidEulerOperators::lmev(solid, scan, scan,
            solid->getMaxVertexId()+1, newPos);
        while ( scan != first ) {
            v = scan->next()->startingVertex;
            newPos = transformationMatrix.multiply(v->position);
            PolyhedralBoundedSolidEulerOperators::lmev(solid, scan->next(),
                scan->next(), solid->getMaxVertexId()+1, newPos);
            newFaceId = solid->getMaxFaceId()+1;
            PolyhedralBoundedSolidEulerOperators::lmef(solid,
                scan->previous(), scan->next()->next(), newFaceId);
            newFaces.add(newFaceId);
            scan = (scan->next()->mirrorHalfEdge())->next();
        }
        newFaceId = solid->getMaxFaceId()+1;
        PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->previous(),
            scan->next()->next(), newFaceId);
        newFaces.add(newFaceId);
    }

    _PolyhedralBoundedSolidFace* newFace;
    for ( i = 0; i < newFaces.size(); i++ ) {
        newFaceId = newFaces.get(i);
        newFace = solid->findFace(newFaceId);
        if ( newFace == nullptr ) {
            Logger::reportMessage("PolyhedralBoundedSolidModeler",
                Logger::WARNING, "translationalSweepExtrudeFacePlanar",
                "Swept face not found, planarity check skipped.");
            continue;
        }
        if ( !PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
                 newFace) ) {
            scan = newFace->boundariesList.get(0)->boundaryStartHalfEdge;
            newFaceId = solid->getMaxFaceId()+1;
            PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->next(),
                scan->previous(), newFaceId);
        }
    }

    newFaces.clear();

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

void PolyhedralBoundedSolidModeler::rotationalSweepExtrudeWireAroundXAxis(
    PolyhedralBoundedSolid* solid, int numberOfFaces)
{
    if ( solid == nullptr || solid->getPolygonsList().size() < 1 ||
         numberOfFaces < 3 ) {
        return;
    }

    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* last;
    findWireSweepEnds(solid, &first, &last);
    _PolyhedralBoundedSolidFace* headf = solid->getPolygonsList().get(0);

    double axisTolerance = VSDK::EPSILON * 100.0;
    Vector3Dd firstEndpointPosition(first->next()->startingVertex->position);
    Vector3Dd lastEndpointPosition(last->startingVertex->position);
    bool firstEndpointOnAxis = isOnXAxis(firstEndpointPosition,
        axisTolerance);
    bool lastEndpointOnAxis = isOnXAxis(lastEndpointPosition,
        axisTolerance);

    _PolyhedralBoundedSolidHalfEdge* cfirst;
    _PolyhedralBoundedSolidHalfEdge* scan = nullptr;
    _PolyhedralBoundedSolidFace* tailf;
    Vector3Dd v;
    Matrix4x4d rotation;

    cfirst = first;
    rotation = rotation.axisRotation((2*M_PI) / numberOfFaces, 1, 0, 0);

    int i;
    for ( i = 0; i < numberOfFaces-1; i++ ) {
        v = rotation.multiply(cfirst->next()->startingVertex->position);
        PolyhedralBoundedSolidEulerOperators::lmev(solid, cfirst->next(),
            cfirst->next(), solid->getMaxVertexId()+1, v);
        scan = cfirst->next();

        while ( scan != last->next() ) {
            v = rotation.multiply(scan->previous()->startingVertex->position);
            PolyhedralBoundedSolidEulerOperators::lmev(solid,
                scan->previous(), scan->previous(),
                solid->getMaxVertexId()+1, v);
            PolyhedralBoundedSolidEulerOperators::lmef(solid,
                scan->previous()->previous(), scan->next(),
                solid->getMaxFaceId()+1);
            scan = (scan->next()->next())->mirrorHalfEdge();
        }
        last = scan;
        cfirst = (cfirst->next()->next())->mirrorHalfEdge();
    }

    tailf = PolyhedralBoundedSolidEulerOperators::lmef(solid, cfirst->next(),
        first->mirrorHalfEdge(), solid->getMaxFaceId()+1);
    while ( cfirst != scan ) {
        PolyhedralBoundedSolidEulerOperators::lmef(solid, cfirst,
            cfirst->next()->next()->next(), solid->getMaxFaceId()+1);
        cfirst = (cfirst->previous())->mirrorHalfEdge()->previous();
    }

    // [MANT1988] 12.2: if a profile endpoint lies on the rotation axis,
    // cap vertices collapse to a single pole instead of leaving a
    // degenerate ring of coincident points.
    if ( firstEndpointOnAxis ) {
        collapseFaceToAxisVertex(headf, firstEndpointPosition.x());
    }
    if ( lastEndpointOnAxis ) {
        collapseFaceToAxisVertex(tailf, lastEndpointPosition.x());
    }
    if ( firstEndpointOnAxis || lastEndpointOnAxis ) {
        PolyhedralBoundedSolidTopologyEditing::maximizeFaces(solid);
    }

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

PolyhedralBoundedSolid*
PolyhedralBoundedSolidModeler::createBrepFromParametricCurve(
    ParametricCurve* curve)
{
    int i;
    BoundaryRepresentationFromCurveBuildState state;
    double* minMax = curve->getMinMax();
    double dx;
    double dy;
    double dz;
    double bboxDiagonal;

    if ( minMax == nullptr ) {
        Logger::reportMessage(java::String(), Logger::WARNING,
            "createBrepFromParametricCurve",
            "Glyph bbox unavailable, falling back to BREP_BIG_EPSILON weld.");
    }
    else {
        dx = minMax[3] - minMax[0];
        dy = minMax[4] - minMax[1];
        dz = minMax[5] - minMax[2];
        bboxDiagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
        if ( std::isfinite(bboxDiagonal) && bboxDiagonal > 0.0 ) {
            state.weldEpsilon = std::fmax(
                PolyhedralBoundedSolidNumericPolicy::BREP_BIG_EPSILON,
                GLYPH_WELD_RELATIVE_FACTOR * bboxDiagonal);
        }
        else {
            Logger::reportMessage(java::String(), Logger::WARNING,
                "createBrepFromParametricCurve",
                "Glyph bbox degenerate, falling back to BREP_BIG_EPSILON weld.");
        }
        delete[] minMax;
    }

    for ( i = 1; i < curve->getPointSize(); i++ ) {
        if ( isBreakMarker(curve, i) ) {
            i++;
            closeLoopWithMef(state);
            continue;
        }
        processSampledSegment(state, sampleCurveSegment(curve, i));
    }

    closeLoopWithMef(state);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(
        state.solid);
    return state.solid;
}

void PolyhedralBoundedSolidModeler::split(
    PolyhedralBoundedSolid* inSolid,
    const InfinitePlane& inSplittingPlane,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow)
{
    _PolyhedralBoundedSolidSplitter::split(inSolid, inSplittingPlane,
                                           outSolidsAbove, outSolidsBelow);
}

PolyhedralBoundedSolid* PolyhedralBoundedSolidModeler::setOp(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    int op,
    bool withDebug,
    bool maximizeResultFaces,
    bool doStrictValidation)
{
    PolyhedralBoundedSolidStatistics::recordSetOpCall(op);
    return _PolyhedralBoundedSolidSetOperator::setOp(inSolidA, inSolidB, op,
        withDebug, maximizeResultFaces, doStrictValidation);
}
