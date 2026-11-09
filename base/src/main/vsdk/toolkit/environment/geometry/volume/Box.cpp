#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
static const Vector3Dd NORMAL_POS_Z(0, 0, 1);
static const Vector3Dd NORMAL_NEG_Z(0, 0, -1);
static const Vector3Dd NORMAL_POS_Y(0, 1, 0);
static const Vector3Dd NORMAL_NEG_Y(0, -1, 0);
static const Vector3Dd NORMAL_POS_X(1, 0, 0);
static const Vector3Dd NORMAL_NEG_X(-1, 0, 0);
static const Vector3Dd TANGENT_POS_Y(0, 1, 0);
static const Vector3Dd TANGENT_NEG_Y(0, -1, 0);
static const Vector3Dd TANGENT_NEG_X(-1, 0, 0);
static const Vector3Dd TANGENT_POS_X(1, 0, 0);
static const Vector3Dd ZERO_VECTOR(0, 0, 0);

Box::Box(double dx, double dy, double dz) : size(dx, dy, dz) {}
Box::Box(const Vector3Dd& s) : size(s) {}

Ray* Box::doIntersectionFirstHit(const Ray& inOutRay) {
    RayHit hit;
    if (doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != nullptr) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool Box::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    double minT = 1e308;
    int hitPlane = 0;
    double x2 = size.x()/2, y2 = size.y()/2, z2 = size.z()/2;
    double ox = inRay.getOrigin().x(), oy = inRay.getOrigin().y(), oz = inRay.getOrigin().z();
    double dx = inRay.getDirection().x(), dy = inRay.getDirection().y(), dz = inRay.getDirection().z();

    if (std::abs(dz) > VSDK::EPSILON) {
        double t = (z2 - oz) / dz;
        if (t > -VSDK::EPSILON) {
            double cx = ox + dx*t, cy = oy + dy*t;
            if (cx >= -x2 && cx <= x2 && cy >= -y2 && cy <= y2) { minT = t; hitPlane = 1; }
        }
    }
    if (std::abs(dz) > VSDK::EPSILON) {
        double t = (-z2 - oz) / dz;
        if (t > -VSDK::EPSILON && t < minT) {
            double cx = ox + dx*t, cy = oy + dy*t;
            if (cx >= -x2 && cx <= x2 && cy >= -y2 && cy <= y2) { minT = t; hitPlane = 2; }
        }
    }
    if (std::abs(dy) > VSDK::EPSILON) {
        double t = (y2 - oy) / dy;
        if (t > -VSDK::EPSILON && t < minT) {
            double cx = ox + dx*t, cz = oz + dz*t;
            if (cx >= -x2 && cx <= x2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 3; }
        }
    }
    if (std::abs(dy) > VSDK::EPSILON) {
        double t = (-y2 - oy) / dy;
        if (t > -VSDK::EPSILON && t < minT) {
            double cx = ox + dx*t, cz = oz + dz*t;
            if (cx >= -x2 && cx <= x2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 4; }
        }
    }
    if (std::abs(dx) > VSDK::EPSILON) {
        double t = (x2 - ox) / dx;
        if (t > -VSDK::EPSILON && t < minT) {
            double cy = oy + dy*t, cz = oz + dz*t;
            if (cy >= -y2 && cy <= y2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 5; }
        }
    }
    if (std::abs(dx) > VSDK::EPSILON) {
        double t = (-x2 - ox) / dx;
        if (t > -VSDK::EPSILON && t < minT) {
            double cy = oy + dy*t, cz = oz + dz*t;
            if (cy >= -y2 && cy <= y2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 6; }
        }
    }

    if (minT == 1e308) return false;
    if (outHit == nullptr) return true;

    if (outHit->shouldStoreRay() || outHit->needsAnySurfaceData()) outHit->setRay(inRay.withT(minT));
    else outHit->setHitDistance(minT);

    if (outHit->needsAnySurfaceData()) {
        double hitX = ox + dx*minT, hitY = oy + dy*minT, hitZ = oz + dz*minT;
        if (outHit->needsPoint()) outHit->point = Vector3Dd(hitX, hitY, hitZ);
        if (outHit->needsTextureCoordinates()) {
            outHit->u = 0; outHit->v = 0;
            switch (hitPlane) {
                case 1: outHit->u = hitY / size.y() - 0.5; outHit->v = 1-(hitX / size.x() - 0.5); break;
                case 2: outHit->u = hitY / size.y() - 0.5; outHit->v = hitX / size.x() - 0.5; break;
                case 3: outHit->u = 1-(hitX / size.x() - 0.5); outHit->v = hitZ / size.z() - 0.5; break;
                case 4: outHit->u = hitX / size.x() - 0.5; outHit->v = hitZ / size.z() - 0.5; break;
                case 5: outHit->u = hitY / size.y() - 0.5; outHit->v = hitZ / size.z() - 0.5; break;
                case 6: outHit->u = 1-(hitY / size.y() - 0.5); outHit->v = hitZ / size.z() - 0.5; break;
            }
        }
        if (outHit->needsNormal()) outHit->normal = planeNormal(hitPlane);
        if (outHit->needsTangent()) outHit->tangent = planeTangent(hitPlane);
    }
    return true;
}

void Box::doExtraInformation(const Ray& inRay, double inT, RayHit* outData) {
    if (outData == nullptr) return;
    double hitX = inRay.getOrigin().x() + inRay.getDirection().x()*inT;
    double hitY = inRay.getOrigin().y() + inRay.getDirection().y()*inT;
    double hitZ = inRay.getOrigin().z() + inRay.getDirection().z()*inT;
    int hitPlane = classifyHitPlane(hitX, hitY, hitZ);
    if (outData->needsPoint()) outData->point = Vector3Dd(hitX, hitY, hitZ);
    if (outData->needsNormal()) outData->normal = planeNormal(hitPlane);
    if (outData->needsTangent()) outData->tangent = planeTangent(hitPlane);
}

Vector3Dd Box::planeNormal(int h) { switch (h){case 1:return NORMAL_POS_Z;case 2:return NORMAL_NEG_Z;case 3:return NORMAL_POS_Y;case 4:return NORMAL_NEG_Y;case 5:return NORMAL_POS_X;case 6:return NORMAL_NEG_X;default:return ZERO_VECTOR;}}
Vector3Dd Box::planeTangent(int h) { switch (h){case 1:case 2:case 5:return TANGENT_POS_Y;case 3:return TANGENT_NEG_X;case 4:return TANGENT_POS_X;case 6:return TANGENT_NEG_Y;default:return ZERO_VECTOR;}}

int Box::classifyHitPlane(double x, double y, double z) const {
    double x2=size.x()/2,y2=size.y()/2,z2=size.z()/2;
    double dxPlus=std::abs(x-x2),dxMinus=std::abs(x+x2),dyPlus=std::abs(y-y2),dyMinus=std::abs(y+y2),dzPlus=std::abs(z-z2),dzMinus=std::abs(z+z2);
    double min=dzPlus; int plane=1;
    if(dzMinus<min){min=dzMinus;plane=2;} if(dyPlus<min){min=dyPlus;plane=3;} if(dyMinus<min){min=dyMinus;plane=4;} if(dxPlus<min){min=dxPlus;plane=5;} if(dxMinus<min){plane=6;} return plane;
}

/**
Check the general interface contract in superclass method
Geometry.doContainmentTest. The box is centered at the origin, with its
faces perpendicular to the axes.
@param p point to classify, in the space of the box
@param distanceTolerance distance to the faces of the box under which
the point is classified as `LIMIT`
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int Box::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    double half[3] = { size.x() / 2, size.y() / 2, size.z() / 2 };
    double coordinates[3] = { p.x(), p.y(), p.z() };
    bool inside = true;

    for ( int i = 0; i < 3; i++ ) {
        double distance = std::abs(coordinates[i]);
        if ( distance > half[i] + distanceTolerance ) {
            return OUTSIDE;
        }
        if ( distance >= half[i] - distanceTolerance ) {
            inside = false;
        }
    }
    return inside ? INSIDE : LIMIT;
}

double* Box::getMinMax() { double* m=new double[6]; m[0]=-size.x()/2; m[1]=-size.y()/2; m[2]=-size.z()/2; m[3]=size.x()/2; m[4]=size.y()/2; m[5]=size.z()/2; return m; }
Vector3Dd Box::getSize() const { return size; }
void Box::setSize(double dx,double dy,double dz){ setSize(Vector3Dd(dx,dy,dz)); }
void Box::setSize(const Vector3Dd& s){ size=s; }

PolyhedralBoundedSolid* Box::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid();
}

PolyhedralBoundedSolid* Box::buildPolyhedralBoundedSolid()
{
    PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(
        solid, Vector3Dd(-size.x()/2, -size.y()/2, -size.z()/2), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 1, 4, Vector3Dd(-size.x()/2, size.y()/2, -size.z()/2));
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 4, 3, Vector3Dd(size.x()/2, size.y()/2, -size.z()/2));
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 3, 2, Vector3Dd(size.x()/2, -size.y()/2, -size.z()/2));
    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, 1, 4, 2, 3, 2);

    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 1, 5, Vector3Dd(-size.x()/2, -size.y()/2, size.z()/2));
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 2, 6, Vector3Dd(size.x()/2, -size.y()/2, size.z()/2));
    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, 5, 1, 6, 2, 3);
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 3, 7, Vector3Dd(size.x()/2, size.y()/2, size.z()/2));
    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, 6, 2, 7, 3, 4);
    PolyhedralBoundedSolidEulerOperators::smev(
        solid, 1, 4, 8, Vector3Dd(-size.x()/2, size.y()/2, size.z()/2));
    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, 7, 3, 8, 4, 5);
    PolyhedralBoundedSolidEulerOperators::mef(
        solid, 1, 1, 5, 6, 8, 4, 6);
    return solid;
}
