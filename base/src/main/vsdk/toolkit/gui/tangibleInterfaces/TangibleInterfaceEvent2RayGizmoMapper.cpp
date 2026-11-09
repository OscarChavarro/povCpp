#include "java/lang/Math.h"
#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/gui/gizmo/RayGizmo.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent2RayGizmoMapper.h"

const Vector3Dd TangibleInterfaceEvent2RayGizmoMapper::MARKER_INTO_SCENE = Vector3Dd(0, -1, 0);
const double TangibleInterfaceEvent2RayGizmoMapper::DISTANCE_FACTOR = 4;
const double TangibleInterfaceEvent2RayGizmoMapper::MAX_GIZMO_DEPTH_FACTOR = 2.5;

TangibleInterfaceEvent2RayGizmoMapper::TangibleInterfaceEvent2RayGizmoMapper(Camera* camera)
    : camera(camera)
{
}

void TangibleInterfaceEvent2RayGizmoMapper::map(const TangibleInterfaceEvent& event, RayGizmo* gizmo)
{
    if ( camera == 0 || gizmo == 0 ) {
        return;
    }

    camera->updateVectors();

    Vector3Dd netPosition = event.getPosition();
    Vector3Dd netDirection = event.getRotation().rotate(MARKER_INTO_SCENE);

    Vector3Dd camPos = camera->getPosition();
    double nearPlane = camera->getNearPlaneDistance();
    double farPlane = camera->getFarPlaneDistance();

    Vector3Dd camRight = camera->getLeft().multiply(-1.0);
    Vector3Dd camUp = camera->getUp();
    Vector3Dd camFront = camera->getFront();

    double midDepth = (nearPlane + farPlane) * 0.5;
    double netZRef = 0.5;
    double depthScale = midDepth / netZRef;

    double safeNetZ = java::Math::max(netPosition.z(), 0.05);
    double gizmoZ = java::Math::min(depthScale * MAX_GIZMO_DEPTH_FACTOR,
        java::Math::max(nearPlane * 1.5, depthScale * netZRef * netZRef / safeNetZ)) - 14;

    Vector3Dd worldPosition =
        camPos
        .add(camRight.multiply(-netPosition.x() * depthScale * DISTANCE_FACTOR))
        .add(camUp.multiply(-netPosition.y() * depthScale * DISTANCE_FACTOR))
        .add(camFront.multiply(gizmoZ));

    Vector3Dd worldDirection =
        camRight.multiply(netDirection.x())
        .add(camUp.multiply(netDirection.y()))
        .add(camFront.multiply(netDirection.z()));

    double rollAngle = computeRollAngle(
        event.getRotation(), worldDirection, camRight, camUp, camFront);

    gizmo->setRay(Ray(worldPosition, worldDirection), rollAngle);
}

double TangibleInterfaceEvent2RayGizmoMapper::computeRollAngle(
    const Quaterniond& rotation,
    const Vector3Dd& worldDirection,
    const Vector3Dd& camRight,
    const Vector3Dd& camUp,
    const Vector3Dd& camFront)
{
    Vector3Dd netCubeUp = rotation.rotate(Vector3Dd(0, 0, 1));
    Vector3Dd worldCubeUp = camRight.multiply(netCubeUp.x())
        .add(camUp.multiply(netCubeUp.y()))
        .add(camFront.multiply(netCubeUp.z()));

    Vector3Dd wdNorm = worldDirection.normalized();
    Vector3Dd projCubeUp = worldCubeUp.subtract(wdNorm.multiply(wdNorm.dotProduct(worldCubeUp)));
    Vector3Dd projRef = camUp.subtract(wdNorm.multiply(wdNorm.dotProduct(camUp)));

    if ( projRef.length() < VSDK::EPSILON ) {
        projRef = camRight.subtract(wdNorm.multiply(wdNorm.dotProduct(camRight)));
    }
    if ( projCubeUp.length() < VSDK::EPSILON || projRef.length() < VSDK::EPSILON ) {
        return 0.0;
    }

    double cosAngle = projRef.dotProduct(projCubeUp);
    double sinAngle = projRef.crossProduct(projCubeUp).dotProduct(wdNorm);
    return std::atan2(sinAngle, cosAngle);
}
