#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
Cone::Cone(double bottomRadius, double topRadius, double height) :
    bottomRadius(bottomRadius), topRadius(topRadius), height(height) {
    addControlSpecification("double;bottomRadius;(0, INFINITE)",
        &Cone::getBottomRadius, &Cone::setBottomRadius);
    addControlSpecification("double;topRadius;[0, INFINITE)",
        &Cone::getTopRadius, &Cone::setTopRadius);
    addControlSpecification("double;height;(0, INFINITE)",
        &Cone::getHeight, &Cone::setHeight);
}

double Cone::sq(double v){ return v*v; }
bool Cone::approxEq(double a, double b){ return std::abs(a-b) <= VSDK::EPSILON; }

double Cone::getBottomRadius() const { return bottomRadius; }
double Cone::getTopRadius() const { return topRadius; }
double Cone::getHeight() const { return height; }
void Cone::setBottomRadius(double value) { bottomRadius = value; }
void Cone::setTopRadius(double value) { topRadius = value; }
void Cone::setHeight(double value) { height = value; }

Ray* Cone::doIntersectionCylinder(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    double A = sq(dx)+sq(dy); if (std::abs(A) <= VSDK::EPSILON) return nullptr;
    double B = 2*((dx*ox)+(dy*oy));
    double C = sq(ox)+sq(oy)-sq(inR);
    double disc = sq(B)-4*A*C; if (disc <= VSDK::EPSILON) return nullptr;
    double t0 = (-B-std::sqrt(disc))/(2*A);
    if (t0 > VSDK::EPSILON) {
        double pz = oz + dz*t0;
        if (pz > inH || pz < 0) return nullptr;
        if (outInfo != nullptr) {
            double px=ox+dx*t0, py=oy+dy*t0;
            outInfo->point = Vector3Dd(px,py,pz);
            outInfo->normal = Vector3Dd(px,py,0).normalized();
        }
        Ray hRay = inOutRay.withT(t0);
        return new Ray(hRay);
    }
    return nullptr;
}

Ray* Cone::doIntersectionCone(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    if (inH <= VSDK::EPSILON) return nullptr;
    double shiftedOz = oz - inH;
    double ratio = inR / inH;
    double ratioSquared = sq(ratio);
    double A = sq(dx)+sq(dy)-sq(dz*ratio); if (std::abs(A)<=VSDK::EPSILON) return nullptr;
    double B = 2*((dx*ox)+(dy*oy)-(dz*shiftedOz*ratioSquared));
    double C = sq(ox)+sq(oy)-sq(shiftedOz*ratio);
    double disc = sq(B)-4*A*C; if (disc <= VSDK::EPSILON) return nullptr;
    double t0 = (-B-std::sqrt(disc))/(2*A);
    if (t0 > VSDK::EPSILON) {
        double shiftedPz = shiftedOz + dz*t0;
        if (shiftedPz > 0 || shiftedPz < -inH) return nullptr;
        if (outInfo != nullptr) {
            double px=ox+dx*t0, py=oy+dy*t0;
            outInfo->point = Vector3Dd(px, py, shiftedPz + inH);
            outInfo->normal = Vector3Dd(px, py, -shiftedPz * ratioSquared).normalized();
        }
        Ray hRay = inOutRay.withT(t0);
        return new Ray(hRay);
    }
    return nullptr;
}

Ray* Cone::doIntersectionTap(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    if (std::abs(dz) > VSDK::EPSILON) {
        double t=(inH-oz)/dz;
        if (t > VSDK::EPSILON) {
            double px=ox+dx*t, py=oy+dy*t;
            if (sq(px)+sq(py) < sq(inR)) {
                if (outInfo != nullptr) {
                    outInfo->normal = Vector3Dd(0,0,1);
                    outInfo->point = Vector3Dd(px,py,inH);
                }
                Ray hRay = inOutRay.withT(t);
                return new Ray(hRay);
            }
        }
    }
    return nullptr;
}

Ray* Cone::doIntersectionFirstHit(const Ray& inOutRay) {
    RayHit hit;
    if (doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != nullptr) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool Cone::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit) {
    RayHit infoTap1, infoTap2, infoBody;
    Ray* bodyHit = nullptr;
    Ray* tap1Hit = nullptr;
    Ray* tap2Hit = nullptr;
    Ray* winner = nullptr;
    RayHit* winnerInfo = nullptr;

    if (topRadius < VSDK::EPSILON && bottomRadius > VSDK::EPSILON) {
        bodyHit = doIntersectionCone(inOutRay, bottomRadius, height, &infoBody);
        tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, &infoTap1);
        if ((tap1Hit != nullptr && bodyHit == nullptr) ||
            (tap1Hit != nullptr && bodyHit != nullptr && tap1Hit->getT() < bodyHit->getT())) {
            infoTap1.normal = infoTap1.normal.multiply(-1);
            winner = new Ray(inOutRay.withT(tap1Hit->getT()));
            winnerInfo = &infoTap1;
        }
        else if (bodyHit != nullptr) {
            winner = new Ray(inOutRay.withT(bodyHit->getT()));
            winnerInfo = &infoBody;
        }
    }
    else if (approxEq(bottomRadius, topRadius)) {
        int nearest = -1;
        bodyHit = doIntersectionCylinder(inOutRay, bottomRadius, height, &infoBody);
        tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, &infoTap1);
        tap2Hit = doIntersectionTap(inOutRay, bottomRadius, height, &infoTap2);

        if (bodyHit != nullptr &&
            ((tap1Hit != nullptr && bodyHit->getT() < tap1Hit->getT()) || tap1Hit == nullptr) &&
            ((tap2Hit != nullptr && bodyHit->getT() < tap2Hit->getT()) || tap2Hit == nullptr)) nearest = 1;
        else if (tap1Hit != nullptr &&
            ((bodyHit != nullptr && tap1Hit->getT() < bodyHit->getT()) || bodyHit == nullptr) &&
            ((tap2Hit != nullptr && tap1Hit->getT() < tap2Hit->getT()) || tap2Hit == nullptr)) nearest = 3;
        else if (tap2Hit != nullptr) nearest = 2;

        if (nearest == 1) { winner = new Ray(inOutRay.withT(bodyHit->getT())); winnerInfo = &infoBody; }
        else if (nearest == 2) { winner = new Ray(inOutRay.withT(tap2Hit->getT())); winnerInfo = &infoTap2; }
        else if (nearest == 3) { winner = new Ray(inOutRay.withT(tap1Hit->getT())); winnerInfo = &infoTap1; }
    }

    if (bodyHit) delete bodyHit;
    if (tap1Hit) delete tap1Hit;
    if (tap2Hit) delete tap2Hit;

    if (winner == nullptr) return false;

    if (outHit != nullptr) {
        outHit->setRay(*winner);
        outHit->point = Vector3Dd(winnerInfo->point);
        outHit->normal = Vector3Dd(winnerInfo->normal).normalized();
        outHit->tangent = Vector3Dd(winnerInfo->tangent);
        outHit->u = winnerInfo->u;
        outHit->v = winnerInfo->v;
        outHit->material = winnerInfo->material;
        outHit->texture = winnerInfo->texture;
        outHit->normalMap = winnerInfo->normalMap;
    }
    delete winner;
    return true;
}

void Cone::doExtraInformation(const Ray& inRay, double, RayHit* outData) {
    if (outData == nullptr) return;
    RayHit hit;
    if (doIntersectionFirstHit(inRay.withT(1e308), &hit)) {
        outData->clone(hit);
    }
}

/**
Check the general interface contract in superclass method
Geometry.doContainmentTest. The cone (or truncated cone, or cylinder)
has its axis along Z, its base of radius `bottomRadius` at z = 0 and its
top of radius `topRadius` at z = `height`. The distance to the side is
measured perpendicular to the side, so the tolerance is the same for the
side and for the caps.
@param p point to classify, in the space of the cone
@param distanceTolerance distance to the surface of the cone under which
the point is classified as `LIMIT`
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int Cone::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    if ( height <= 0 ) {
        return OUTSIDE;
    }
    double z = p.z();
    double radialDistance = std::sqrt(p.x() * p.x() + p.y() * p.y());
    double radiusAtZ = bottomRadius + (topRadius - bottomRadius) * (z / height);
    double radiusChange = bottomRadius - topRadius;
    double sideCosine = height / std::sqrt(height * height + radiusChange * radiusChange);
    double sideDistance = (radialDistance - radiusAtZ) * sideCosine;

    if ( z < -distanceTolerance || z > height + distanceTolerance ||
         sideDistance > distanceTolerance ) {
        return OUTSIDE;
    }
    if ( z > distanceTolerance && z < height - distanceTolerance &&
         sideDistance < -distanceTolerance ) {
        return INSIDE;
    }
    return LIMIT;
}

double* Cone::getMinMax() {
    double* m = new double[6];
    double r = (bottomRadius > topRadius) ? bottomRadius : topRadius;
    m[0]=-r; m[1]=-r; m[2]=0; m[3]=r; m[4]=r; m[5]=height;
    return m;
}

PolyhedralBoundedSolid* Cone::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(
        DEFAULT_CIRCUMFERENCE_DIVISIONS, DEFAULT_HEIGHT_DIVISIONS);
}

PolyhedralBoundedSolid* Cone::exportToPolyhedralBoundedSolid(
    int circumferenceDivisions, int heightDivisions)
{
    int normalizedCircumferenceDivisions =
        circumferenceDivisions > MIN_CIRCUMFERENCE_DIVISIONS ?
        circumferenceDivisions : MIN_CIRCUMFERENCE_DIVISIONS;
    int normalizedHeightDivisions =
        heightDivisions > MIN_HEIGHT_DIVISIONS ?
        heightDivisions : MIN_HEIGHT_DIVISIONS;

    if ( normalizedCircumferenceDivisions == DEFAULT_CIRCUMFERENCE_DIVISIONS &&
         normalizedHeightDivisions == DEFAULT_HEIGHT_DIVISIONS ) {
        return exportToPolyhedralBoundedSolid();
    }

    return buildPolyhedralBoundedSolid(normalizedCircumferenceDivisions,
        normalizedHeightDivisions);
}

/**
Current implementation of the cylinder follows the idea suggested on
section [MANT1988].12.3.1 and program [MANT1988].12.4, where the
cylinder is built upon a circular lamina base and an extrusion
(translational sweep) operation. The cone case is done manually,
*/
void Cone::closeTopFaceToApex(PolyhedralBoundedSolid* solid, double apexZ)
{
    _PolyhedralBoundedSolidFace* topFace = solid->findFace(1);
    if ( topFace == 0 || topFace->boundariesList.size() <= 0 ) {
        return;
    }

    _PolyhedralBoundedSolidLoop* loop = topFace->boundariesList.get(0);
    _PolyhedralBoundedSolidHalfEdge* start = loop->boundaryStartHalfEdge;
    if ( start == 0 ) {
        return;
    }

    java::ArrayList<int> ringVertexIds;
    _PolyhedralBoundedSolidHalfEdge* he = start;
    do {
        ringVertexIds.add(he->startingVertex->id);
        he = he->next();
    } while ( he != start );

    if ( ringVertexIds.size() < 3 ) {
        return;
    }

    int apexVertexId = solid->getMaxVertexId() + 1;
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, ringVertexIds.get(0),
        apexVertexId, Vector3Dd(0.0, 0.0, apexZ));

    long int i;
    for ( i = 0; i < ringVertexIds.size() - 2; i++ ) {
        PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
            apexVertexId,
            ringVertexIds.get(i),
            ringVertexIds.get(i+1),
            ringVertexIds.get(i+2),
            solid->getMaxFaceId() + 1);
    }

    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
        apexVertexId,
        ringVertexIds.get(ringVertexIds.size()-2),
        ringVertexIds.get(ringVertexIds.size()-1),
        ringVertexIds.get(0),
        solid->getMaxFaceId() + 1);
}

PolyhedralBoundedSolid* Cone::buildPolyhedralBoundedSolid(int nsides,
    int heightDivisions)
{
    PolyhedralBoundedSolid* solid;
    Matrix4x4d T;
    Matrix4x4d S;
    Matrix4x4d M;

    solid = PolyhedralBoundedSolidModeler::createCircularLamina(
        0.0, 0.0, bottomRadius, 0.0, nsides);

    if ( topRadius > VSDK::EPSILON && bottomRadius > VSDK::EPSILON ) {
        double prevRadius = bottomRadius;
        double zStep = height / ((double)heightDivisions);
        int i;
        for ( i = 1; i <= heightDivisions; i++ ) {
            double nextRadius = bottomRadius + (topRadius - bottomRadius) *
                (((double)i) / ((double)heightDivisions));
            double f = nextRadius / prevRadius;
            T = Matrix4x4d();
            T = T.translation(0.0, 0.0, zStep);
            S = Matrix4x4d();
            S = S.scale(f, f, 1.0);
            M = T.multiply(S);
            PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
                solid, solid->findFace(1), M);
            prevRadius = nextRadius;
        }
    }
    else if ( topRadius <= VSDK::EPSILON && bottomRadius > VSDK::EPSILON ) {
        // Cone case, with optional vertical subdivisions.
        double prevRadius = bottomRadius;
        double zStep = height / ((double)heightDivisions);
        int i;
        for ( i = 1; i < heightDivisions; i++ ) {
            double nextRadius = bottomRadius *
                (1.0 - (((double)i) / ((double)heightDivisions)));
            double f = nextRadius / prevRadius;
            T = Matrix4x4d();
            T = T.translation(0.0, 0.0, zStep);
            S = Matrix4x4d();
            S = S.scale(f, f, 1.0);
            M = T.multiply(S);
            PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
                solid, solid->findFace(1), M);
            prevRadius = nextRadius;
        }
        closeTopFaceToApex(solid, height);
    }
    return solid;
}
