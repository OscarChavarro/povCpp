//= References:                                                             =
//= [WAYN1990] Knapp Wayne. "Ray with Bicubic Patch Intersection Problem",  =
//=            Ray Tracing News, volume 3, number 3, july 13 1990.          =
//=            available at                                                 =
//=         http://jedi.ks.uiuc.edu/~johns/raytracer/rtn/rtnv3n3.html#art19 =
//= [FOLE1992] Foley, vanDam, Feiner, Hughes. "Computer Graphics, princi-   =
//=            ples and practice" - second edition, Addison Wesley, 1992.   =

#include <cstdio>
#include "java/util/concurrent/locks/ReentrantLock.h"

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/ParametricBiCubicPatch.h"

namespace {
java::ReentrantLock& rayIntersectorAccess();
}
ParametricBiCubicPatch::ParametricBiCubicPatch()
    : contourCurve(nullptr), hasControlMeshPoints(false),
      hasCoefficientMatrices(false), approximationSteps(INITIAL_APPROXIMATION_STEPS), type(ParametricCurve::HERMITE)
{
}

void ParametricBiCubicPatch::buildFergusonPatch(ParametricCurve* curve)
{
    contourCurve = curve;
    approximationSteps = INITIAL_APPROXIMATION_STEPS;
    type = FERGUSON;
    calculateMatrices();
}

void ParametricBiCubicPatch::buildBezierPatch(const Vector3Dd inControlMeshPoints[4][4])
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            controlMeshPoints[i][j] = inControlMeshPoints[i][j];
        }
    }
    hasControlMeshPoints = true;
    approximationSteps = INITIAL_APPROXIMATION_STEPS;
    type = ParametricCurve::BEZIER;
    calculateMatrices();
}

void ParametricBiCubicPatch::calculateMatrices()
{
    if (type == ParametricCurve::BEZIER) {
        buildGeometryMatricesXYZ_Bezier();
        basisMatrix = ParametricCurve::BEZIER_MATRIX;
    }
    else if (type == ParametricCurve::HERMITE) {
        buildGeometryMatricesXYZ_Hermite();
        basisMatrix = ParametricCurve::HERMITE_MATRIX;
    }
    else if (type == FERGUSON) {
        buildGeometryMatricesXYZ_Ferguson();
        basisMatrix = ParametricCurve::HERMITE_MATRIX;
    }

    transposedBasisMatrix = Matrix4x4d(basisMatrix).transpose();
    coefficientMatrixX = basisMatrix.multiply(geometryMatrixX).multiply(transposedBasisMatrix);
    coefficientMatrixY = basisMatrix.multiply(geometryMatrixY).multiply(transposedBasisMatrix);
    coefficientMatrixZ = basisMatrix.multiply(geometryMatrixZ).multiply(transposedBasisMatrix);
    sParameterMatrix = Matrix4x4d();
    tParameterMatrix = Matrix4x4d();
    sDerivativeParameterMatrix = Matrix4x4d();
    tDerivativeParameterMatrix = Matrix4x4d();
    hasCoefficientMatrices = true;
    {
        java::ReentrantLockGuard guard(rayIntersectorAccess());
        rayIntersector.reset();
    }
}

namespace {
/**
Serializes the lazy creation (and the reads) of the intersectors of the
patches, as the synchronized block of the Java version.
*/
java::ReentrantLock& rayIntersectorAccess()
{
    static java::ReentrantLock lock;
    return lock;
}
}

/**
Returns the ray intersection structures for current patch, building them
on first use.
@return intersector, or null if the patch has not been built yet
*/
SharedPointer<const _ParametricBiCubicPatchIntersector>
ParametricBiCubicPatch::getRayIntersector() const
{
    java::ReentrantLockGuard guard(rayIntersectorAccess());
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        rayIntersector;
    if ( !intersector ) {
        if ( !hasCoefficientMatrices ) {
            return intersector;
        }
        intersector = SharedPointer<const _ParametricBiCubicPatchIntersector>(
            new _ParametricBiCubicPatchIntersector(
                coefficientMatrixX, coefficientMatrixY, coefficientMatrixZ));
        rayIntersector = intersector;
    }
    return intersector;
}

bool ParametricBiCubicPatch::getBezierControlPoint(int i, int j,
                                                   Vector3Dd& outPoint) const
{
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        getRayIntersector();
    if ( !intersector ) {
        return false;
    }
    outPoint = intersector->getBezierControlPoint(i, j);
    return true;
}

int ParametricBiCubicPatch::getApproximationSteps() const { return approximationSteps; }
void ParametricBiCubicPatch::setApproximationSteps(int n) { approximationSteps = n; }
int ParametricBiCubicPatch::getType() const { return type; }
void ParametricBiCubicPatch::setType(int t) { type = t; }

void ParametricBiCubicPatch::buildGeometryMatricesXYZ_Bezier()
{
    double mx[4][4], my[4][4], mz[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vector3Dd& vp = controlMeshPoints[i][j];
            mx[i][j] = vp.x();
            my[i][j] = vp.y();
            mz[i][j] = vp.z();
        }
    }
    geometryMatrixX = Matrix4x4d::copyOf(mx);
    geometryMatrixY = Matrix4x4d::copyOf(my);
    geometryMatrixZ = Matrix4x4d::copyOf(mz);
}

void ParametricBiCubicPatch::buildGeometryMatricesXYZ_Hermite()
{
    if (contourCurve == nullptr) return;

    double mx[4][4], my[4][4], mz[4][4];

    int p = 0;
    int i = 0;
    for (int j = 0; j < 2; j++) {
        const Vector3Dd* vp = contourCurve->getPoint(p);
        mx[i][j] = vp[0].x(); my[i][j] = vp[0].y(); mz[i][j] = vp[0].z();
        mx[i][j + 2] = vp[2 - j].x(); my[i][j + 2] = vp[2 - j].y(); mz[i][j + 2] = vp[2 - j].z();
        mx[i + 2][j] = vp[1 + j].x(); my[i + 2][j] = vp[1 + j].y(); mz[i + 2][j] = vp[1 + j].z();
        mx[i + 2][j + 2] = 0; my[i + 2][j + 2] = 0; mz[i + 2][j + 2] = 0;
        p++;
    }

    p = 2;
    i = 1;
    for (int j = 0; j < 2; j++) {
        const Vector3Dd* vp = contourCurve->getPoint(p);
        mx[i][j] = vp[0].x(); my[i][j] = vp[0].y(); mz[i][j] = vp[0].z();
        mx[i][j + 2] = vp[j + 1].x(); my[i][j + 2] = vp[j + 1].y(); mz[i][j + 2] = vp[j + 1].z();
        mx[i + 2][j] = vp[2 - j].x(); my[i + 2][j] = vp[2 - j].y(); mz[i + 2][j] = vp[2 - j].z();
        mx[i + 2][j + 2] = 0; my[i + 2][j + 2] = 0; mz[i + 2][j + 2] = 0;
        p++;
    }

    geometryMatrixX = Matrix4x4d::copyOf(mx);
    geometryMatrixY = Matrix4x4d::copyOf(my);
    geometryMatrixZ = Matrix4x4d::copyOf(mz);
}

void ParametricBiCubicPatch::printGeometryMatrices() const
{
    double** mx = geometryMatrixX.toArrayCopy();
    double** my = geometryMatrixY.toArrayCopy();
    double** mz = geometryMatrixZ.toArrayCopy();

    for (int r = 0; r < 4; r++) {
        std::printf("[ <%.2f, %.2f, %.2f> | <%.2f, %.2f, %.2f> | <%.2f, %.2f, %.2f> | <%.2f, %.2f, %.2f> ]\n",
            mx[r][0], my[r][0], mz[r][0], mx[r][1], my[r][1], mz[r][1],
            mx[r][2], my[r][2], mz[r][2], mx[r][3], my[r][3], mz[r][3]);
    }

    for (int i = 0; i < 4; i++) {
        delete[] mx[i];
        delete[] my[i];
        delete[] mz[i];
    }
    delete[] mx;
    delete[] my;
    delete[] mz;
}

void ParametricBiCubicPatch::buildGeometryMatricesXYZ_Ferguson()
{
    if (contourCurve == nullptr) return;

    double mx[4][4], my[4][4], mz[4][4];
    const Vector3Dd* vp00 = contourCurve->getPoint(0);
    const Vector3Dd* vp10 = contourCurve->getPoint(1);
    const Vector3Dd* vp11 = contourCurve->getPoint(2);
    const Vector3Dd* vp01 = contourCurve->getPoint(3);

    mx[0][0] = vp00[0].x(); my[0][0] = vp00[0].y(); mz[0][0] = vp00[0].z();
    mx[0][1] = vp01[0].x(); my[0][1] = vp01[0].y(); mz[0][1] = vp01[0].z();
    mx[1][0] = vp10[0].x(); my[1][0] = vp10[0].y(); mz[1][0] = vp10[0].z();
    mx[1][1] = vp11[0].x(); my[1][1] = vp11[0].y(); mz[1][1] = vp11[0].z();

    mx[2][0] = (vp00[2].x()); my[2][0] = (vp00[2].y()); mz[2][0] = (vp00[2].z());
    mx[2][1] = -(vp01[1].x()); my[2][1] = -(vp01[1].y()); mz[2][1] = -(vp01[1].z());
    mx[3][0] = (vp10[1].x()); my[3][0] = (vp10[1].y()); mz[3][0] = (vp10[1].z());
    mx[3][1] = -(vp11[2].x()); my[3][1] = -(vp11[2].y()); mz[3][1] = -(vp11[2].z());

    mx[0][2] = -(vp00[1].x()); my[0][2] = -(vp00[1].y()); mz[0][2] = -(vp00[1].z());
    mx[0][3] = -(vp01[2].x()); my[0][3] = -(vp01[2].y()); mz[0][3] = -(vp01[2].z());
    mx[1][2] = (vp10[2].x()); my[1][2] = (vp10[2].y()); mz[1][2] = (vp10[2].z());
    mx[1][3] = (vp11[1].x()); my[1][3] = (vp11[1].y()); mz[1][3] = (vp11[1].z());

    mx[2][2] = 0; my[2][2] = 0; mz[2][2] = 0;
    mx[2][3] = 0; my[2][3] = 0; mz[2][3] = 0;
    mx[3][2] = 0; my[3][2] = 0; mz[3][2] = 0;
    mx[3][3] = 0; my[3][3] = 0; mz[3][3] = 0;

    geometryMatrixX = Matrix4x4d::copyOf(mx);
    geometryMatrixY = Matrix4x4d::copyOf(my);
    geometryMatrixZ = Matrix4x4d::copyOf(mz);
}

void ParametricBiCubicPatch::evaluate(Vector3Dd& p, double s, double t)
{
    sParameterMatrix = sParameterMatrix.withVal(0, 0, s*s*s).withVal(0, 1, s*s).withVal(0, 2, s).withVal(0, 3, 1);
    tParameterMatrix = tParameterMatrix.withVal(0, 0, t*t*t).withVal(1, 0, t*t).withVal(2, 0, t).withVal(3, 0, 1);

    Matrix4x4d Qx_MATRIX = sParameterMatrix.multiply(coefficientMatrixX).multiply(tParameterMatrix);
    Matrix4x4d Qy_MATRIX = sParameterMatrix.multiply(coefficientMatrixY).multiply(tParameterMatrix);
    Matrix4x4d Qz_MATRIX = sParameterMatrix.multiply(coefficientMatrixZ).multiply(tParameterMatrix);

    p = Vector3Dd(Qx_MATRIX.get(0, 0), Qy_MATRIX.get(0, 0), Qz_MATRIX.get(0, 0));
}

Vector3Dd ParametricBiCubicPatch::evaluateTangent(double s, double t)
{
    sDerivativeParameterMatrix = sDerivativeParameterMatrix.withVal(0, 0, 3*s*s).withVal(0, 1, 2*s).withVal(0, 2, 1).withVal(0, 3, 0);
    tParameterMatrix = tParameterMatrix.withVal(0, 0, t*t*t).withVal(1, 0, t*t).withVal(2, 0, t).withVal(3, 0, 1);

    Matrix4x4d Qx_MATRIX = sDerivativeParameterMatrix.multiply(coefficientMatrixX).multiply(tParameterMatrix);
    Matrix4x4d Qy_MATRIX = sDerivativeParameterMatrix.multiply(coefficientMatrixY).multiply(tParameterMatrix);
    Matrix4x4d Qz_MATRIX = sDerivativeParameterMatrix.multiply(coefficientMatrixZ).multiply(tParameterMatrix);

    return Vector3Dd(Qx_MATRIX.get(0, 0), Qy_MATRIX.get(0, 0), Qz_MATRIX.get(0, 0)).normalized();
}

Vector3Dd ParametricBiCubicPatch::evaluateBinormal(double s, double t)
{
    sParameterMatrix = sParameterMatrix.withVal(0, 0, s*s*s).withVal(0, 1, s*s).withVal(0, 2, s).withVal(0, 3, 1);
    tDerivativeParameterMatrix = tDerivativeParameterMatrix.withVal(0, 0, 3*t*t).withVal(1, 0, 2*t).withVal(2, 0, 1).withVal(3, 0, 0);

    Matrix4x4d Qx_MATRIX = sParameterMatrix.multiply(coefficientMatrixX).multiply(tDerivativeParameterMatrix);
    Matrix4x4d Qy_MATRIX = sParameterMatrix.multiply(coefficientMatrixY).multiply(tDerivativeParameterMatrix);
    Matrix4x4d Qz_MATRIX = sParameterMatrix.multiply(coefficientMatrixZ).multiply(tDerivativeParameterMatrix);

    return Vector3Dd(Qx_MATRIX.get(0, 0), Qy_MATRIX.get(0, 0), Qz_MATRIX.get(0, 0)).normalized();
}

Vector3Dd ParametricBiCubicPatch::evaluateNormal(double s, double t)
{
    Vector3Dd dQds = evaluateTangent(s, t);
    Vector3Dd dQdt = evaluateBinormal(s, t);
    return dQds.crossProduct(dQdt).normalized();
}

/**
Check the general interface contract in superclass method
Geometry.doIntersectionFirstHit.

Check the discusion in [WAYN1990] about solving this problem. This
implementation follows the subdivision strategy of POV-Ray's
bicubic_patch (as replicated on povCpp): the patch is converted to Bezier
form and recursively subdivided into nearly flat sub-patches, organized on
a tree of bounding spheres. Hits against the flat sub-patches are then
refined with Newton iterations over the exact surface.
@param r ray to test
@return a new copy of given ray with its t set at the nearest intersection,
or null if the ray misses the patch
*/
Ray* ParametricBiCubicPatch::doIntersectionFirstHit(const Ray& r)
{
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        getRayIntersector();
    _ParametricBiCubicPatchIntersector::PatchHit hit;
    if ( !intersector || !intersector->intersect(r, hit) ) {
        return nullptr;
    }
    return new Ray(r.withT(hit.t));
}

bool ParametricBiCubicPatch::doIntersectionFirstHit(const Ray& inRay,
                                                    RayHit* outHit)
{
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        getRayIntersector();
    _ParametricBiCubicPatchIntersector::PatchHit hit;
    if ( !intersector || !intersector->intersect(inRay, hit) ) {
        return false;
    }

    if ( outHit != nullptr ) {
        if ( outHit->shouldStoreRay() || outHit->needsAnySurfaceData() ) {
            Ray hitRay = inRay.withT(hit.t);
            outHit->setRay(hitRay);
            if ( outHit->needsAnySurfaceData() ) {
                fillHitInformation(*intersector, hit, inRay, outHit);
            }
        }
        else {
            outHit->setHitDistance(hit.t);
        }
    }
    return true;
}

/**
Check the general interface contract in superclass method
Geometry.doExtraInformation.

Since a patch surface point can not be mapped back to its (s, t)
parameters in closed form, the ray is intersected again and the
information reported is the one for the nearest hit.
@param inRay ray that intersects current patch
@param intT ray parameter at the intersection point
@param outData receives the intersection details
*/
void ParametricBiCubicPatch::doExtraInformation(const Ray& inRay, double intT,
                                                RayHit* outData)
{
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        getRayIntersector();
    if ( outData == nullptr || !intersector ) {
        return;
    }
    _ParametricBiCubicPatchIntersector::PatchHit hit;
    if ( !intersector->intersect(inRay, hit) ) {
        if ( outData->needsPoint() ) {
            outData->point = Vector3Dd(
                inRay.getOrigin().x() + intT*inRay.getDirection().x(),
                inRay.getOrigin().y() + intT*inRay.getDirection().y(),
                inRay.getOrigin().z() + intT*inRay.getDirection().z());
        }
        return;
    }
    fillHitInformation(*intersector, hit, inRay, outData);
}

/**
Fills point, normal, tangent and (s, t) texture coordinates for a hit.
The normal is oriented against the ray direction, as the patch is an
open surface.
*/
void ParametricBiCubicPatch::fillHitInformation(
    const _ParametricBiCubicPatchIntersector& intersector,
    const _ParametricBiCubicPatchIntersector::PatchHit& hit,
    const Ray& inRay, RayHit* outData)
{
    if ( outData->needsPoint() ) {
        outData->point = Vector3Dd(hit.pointX, hit.pointY, hit.pointZ);
    }
    if ( !outData->needsNormal() && !outData->needsTangent() &&
         !outData->needsTextureCoordinates() ) {
        return;
    }

    double derivatives[9];
    intersector.evaluate(hit.u, hit.v, derivatives);
    Vector3Dd dQds(derivatives[3], derivatives[4], derivatives[5]);
    Vector3Dd dQdt(derivatives[6], derivatives[7], derivatives[8]);
    Vector3Dd triangleNormal(hit.triangleNx, hit.triangleNy, hit.triangleNz);

    Vector3Dd normal = dQds.crossProduct(dQdt);
    if ( normal.length() <= VSDK::EPSILON * VSDK::EPSILON ) {
        // Degenerated parameterization (i.e. collapsed patch border)
        normal = triangleNormal;
    }
    normal = normal.normalized();
    if ( normal.dotProduct(inRay.getDirection()) > 0 ) {
        normal = normal.multiply(-1);
    }

    if ( outData->needsNormal() ) {
        outData->normal = normal;
    }
    if ( outData->needsTangent() ) {
        Vector3Dd tangent = dQds;
        if ( tangent.length() <= VSDK::EPSILON * VSDK::EPSILON ) {
            tangent = dQdt;
        }
        outData->tangent = tangent.normalized();
    }
    if ( outData->needsTextureCoordinates() ) {
        outData->u = hit.u;
        outData->v = hit.v;
    }
}

/**
Returns a bounding volume minmax for current patch. When the patch has
been built, this is the minmax of its equivalent Bezier control net, which
contains the whole patch by the convex hull property.
@return a new 6 valued double array containing the coordinates of a min-max
bounding box for current geometry.
*/
double* ParametricBiCubicPatch::getMinMax()
{
    SharedPointer<const _ParametricBiCubicPatchIntersector> intersector =
        getRayIntersector();
    if ( intersector ) {
        double* minMax = new double[6];
        intersector->getMinMax(minMax);
        return minMax;
    }
    else if (contourCurve != nullptr) {
        return contourCurve->getMinMax();
    }

    double minX = 1e308, minY = 1e308, minZ = 1e308;
    double maxX = -1e308, maxY = -1e308, maxZ = -1e308;
    double* minMax = new double[6];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vector3Dd& p = controlMeshPoints[i][j];
            if (p.x() < minX) minX = p.x();
            if (p.y() < minY) minY = p.y();
            if (p.z() < minZ) minZ = p.z();
            if (p.x() > maxX) maxX = p.x();
            if (p.y() > maxY) maxY = p.y();
            if (p.z() > maxZ) maxZ = p.z();
        }
    }

    minMax[0] = minX; minMax[1] = minY; minMax[2] = minZ;
    minMax[3] = maxX; minMax[4] = maxY; minMax[5] = maxZ;
    return minMax;
}
