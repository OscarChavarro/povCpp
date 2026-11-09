#ifndef __TANGIBLE_INTERFACE_EVENT_2_INFINITE_PLANE_GIZMO_MAPPER__
#define __TANGIBLE_INTERFACE_EVENT_2_INFINITE_PLANE_GIZMO_MAPPER__

class Camera;
class InfinitePlaneGizmo;
class TangibleInterfaceEvent;

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class TangibleInterfaceEvent2InfinitePlaneGizmoMapper {
  private:
    static const Vector3Dd MARKER_INTO_SCENE;
    static const Vector3Dd MARKER_PLANE_NORMAL;
    static const double DISTANCE_FACTOR;
    static const double MAX_GIZMO_DEPTH_FACTOR;

    Camera* camera;

    static Vector3Dd mapDirection(
        const Vector3Dd& netDirection,
        const Vector3Dd& camRight,
        const Vector3Dd& camUp,
        const Vector3Dd& camFront);
    static Vector3Dd removeRayComponent(
        const Vector3Dd& normal,
        const Vector3Dd& rayDirection,
        const Vector3Dd& fallback);

  public:
    explicit TangibleInterfaceEvent2InfinitePlaneGizmoMapper(Camera* camera);
    void map(const TangibleInterfaceEvent& event, InfinitePlaneGizmo* gizmo);
};

#endif
