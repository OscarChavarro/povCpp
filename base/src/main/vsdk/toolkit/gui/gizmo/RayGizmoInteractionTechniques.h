#ifndef __RAY_GIZMO_INTERACTION_TECHNIQUES__
#define __RAY_GIZMO_INTERACTION_TECHNIQUES__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/gui/KeyEvent.h"

class InputGizmo;
class RayGizmo;

/**
Keyboard interaction for a RayGizmo. The ray can be toggled with one of the
configured activation keys and then controlled from the numeric keypad:
NUM4/NUM6 move X, NUM2/NUM8 move Y, NUM1/NUM7 move Z, NUM* and NUM/ change
yaw, and NUM+/NUM- change pitch. Regular numeric editing is delegated to the
gizmo's five-field InputGizmo: x, y, z, yaw and pitch.
*/
class RayGizmoInteractionTechniques {
private:
    static const double ORIGIN_STEP;
    static const double ANGLE_STEP_DEGREES;

    RayGizmo* gizmo;
    java::ArrayList<int> activationKeys;
    bool active;
    Vector3Dd origin;
    double yawInDegrees;
    double pitchInDegrees;

    bool processNumericPadCommand(const KeyEvent& event);
    bool isActivationKey(const KeyEvent& event) const;
    Ray currentRay() const;
    void applyRay();
    void applyInputGizmoValues();
    void updateInputGizmo();
    void updateAnglesFromDirection(const Vector3Dd& direction);
    Vector3Dd direction() const;
    void clampPitch();

public:
    RayGizmoInteractionTechniques(RayGizmo* gizmo);
    RayGizmoInteractionTechniques(RayGizmo* gizmo, int activationKey1);
    RayGizmoInteractionTechniques(RayGizmo* gizmo, int activationKey1,
                                  int activationKey2);
    ~RayGizmoInteractionTechniques();

    bool isActive() const;
    void setActive(bool active);
    InputGizmo* getInputGizmo();
    bool processKeyPressedEvent(const KeyEvent& event);
    void setRay(const Ray* ray);
};

#endif
