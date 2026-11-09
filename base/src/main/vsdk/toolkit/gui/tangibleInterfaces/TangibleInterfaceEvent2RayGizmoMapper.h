#ifndef __TANGIBLE_INTERFACE_EVENT_2_RAY_GIZMO_MAPPER__
#define __TANGIBLE_INTERFACE_EVENT_2_RAY_GIZMO_MAPPER__

class Camera;
class RayGizmo;
class TangibleInterfaceEvent;

#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class TangibleInterfaceEvent2RayGizmoMapper {
private:
    static const Vector3Dd MARKER_INTO_SCENE;
    static const double DISTANCE_FACTOR;
    static const double MAX_GIZMO_DEPTH_FACTOR;

    Camera* camera;

    static double computeRollAngle(
        const Quaterniond& rotation,
        const Vector3Dd& worldDirection,
        const Vector3Dd& camRight,
        const Vector3Dd& camUp,
        const Vector3Dd& camFront);

public:
    explicit TangibleInterfaceEvent2RayGizmoMapper(Camera* camera);
    void map(const TangibleInterfaceEvent& event, RayGizmo* gizmo);
};

#endif
