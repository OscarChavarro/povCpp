#include <cctype>
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"
#include "vsdk/toolkit/gui/gizmo/RotateGizmo.h"
#include "vsdk/toolkit/gui/gizmo/RotateGizmoInteractionTechnique.h"

namespace {
const double KEY_ROTATION_STEP = 1.0 * M_PI / 180.0;

/// Minimum cosine of the angle between the view ray and the axis of the
/// ring (that is, sine of the angle with its plane) to measure angles
const double MIN_RAY_TO_AXIS_COSINE = 0.05;
}

RotateGizmoInteractionTechnique::RotateGizmoInteractionTechnique(
    RotateGizmo* gizmo)
    : gizmo(gizmo), active(false)
{
    endGesture();
}

RotateGizmo* RotateGizmoInteractionTechnique::getGizmo() const
{
    return gizmo;
}

bool RotateGizmoInteractionTechnique::isActive() const
{
    return active;
}

Viewport* RotateGizmoInteractionTechnique::getDragViewport() const
{
    return dragViewport;
}

bool RotateGizmoInteractionTechnique::isDragging() const
{
    return dragRing >= 0;
}

bool RotateGizmoInteractionTechnique::isInputGizmoKey(
    const KeyEvent& keyEvent)
{
    return gizmo->getInputGizmo()->consumesKey(keyEvent);
}

bool RotateGizmoInteractionTechnique::processKeyPressedEvent(
    const KeyEvent& keyEvent)
{
    InputGizmo* inputGizmo = gizmo->getInputGizmo();

    if ( inputGizmo->processKeyPressedEvent(keyEvent) ) {
        if ( inputGizmo->consumeCommit() ) {
            java::ArrayList<double> values = inputGizmo->getValuesWithEdits();

            inputGizmo->cancelEditing();

            Matrix4x4d rotation =
                RotateGizmo::createRotationFromAnglesInDegrees(
                    values[0], values[1], values[2]).withoutTranslation();
            double cameraDeltaDegrees =
                values[RotateGizmo::CAMERA_INPUT_FIELD_INDEX];

            if ( std::fabs(cameraDeltaDegrees) > VSDK::EPSILON ) {
                // The fourth field is relative: it rotates on top of the
                // orientation given by the other three, around the
                // current camera axis, and goes back to 0
                Matrix4x4d cameraDelta = Matrix4x4d().axisRotation(
                    cameraDeltaDegrees * M_PI / 180.0,
                    gizmo->getCameraAxisDirection());

                rotation = cameraDelta.multiply(rotation);
            }
            gizmo->setTransformationMatrix(
                rotation.withTranslation(gizmo->getPosition()));
            return true;
        }
        return false;
    }

    Vector3Dd axis;

    switch ( keyEvent.unicodeId ) {
      case 'x': case 'X':
        axis = Vector3Dd(1, 0, 0);
        break;
      case 'y': case 'Y':
        axis = Vector3Dd(0, 1, 0);
        break;
      case 'z': case 'Z':
        axis = Vector3Dd(0, 0, 1);
        break;
      default:
        return false;
    }

    double angle = std::isupper((unsigned char)keyEvent.unicodeId) ?
        KEY_ROTATION_STEP : -KEY_ROTATION_STEP;
    Matrix4x4d delta = Matrix4x4d().axisRotation(angle, axis);

    inputGizmo->cancelEditing();
    gizmo->setTransformationMatrix(
        gizmo->getTransformationMatrix().multiply(delta));
    return true;
}

bool RotateGizmoInteractionTechnique::processKeyReleasedEvent(const KeyEvent&)
{
    return false;
}

bool RotateGizmoInteractionTechnique::processMousePressedEvent(
    const MouseEvent& e, Viewport* viewport)
{
    bool changed = processMousePressedEvent(e);

    dragViewport = isDragging() ? viewport : nullptr;
    return changed;
}

bool RotateGizmoInteractionTechnique::processMousePressedEvent(
    const MouseEvent& e)
{
    endGesture();
    int selection = calculateSelection(e.getX(), e.getY());

    if ( selection != RotateGizmo::NULL_GROUP ) {
        // Groups are 1-based (X=1, Y=2, Z=3, camera=4); ring indexes are
        // 0-based (0, 1, 2, and RotateGizmo::CAMERA_RING_INDEX for the
        // camera ring), so they line up as `selection - 1`
        beginGesture(selection - 1, e.getX(), e.getY());
    }
    return false;
}

bool RotateGizmoInteractionTechnique::processMouseReleasedEvent(
    const MouseEvent&)
{
    bool wasDragging = isDragging();

    endGesture();
    return wasDragging;
}

bool RotateGizmoInteractionTechnique::processMouseClickedEvent(
    const MouseEvent& e)
{
    int previousSelection = gizmo->getCurrentSelection();
    int selection = calculateSelection(e.getX(), e.getY());

    if ( selection == RotateGizmo::NULL_GROUP ) {
        selection = previousSelection;
    }
    gizmo->setPersistentSelection(selection);
    return selection != previousSelection;
}

bool RotateGizmoInteractionTechnique::processMouseMovedEvent(
    const MouseEvent& e)
{
    if ( isDragging() ) {
        // The ring is fixed while dragging
        return false;
    }
    int previousSelection = gizmo->getCurrentSelection();
    int selection = calculateSelection(e.getX(), e.getY());

    gizmo->setVolatileSelection(selection);
    return gizmo->getCurrentSelection() != previousSelection;
}

bool RotateGizmoInteractionTechnique::processMouseDraggedEvent(
    const MouseEvent& e)
{
    if ( !isDragging() ) {
        return false;
    }

    double angle = calculateAngle(e.getX(), e.getY());

    if ( std::isnan(angle) ) {
        return false;
    }
    if ( std::isnan(dragLastAngle) ) {
        // First point of the plane the cursor defines: the arc starts here
        dragStartAngle = angle;
    }
    else {
        double delta = angle - dragLastAngle;

        // The shortest way from the previous angle: the sweep is
        // continuous when the cursor goes around the axis
        delta -= 2 * M_PI * std::floor(delta / (2 * M_PI) + 0.5);
        dragSweep += delta;
    }
    dragLastAngle = angle;

    // `dragAxis` is fixed (in world space) since the gesture started, so
    // rotating around it, then keeping the position the gizmo had when
    // pressed, is equivalent to rotating around the matching local axis
    // of `dragStartTransformation` (used for the X, Y and Z rings) but
    // also works for the camera ring, whose axis has no local counterpart
    Matrix4x4d rotationOnly = Matrix4x4d().axisRotation(dragSweep, dragAxis)
        .multiply(dragStartTransformation.withoutTranslation());

    gizmo->getInputGizmo()->cancelEditing();
    gizmo->setTransformationMatrix(
        rotationOnly.withTranslation(
            dragStartTransformation.extractTranslation()));
    gizmo->setArc(dragRing, dragU, dragV, dragStartAngle, dragSweep);
    return true;
}

bool RotateGizmoInteractionTechnique::processMouseWheelEvent(
    const MouseEvent&)
{
    return false;
}

void RotateGizmoInteractionTechnique::beginGesture(int ring, int x, int y)
{
    dragRing = ring;
    dragStartTransformation = gizmo->getTransformationMatrix();
    if ( ring == RotateGizmo::CAMERA_RING_INDEX ) {
        dragAxis = gizmo->getCameraAxisDirection();
        dragU = gizmo->getCameraPlaneRightDirection();
        dragV = gizmo->getCameraPlaneUpDirection();
        gizmo->setPersistentSelection(RotateGizmo::CAMERA_RING_GROUP);
    }
    else {
        dragAxis = gizmo->getAxisDirection(ring);
        dragU = gizmo->getAxisDirection((ring + 1) % RotateGizmo::RING_COUNT);
        dragV = gizmo->getAxisDirection((ring + 2) % RotateGizmo::RING_COUNT);
        gizmo->setPersistentSelection(RotateGizmo::groupOfRing(ring));
    }
    dragStartAngle = 0;
    dragLastAngle = NAN;
    dragSweep = 0;
    gizmo->clearArc();

    // NaN if the ring is seen edge on: the first angle is taken when dragging
    double angle = calculateAngle(x, y);

    if ( !std::isnan(angle) ) {
        dragStartAngle = angle;
        dragLastAngle = angle;
    }
}

void RotateGizmoInteractionTechnique::endGesture()
{
    dragRing = -1;
    dragViewport = nullptr;
    dragStartTransformation = Matrix4x4d();
    dragU = Vector3Dd();
    dragV = Vector3Dd();
    dragAxis = Vector3Dd();
    dragStartAngle = 0;
    dragLastAngle = NAN;
    dragSweep = 0;
    gizmo->clearArc();
}

double RotateGizmoInteractionTechnique::calculateAngle(int x, int y)
{
    Camera* camera = gizmo->getCamera();

    camera->updateVectors();
    Ray ray = camera->generateRay(x, y);
    Vector3Dd direction = ray.getDirection().normalized();
    double cosine = direction.dotProduct(dragAxis);

    if ( std::fabs(cosine) < MIN_RAY_TO_AXIS_COSINE ) {
        return NAN;
    }

    Vector3Dd center = dragStartTransformation.extractTranslation();
    double distance = center.subtract(ray.getOrigin()).dotProduct(dragAxis) /
        cosine;

    // The rays of an orthogonal camera start at a plane, and the gizmo can
    // be behind it
    if ( distance <= 0 &&
         camera->getProjectionMode() != Camera::PROJECTION_MODE_ORTHOGONAL ) {
        return NAN;
    }

    Vector3Dd fromCenter = ray.getOrigin().add(
        direction.multiply(distance)).subtract(center);

    return std::atan2(fromCenter.dotProduct(dragV),
                      fromCenter.dotProduct(dragU));
}

int RotateGizmoInteractionTechnique::calculateSelection(int x, int y)
{
    Camera* camera = gizmo->getCamera();

    gizmo->updateGeometryState();
    camera->updateVectors();
    Ray ray = camera->generateRay(x, y);
    int selection = gizmo->pickRing(ray);

    active = selection != RotateGizmo::NULL_GROUP;
    return selection;
}
