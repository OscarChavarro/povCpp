#include <cfloat>

#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleBodyGroup.h"
SimpleBodyGroup::SimpleBodyGroup()
    : position(0, 0, 0), scale(1, 1, 1), rotation(), rotationInverse(), name("")
{
}

java::ArrayList<SimpleBody*>& SimpleBodyGroup::getBodies() { return bodies; }
const java::String& SimpleBodyGroup::getName() const { return name; }
void SimpleBodyGroup::setName(const java::String& n) { name = n; }

Matrix4x4d SimpleBodyGroup::getRotation() const { return rotation; }
void SimpleBodyGroup::setRotation(const Matrix4x4d& r) { rotation = r.withoutTranslation(); }
Matrix4x4d SimpleBodyGroup::getRotationInverse() const { return rotationInverse; }
void SimpleBodyGroup::setRotationInverse(const Matrix4x4d& value) { rotationInverse = value; }

Vector3Dd SimpleBodyGroup::getPosition() const { return position; }
void SimpleBodyGroup::setPosition(const Vector3Dd& p) { position = p; }
Vector3Dd SimpleBodyGroup::getScale() const { return scale; }
void SimpleBodyGroup::setScale(const Vector3Dd& s) { scale = s; }

Matrix4x4d SimpleBodyGroup::getTransformationMatrix() const
{
    Matrix4x4d S = Matrix4x4d().scale(scale);
    Matrix4x4d T = Matrix4x4d().translation(position);
    return T.multiply(rotation.multiply(S));
}

Ray* SimpleBodyGroup::doIntersectionFirstHit(const Ray& inputRay)
{
    Ray inOutRay = inputRay.withT(DBL_MAX);

    Ray myRay(
        rotationInverse.multiply(inOutRay.getOrigin().subtract(position)),
        rotationInverse.multiply(inOutRay.getDirection()),
        inOutRay.getT());

    Ray* nearestHit = 0;

    for (long int i = 0; i < bodies.size(); i++) {
        if ( bodies[i] == 0 || bodies[i]->getGeometry() == 0 ) continue;
        RayHit hit;
        if ( bodies[i]->getGeometry()->doIntersectionFirstHit(myRay, &hit) && hit.getRay() != 0 ) {
            if ( hit.getRay()->getT() < inOutRay.getT() ) {
                inOutRay = inOutRay.withT(hit.getRay()->getT());
                if ( nearestHit != 0 ) delete nearestHit;
                nearestHit = new Ray(inOutRay);
            }
        }
    }

    return nearestHit;
}

double* SimpleBodyGroup::getMinMax()
{
    java::ArrayList<Vector3Dd> points;

    for (long int i = 0; i < bodies.size(); i++) {
        SimpleBody* bi = bodies[i];
        if ( bi == 0 || bi->getGeometry() == 0 ) continue;
        double* minmaxSub = bi->getGeometry()->getMinMax();
        Matrix4x4d R = bi->getRotation();
        Matrix4x4d T = Matrix4x4d().translation(bi->getPosition());
        Matrix4x4d S = Matrix4x4d().scale(bi->getScale());
        Matrix4x4d M = T.multiply(R).multiply(S);

        points.add(M.multiply(Vector3Dd(minmaxSub[0], minmaxSub[1], minmaxSub[2])));
        points.add(M.multiply(Vector3Dd(minmaxSub[3], minmaxSub[1], minmaxSub[2])));
        points.add(M.multiply(Vector3Dd(minmaxSub[0], minmaxSub[4], minmaxSub[2])));
        points.add(M.multiply(Vector3Dd(minmaxSub[3], minmaxSub[4], minmaxSub[2])));
        points.add(M.multiply(Vector3Dd(minmaxSub[0], minmaxSub[1], minmaxSub[5])));
        points.add(M.multiply(Vector3Dd(minmaxSub[3], minmaxSub[1], minmaxSub[5])));
        points.add(M.multiply(Vector3Dd(minmaxSub[0], minmaxSub[4], minmaxSub[5])));
        points.add(M.multiply(Vector3Dd(minmaxSub[3], minmaxSub[4], minmaxSub[5])));

        delete[] minmaxSub;
    }

    double* MinMax = new double[6];
    double minX = DBL_MAX, minY = DBL_MAX, minZ = DBL_MAX;
    double maxX = -DBL_MAX, maxY = -DBL_MAX, maxZ = -DBL_MAX;

    for (long int i = 0; i < points.size(); i++) {
        const Vector3Dd& p = points[i];
        if ( p.x() < minX ) minX = p.x();
        if ( p.y() < minY ) minY = p.y();
        if ( p.z() < minZ ) minZ = p.z();
        if ( p.x() > maxX ) maxX = p.x();
        if ( p.y() > maxY ) maxY = p.y();
        if ( p.z() > maxZ ) maxZ = p.z();
    }

    MinMax[0] = minX; MinMax[1] = minY; MinMax[2] = minZ;
    MinMax[3] = maxX; MinMax[4] = maxY; MinMax[5] = maxZ;
    return MinMax;
}
