#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Arrow.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
const double Arrow::NO_HIT = 1e308;

int Arrow::normalizedSweepSides()
{
    return 36 / 4;
}

double Arrow::snapSweepCoordinate(double value)
{
    return std::round(value * 1.0e10) / 1.0e10;
}

void Arrow::addArcToExistingFace(
    PolyhedralBoundedSolid* solid,
    int faceId,
    int vertexId,
    double cx,
    double cy,
    double radius,
    double height,
    double phi1,
    double phi2,
    int steps)
{
    double angle = phi1 * M_PI / 180.0;
    double inc = ((phi2 - phi1) / static_cast<double>(steps)) * M_PI / 180.0;
    int prev = vertexId;

    for ( int i = 0; i < steps; ++i ) {
        angle += inc;
        double x = snapSweepCoordinate(cx + radius * std::cos(angle));
        double y = snapSweepCoordinate(cy + radius * std::sin(angle));
        int nextVertexId = solid->getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators::smev(
            solid, faceId, prev, nextVertexId, Vector3Dd(x, y, height));
        prev = nextVertexId;
    }
}

PolyhedralBoundedSolid* Arrow::createCircularLamina(
    double cx, double cy, double radius, double height, int sides)
{
    PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(
        solid, Vector3Dd(cx + radius, cy, height), 1, 1);
    addArcToExistingFace(
        solid, 1, 1, cx, cy, radius, height, 0.0,
        (sides - 1) * 360.0 / static_cast<double>(sides), sides - 1);
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, sides, 1, 2);
    return solid;
}

void Arrow::translationalSweepExtrudeFacePlanar(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face,
    const Matrix4x4d& transformationMatrix)
{
    if ( solid == 0 || face == 0 ) {
        return;
    }

    java::ArrayList<int> newFaces;
    for ( long i = 0; i < face->boundariesList.size(); ++i ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            continue;
        }

        _PolyhedralBoundedSolidHalfEdge* first = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* scan = first->next();
        if ( scan == 0 || scan->startingVertex == 0 ) {
            continue;
        }
        Vector3Dd newPos = transformationMatrix.multiply(scan->startingVertex->position);
        PolyhedralBoundedSolidEulerOperators::lmev(
            solid, scan, scan, solid->getMaxVertexId() + 1, newPos);
        while ( scan != first ) {
            _PolyhedralBoundedSolidHalfEdge* scanNext = scan->next();
            if ( scanNext == 0 || scanNext->startingVertex == 0 ) {
                return;
            }

            newPos = transformationMatrix.multiply(scanNext->startingVertex->position);
            PolyhedralBoundedSolidEulerOperators::lmev(
                solid, scanNext, scanNext, solid->getMaxVertexId() + 1, newPos);
            int newFaceId = solid->getMaxFaceId() + 1;
            PolyhedralBoundedSolidEulerOperators::lmef(
                solid, scan->previous(), scan->next()->next(), newFaceId);
            newFaces.add(newFaceId);

            _PolyhedralBoundedSolidHalfEdge* mirror = scan->next()->mirrorHalfEdge();
            if ( mirror == 0 ) {
                return;
            }
            scan = mirror->next();
            if ( scan == 0 ) {
                return;
            }
        }

        int newFaceId = solid->getMaxFaceId() + 1;
        PolyhedralBoundedSolidEulerOperators::lmef(
            solid, scan->previous(), scan->next()->next(), newFaceId);
        newFaces.add(newFaceId);
    }

    for ( long i = 0; i < newFaces.size(); ++i ) {
        int newFaceId = newFaces.get(i);
        _PolyhedralBoundedSolidFace* newFace = solid->findFace(newFaceId);
        if ( newFace == 0 ) {
            continue;
        }
        if ( !PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(newFace) ) {
            _PolyhedralBoundedSolidLoop* loop = newFace->boundariesList.get(0);
            if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
                continue;
            }
            _PolyhedralBoundedSolidHalfEdge* scan = loop->boundaryStartHalfEdge;
            PolyhedralBoundedSolidEulerOperators::lmef(
                solid,
                scan->next(),
                scan->previous(),
                solid->getMaxFaceId() + 1);
        }
    }

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

Arrow::Arrow(double bl, double hl, double br, double hr)
    : baseLength(bl), headLength(hl), baseRadius(br), headRadius(hr) {
    baseCylinder = new Cone(baseRadius, baseRadius, baseLength);
    headCone = new Cone(headRadius, 0, headLength);
    lastElement = baseCylinder;
}

Arrow::~Arrow() {
    delete baseCylinder;
    delete headCone;
}

double Arrow::getBaseLength() const { return baseLength; }
void Arrow::setBaseLength(double val) { baseLength = val; baseCylinder->setHeight(val); }
double Arrow::getHeadLength() const { return headLength; }
void Arrow::setHeadLength(double val) { headLength = val; headCone->setHeight(val); }
double Arrow::getBaseRadius() const { return baseRadius; }
void Arrow::setBaseRadius(double val) { baseRadius = val; baseCylinder->setBottomRadius(val); baseCylinder->setTopRadius(val); }
double Arrow::getHeadRadius() const { return headRadius; }
void Arrow::setHeadRadius(double val) { headRadius = val; headCone->setBottomRadius(val); }

Ray* Arrow::doIntersectionFirstHit(const Ray& inOutRay) {
    Vector3Dd tr(0,0,-baseLength);
    Ray headRay(inOutRay.getOrigin().add(tr), inOutRay.getDirection());
    Ray baseRay(inOutRay);

    Ray* baseHit = baseCylinder->doIntersectionFirstHit(baseRay);
    Ray* headHit = headCone->doIntersectionFirstHit(headRay);

    Ray* result = nullptr;
    if ((baseHit != nullptr && headHit == nullptr) ||
        (baseHit != nullptr && headHit != nullptr && baseHit->getT() < headHit->getT())) {
        lastElement = baseCylinder;
        result = new Ray(inOutRay.withT(baseHit->getT()));
    }
    else if ((baseHit == nullptr && headHit != nullptr) ||
             (baseHit != nullptr && headHit != nullptr && headHit->getT() < baseHit->getT())) {
        lastElement = headCone;
        result = new Ray(inOutRay.withT(headHit->getT()));
    }

    if (baseHit) delete baseHit;
    if (headHit) delete headHit;
    return result;
}

bool Arrow::doIntersectionDistanceOnly(const Ray& inRay, RayHit* outHit) {
    Vector3Dd shiftedHeadOrigin(inRay.getOrigin().x(), inRay.getOrigin().y(), inRay.getOrigin().z() - baseLength);
    Ray shiftedHeadRay(shiftedHeadOrigin, inRay.getDirection(), inRay.getT());

    RayHit localHit(RayHit::DETAIL_NONE, false);
    RayHit* candidateHit = (outHit != nullptr) ? outHit : &localHit;
    bool shouldStoreRay = (outHit != nullptr) ? outHit->shouldStoreRay() : false;
    candidateHit->setStoreRay(false);

    double baseT = NO_HIT;
    candidateHit->resetForDistanceOnly();
    if (baseCylinder->doIntersectionFirstHit(inRay, candidateHit)) baseT = candidateHit->getHitDistance();

    double headT = NO_HIT;
    candidateHit->resetForDistanceOnly();
    if (headCone->doIntersectionFirstHit(shiftedHeadRay, candidateHit)) headT = candidateHit->getHitDistance();

    double winnerT = (baseT < headT) ? baseT : headT;
    if (winnerT == NO_HIT) return false;

    if (outHit != nullptr) {
        if (shouldStoreRay) outHit->setRay(inRay.withT(winnerT));
        else outHit->setHitDistance(winnerT);
        outHit->setStoreRay(shouldStoreRay);
    }
    return true;
}

bool Arrow::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    if (outHit == nullptr || !outHit->needsAnySurfaceData()) {
        return doIntersectionDistanceOnly(inRay, outHit);
    }

    Vector3Dd tr(0,0,-baseLength);
    Ray shiftedHeadRay(inRay.getOrigin().add(tr), inRay.getDirection(), inRay.getT());

    RayHit baseHit(outHit->getRequiredDetailMask());
    RayHit headHit(outHit->getRequiredDetailMask());
    bool hasBase = baseCylinder->doIntersectionFirstHit(inRay, &baseHit);
    bool hasHead = headCone->doIntersectionFirstHit(shiftedHeadRay, &headHit);
    if (!hasBase && !hasHead) return false;

    double baseT = hasBase ? (baseHit.getRay()!=nullptr ? baseHit.getRay()->getT() : baseHit.getHitDistance()) : NO_HIT;
    double headT = hasHead ? (headHit.getRay()!=nullptr ? headHit.getRay()->getT() : headHit.getHitDistance()) : NO_HIT;

    if (hasBase && (!hasHead || baseT < headT)) {
        outHit->clone(baseHit);
        outHit->setRay(inRay.withT(baseT));
    }
    else {
        outHit->clone(headHit);
        outHit->setRay(inRay.withT(headT));
        if (outHit->needsPoint()) {
            outHit->point = Vector3Dd(outHit->point.x(), outHit->point.y(), outHit->point.z() + baseLength);
        }
    }
    return true;
}

void Arrow::doExtraInformation(const Ray& inRay, double inT, RayHit* outData) {
    if (outData == nullptr) return;
    RayHit hit;
    if (doIntersectionFirstHit(inRay.withT(inT), &hit)) outData->clone(hit);
}

double* Arrow::getMinMax() {
    double* m = new double[6];
    double r = baseRadius > headRadius ? baseRadius : headRadius;
    m[0]=-r; m[1]=-r; m[2]=0; m[3]=r; m[4]=r; m[5]=baseLength+headLength;
    return m;
}

PolyhedralBoundedSolid* Arrow::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(
        DEFAULT_CIRCUMFERENCE_DIVISIONS, DEFAULT_HEIGHT_DIVISIONS);
}

PolyhedralBoundedSolid* Arrow::exportToPolyhedralBoundedSolid(
    int circumferenceDivisions, int heightDivisions)
{
    int normalizedCircumferenceDivisions =
        circumferenceDivisions < MIN_CIRCUMFERENCE_DIVISIONS ?
        MIN_CIRCUMFERENCE_DIVISIONS : circumferenceDivisions;
    int normalizedHeightDivisions =
        heightDivisions < MIN_HEIGHT_DIVISIONS ?
        MIN_HEIGHT_DIVISIONS : heightDivisions;

    if ( normalizedCircumferenceDivisions ==
             DEFAULT_CIRCUMFERENCE_DIVISIONS &&
         normalizedHeightDivisions == DEFAULT_HEIGHT_DIVISIONS ) {
        return exportToPolyhedralBoundedSolid();
    }

    return buildPolyhedralBoundedSolid(
        normalizedCircumferenceDivisions, normalizedHeightDivisions);
}

void Arrow::closeTopFaceToApex(
    PolyhedralBoundedSolid* solid, int nsides, double apexZ)
{
    int base1 = 2 * nsides + 1;
    int base2 = 3 * nsides + 1;
    Vector3Dd apex(0, 0, apexZ);
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, base1, base2, apex);

    int i = 0;
    for ( i = 0; i < nsides - 2; ++i ) {
        PolyhedralBoundedSolidEulerOperators::mef(
            solid, 1, 1, base2, base1 + i, base1 + i + 1, base1 + i + 2, base2 + i + 1);
    }

    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, base2, base1 + i, base1 + i + 1, base1, base2 + i + 1);
}

PolyhedralBoundedSolid* Arrow::buildPolyhedralBoundedSolid(
    int nsides, int heightDivisions)
{
    PolyhedralBoundedSolid* solid =
        createCircularLamina(0.0, 0.0, baseRadius, 0.0, nsides);

    Matrix4x4d transform;
    double cylinderZStep = baseLength / static_cast<double>(heightDivisions);
    for ( int i = 0; i < heightDivisions; ++i ) {
        transform = transform.translation(0.0, 0.0, cylinderZStep);
        translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), transform);
    }

    double scaleFactor = headRadius / baseRadius;
    Matrix4x4d scaleTransform;
    scaleTransform = scaleTransform.scale(scaleFactor, scaleFactor, 1.0);
    translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), scaleTransform);

    double prevRadius = headRadius;
    double coneZStep = headLength / static_cast<double>(heightDivisions);
    for ( int i = 1; i < heightDivisions; ++i ) {
        double nextRadius = headRadius *
            (1.0 - (static_cast<double>(i) / static_cast<double>(heightDivisions)));
        double coneScale = std::abs(prevRadius) > VSDK::EPSILON ?
            nextRadius / prevRadius : 0.0;
        transform = transform.translation(0.0, 0.0, coneZStep);
        scaleTransform = scaleTransform.scale(coneScale, coneScale, 1.0);
        translationalSweepExtrudeFacePlanar(
            solid, solid->findFace(1), transform.multiply(scaleTransform));
        prevRadius = nextRadius;
    }

    closeTopFaceToApex(solid, nsides, baseLength + headLength);

    return solid;
}
