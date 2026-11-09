#include "java/lang/Math.h"

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/gui/gizmo/InfinitePlaneGizmo.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent2InfinitePlaneGizmoMapper.h"

const Vector3Dd TangibleInterfaceEvent2InfinitePlaneGizmoMapper::MARKER_INTO_SCENE = Vector3Dd(0, -1, 0);
const Vector3Dd TangibleInterfaceEvent2InfinitePlaneGizmoMapper::MARKER_PLANE_NORMAL = Vector3Dd(0, 0, 1);
const double TangibleInterfaceEvent2InfinitePlaneGizmoMapper::DISTANCE_FACTOR = 4;
const double TangibleInterfaceEvent2InfinitePlaneGizmoMapper::MAX_GIZMO_DEPTH_FACTOR = 2.5;

TangibleInterfaceEvent2InfinitePlaneGizmoMapper::TangibleInterfaceEvent2InfinitePlaneGizmoMapper(Camera* camera)
    : camera(camera)
{
}

void TangibleInterfaceEvent2InfinitePlaneGizmoMapper::map(const TangibleInterfaceEvent& event, InfinitePlaneGizmo* gizmo)
{
    if ( camera == 0 || gizmo == 0 ) {
        return;
    }

    camera->updateVectors();

    Vector3Dd netPosition = event.getPosition();
    Vector3Dd netNormal = event.getRotation().rotate(MARKER_PLANE_NORMAL);

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

    Vector3Dd worldNormal =
        camRight.multiply(netNormal.x())
        .add(camUp.multiply(netNormal.y()))
        .add(camFront.multiply(netNormal.z()));

    if ( worldNormal.length() < VSDK::EPSILON ) {
        worldNormal = camUp;
    }

    Vector3Dd worldRayDirection =
        mapDirection(event.getRotation().rotate(MARKER_INTO_SCENE),
            camRight, camUp, camFront);
    worldNormal = removeRayComponent(worldNormal, worldRayDirection, camUp);

    gizmo->setPlane(worldPosition, worldNormal);
}

Vector3Dd TangibleInterfaceEvent2InfinitePlaneGizmoMapper::mapDirection(
    const Vector3Dd& netDirection,
    const Vector3Dd& camRight,
    const Vector3Dd& camUp,
    const Vector3Dd& camFront)
{
    return camRight.multiply(netDirection.x())
        .add(camUp.multiply(netDirection.y()))
        .add(camFront.multiply(netDirection.z()));
}

Vector3Dd TangibleInterfaceEvent2InfinitePlaneGizmoMapper::removeRayComponent(
    const Vector3Dd& normal,
    const Vector3Dd& rayDirection,
    const Vector3Dd& fallback)
{
    if ( rayDirection.length() < VSDK::EPSILON ) {
        return normal.normalized();
    }
    Vector3Dd ray = rayDirection.normalized();
    Vector3Dd projected = normal.subtract(ray.multiply(normal.dotProduct(ray)));
    if ( projected.length() >= VSDK::EPSILON ) {
        return projected.normalized();
    }
    projected = fallback.subtract(ray.multiply(fallback.dotProduct(ray)));
    if ( projected.length() >= VSDK::EPSILON ) {
        return projected.normalized();
    }
    return normal.normalized();
}
