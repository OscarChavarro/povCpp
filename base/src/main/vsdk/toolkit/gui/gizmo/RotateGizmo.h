#ifndef __ROTATE_GIZMO__
#define __ROTATE_GIZMO__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/gizmo/Gizmo.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"

class Camera;
class Ray;
class SimpleBody;
class Torus;
class ViewportElementScaler;

/**
Gizmo to specify the orientation of an object, made of three rings, one around
each axis of the frame of the gizmo (in the style of the rotation gizmo of
3D Studio Max, and following the structure of `TranslateGizmo`), plus a fourth
ring around the front vector of the camera that views the gizmo.

- Geometric model: each ring has an internal model based on a `Torus` (a
  `SimpleBody` of the ring, which owns it), used to interact with the mouse:
  the tube of the torus is a little thicker than the drawn ring, so it is
  easy to point at.
- Size: like `TranslateGizmo`, the gizmo keeps an apparent size in pixels
  from its camera, and its base size and line widths are designed for legacy
  resolutions and scaled for the one of the screen (see `applyScale`).
- Drawing: rings are drawn as strips of quads facing the camera (see
  `buildRingStrips`), so each ring can have its own width in pixels.
- Visibility: the three axis rings only show (and can only be picked on) the
  half closer to the camera, relative to a sphere that contains them (see
  `isPointOnVisibleHemisphere`), unless the ring is seen almost face on (its
  axis nearly aligned with the view direction), when it is shown whole, like
  the camera ring always is (see `buildCameraRingStrip`).
- Camera ring: a fourth ring, around the front vector of the camera that
  views the gizmo, drawn gray and slightly larger than the other three (see
  `getCameraRingRadius`). Dragging it rotates the object around the current
  view direction. It never hides a half, as it always faces the camera.
- Selection: the ring under the cursor is the volatile selection and the one
  clicked is the persistent selection (see `RotateGizmoInteractionTechnique`).
  Rings are drawn with the color of their axis (or gray for the camera ring),
  unless they are the current selection (see `getCurrentSelection`): then
  they are yellow.
- Rotation arc: while a ring is being dragged, the gizmo keeps the arc the
  rotation has swept (see `setArc`), to be drawn as a translucent sector with
  the color of its ring (never yellow), and labeled with its angle.
- Numeric input: the angles (in degrees, with two decimals) of the
  orientation of the gizmo are shown and edited with a nested `InputGizmo`.
  The first three are the angles of the rotations around the X, Y and Z
  axes, applied in that order, that give the orientation (see
  `extractAnglesInDegrees`). The fourth (gray) field is not part of that
  orientation: it always shows 0 and lets the user type a relative angle
  that, on ENTER, rotates the object that much around the current camera
  axis and goes back to 0.

The transformation matrix of the gizmo has the orientation and the position
of the frame the rings belong to. The camera is referenced, not owned.
*/
class RotateGizmo : public Gizmo {
public:
    static const int NULL_GROUP = 0;
    static const int X_RING_GROUP = 1;
    static const int Y_RING_GROUP = 2;
    static const int Z_RING_GROUP = 3;
    static const int CAMERA_RING_GROUP = 4;

    /// Number of colored axis rings (X, Y, Z), and of their input gizmo fields
    static const int RING_COUNT = 3;
    /// Index of the camera ring where a ring index is expected (i.e. in
    /// `setArc`, or the `ring` argument of `RotateGizmoInteractionTechnique`)
    static const int CAMERA_RING_INDEX = RING_COUNT;
    /// Number of input gizmo fields, the axis ones plus the camera one
    static const int INPUT_FIELD_COUNT = RING_COUNT + 1;
    /// Index of the (always 0, relative) field of the camera ring
    static const int CAMERA_INPUT_FIELD_INDEX = RING_COUNT;

    /// Size and line width designed for legacy resolutions; the width
    /// matches TranslateGizmo::DEFAULT_LINE_WIDTH so both gizmos look
    /// visually consistent (same ribbon thickness) in the 3D UI
    static const int DEFAULT_APPARENT_SIZE_IN_PIXELS = 100;
    static const double DEFAULT_LINE_WIDTH;

    /// Decimals of the angles of the input gizmo, and maximum digits of the
    /// integer part (-180 to 180)
    static const int ANGLE_DECIMALS = 2;
    static const int ANGLE_INTEGER_DIGITS = 3;

    /// Number of straight pieces of the strip of a ring
    static const int RING_SEGMENTS = 96;

    /// Maximum angle of each triangle of the sector that shows the rotation
    /// arc
    static const double ARC_STEP_IN_DEGREES;

private:
    Matrix4x4d transformationMatrix;
    Camera* camera;

    /// Geometric model based in primitive instancing: one torus for each ring
    Torus* ringModels[RING_COUNT];
    SimpleBody* ringInstances[RING_COUNT];
    ColorRgb axisColors[RING_COUNT];
    Torus* cameraRingModel;
    SimpleBody* cameraRingInstance;

    /// Apparent size (in legacy resolution pixels) the user has chosen
    int baseApparentSizeInPixels;
    /// Apparent size in pixels of the screen currently in use
    int apparentSizeInPixels;
    double baseRingLineWidths[RING_COUNT];
    double ringLineWidths[RING_COUNT];
    double baseCameraRingLineWidth;
    double cameraRingLineWidth;
    double currentScale;

    /// Rotation arc: ring dragged (-1 if none, `CAMERA_RING_INDEX` for the
    /// camera ring), the axes of its plane (fixed while dragging) and the
    /// angles, in radians, where the arc starts and how much it goes on
    /// (negative for the other way)
    int arcRing;
    Vector3Dd arcU;
    Vector3Dd arcV;
    double arcStartAngle;
    double arcSweep;

    /// Interaction state
    InputGizmo inputGizmo;
    int persistentSelection;
    int volatileSelection;

    void updateScale();
    bool needsHemisphereClipping(const Vector3Dd& axis,
                                 const Vector3Dd& center) const;
    double visibilityMetric(const Vector3Dd& point,
                            const Vector3Dd& center) const;
    java::ArrayList<Vector3Dd> buildFullRingStrip(const Vector3Dd& center,
        const Vector3Dd& axis, const Vector3Dd& u, const Vector3Dd& v,
        double radius, double halfWidth) const;
    java::ArrayList<java::ArrayList<Vector3Dd> > buildVisibleRingArcs(
        const Vector3Dd& center, const Vector3Dd& axis, const Vector3Dd& u,
        const Vector3Dd& v, double radius, double halfWidth) const;
    static Vector3Dd interpolateOnCircle(const Vector3Dd& center,
        const Vector3Dd& a, const Vector3Dd& b, double metricA,
        double metricB, double radius);
    java::ArrayList<Vector3Dd> toRibbonStrip(
        const java::ArrayList<Vector3Dd>& polyline, const Vector3Dd& axis,
        double halfWidth) const;
    Vector3Dd sideAcross(const Vector3Dd& point, const Vector3Dd& tangent,
                         const Vector3Dd& axis, double halfWidth) const;
    static Vector3Dd unitAxis(int axis);
    static Matrix4x4d rotationFromBasis(const Vector3Dd& xAxis,
        const Vector3Dd& yAxis, const Vector3Dd& zAxis);
    double arcRingRadius() const;
    Vector3Dd pointOfArc(double angle, double radiusFactor) const;
    static void checkRing(int ring);
    static void checkRingOrCamera(int ring);

    RotateGizmo(const RotateGizmo& other);
    RotateGizmo& operator=(const RotateGizmo& other);

public:
    /**
    @param cam camera the gizmo is seen from
    */
    explicit RotateGizmo(Camera* cam);
    virtual ~RotateGizmo();

    //= Camera, size and line widths ======================================
    void setCamera(Camera* cam);
    Camera* getCamera() const;
    int getApparentSizeInPixels() const;
    void setApparentSizeInPixels(int size);
    int getBaseApparentSizeInPixels() const;
    void setBaseApparentSizeInPixels(int size);
    double getRingLineWidth(int ring) const;
    void setRingLineWidth(int ring, double lineWidth);
    double getBaseRingLineWidth(int ring) const;
    void setBaseRingLineWidth(int ring, double lineWidth);
    double getCameraRingLineWidth() const;
    void setCameraRingLineWidth(double lineWidth);
    double getBaseCameraRingLineWidth() const;
    void setBaseCameraRingLineWidth(double lineWidth);

    /**
    Sets the apparent size and the line widths of the gizmo to the values
    that make it look proportional to the screen resolution: the base values
    (designed for legacy resolutions) multiplied by the scale of the given
    scaler. The new size is used by the gizmo the next time its
    transformation is set.
    */
    void applyScale(const ViewportElementScaler* scaler);
    double getCurrentScale() const;
    double getRingLineWidthInWorldUnits(int ring) const;
    double getCameraRingLineWidthInWorldUnits() const;
    double getRingRadius() const;
    double getCameraRingRadius() const;

    //= Transformation ====================================================
    Vector3Dd getPosition() const;
    void setPosition(const Vector3Dd& p);

    /**
    Sets the frame of the gizmo and recalculates the geometry of its rings.
    @param transformationMatrix orientation and position of the frame the
    rings belong to
    */
    void setTransformationMatrix(const Matrix4x4d& transformationMatrix);
    Matrix4x4d getTransformationMatrix() const;

    /**
    Recalculates the geometry of the rings of the gizmo from its current
    transformation, apparent size and camera.
    */
    void updateGeometryState();

    //= Geometry of the rings =============================================
    Torus* getRingModel(int ring) const;
    SimpleBody* getRingInstance(int ring) const;
    Torus* getCameraRingModel() const;
    SimpleBody* getCameraRingInstance() const;

    /**
    @return the direction, in world space, of the axis of the ring
    */
    Vector3Dd getAxisDirection(int ring) const;

    /**
    @return the direction, in world space, of the axis of the camera ring:
    the front vector of the camera that views the gizmo
    PRE: `camera->updateVectors()` has been called
    */
    Vector3Dd getCameraAxisDirection() const;

    /**
    @return the direction, in world space, that goes across the screen to the
    right, as seen from the camera of the gizmo; one of the two axes of the
    plane of the camera ring, built as a proper (determinant +1) orthonormal
    basis with the camera axis (see the Java version for details)
    PRE: `camera->updateVectors()` has been called
    */
    Vector3Dd getCameraPlaneRightDirection() const;

    /**
    @return the direction, in world space, that goes up the screen, as seen
    from the camera of the gizmo; the other axis of the plane of the camera
    ring (see `getCameraPlaneRightDirection`)
    PRE: `camera->updateVectors()` has been called
    */
    Vector3Dd getCameraPlaneUpDirection() const;

    /**
    @return true if the point is on the half of the sphere (centered at the
    position of the gizmo) that faces the camera, so an axis ring is only
    drawn and picked where this is true
    */
    bool isPointOnVisibleHemisphere(const Vector3Dd& point) const;

    /**
    Builds the geometry to draw a ring as one or more strips of quads facing
    the camera of the gizmo. Only the half closer to the camera is returned,
    unless the ring is seen almost face on, when it is returned whole.
    PRE: the transformation matrix of the gizmo has been set.
    @return the vertices of one or more triangle strips (an alternating
    pair, one at each side of the ring, for each position along it)
    */
    java::ArrayList<java::ArrayList<Vector3Dd> > buildRingStrips(int ring) const;

    /**
    Builds the geometry to draw the camera ring as a closed triangle strip in
    world space, that follows the ring with its width in pixels at every
    vertex. It is always whole, as it always faces the camera.
    PRE: the transformation matrix of the gizmo has been set.
    */
    java::ArrayList<Vector3Dd> buildCameraRingStrip() const;

    //= Selection =========================================================

    /**
    Finds the ring (or the camera ring) a ray points at, using the torus
    models of the rings, and ignoring hits on the far half of an axis ring
    that needs clipping.
    PRE: the transformation matrix of the gizmo has been set.
    @return the `*_RING_GROUP` constant of the nearest ring hit by the ray,
    or `NULL_GROUP` if it hits none
    */
    int pickRing(const Ray& ray) const;
    static int groupOfRing(int ring);
    int getPersistentSelection() const;
    void setPersistentSelection(int selection);
    int getVolatileSelection() const;
    void setVolatileSelection(int selection);
    int getCurrentSelection() const;
    bool isRingHighlighted(int ring) const;
    bool isCameraRingHighlighted() const;
    ColorRgb getRingColor(int ring) const;
    ColorRgb getCameraRingColor() const;

    //= Rotation arc ======================================================

    /**
    Sets the arc of the rotation being done around a ring, so it is shown.
    @param ring 0, 1 or 2 for the ring around the X, Y or Z axis, or
    `CAMERA_RING_INDEX` for the camera ring
    @param u direction, in world space, where the angles are measured from
    @param v direction, in world space, perpendicular to `u` and to the
    axis of the ring, so angles grow from `u` towards it
    @param startAngle angle, in radians, where the arc starts
    @param sweep angle, in radians, the arc goes on from its start
    */
    void setArc(int ring, const Vector3Dd& u, const Vector3Dd& v,
                double startAngle, double sweep);
    void clearArc();
    bool isArcVisible() const;
    int getArcRing() const;
    double getArcSweep() const;
    double getArcSweepInDegrees() const;
    ColorRgb getArcColor() const;

    /**
    Builds the geometry to draw the arc as a translucent sector: a triangle
    fan in world space, with the center of the gizmo as its first vertex and
    then the points of the arc at the radius of its ring, from its start.
    An arc of more than a turn is shown as a whole disc.
    @return the vertices of the triangle fan, or an empty list if there is
    no arc to show, or it has no angle yet
    */
    java::ArrayList<Vector3Dd> buildArcFan() const;

    /**
    @param outPosition the point, in world space, where the label with the
    angle of the arc must be anchored: just outside the ring, at the middle
    of the arc
    @return false if there is no arc to show (the Java version returns null)
    */
    bool getArcLabelPosition(Vector3Dd* outPosition) const;

    //= Numeric input =====================================================

    /**
    @return the input gizmo that shows (and lets the user type) the angles,
    in degrees, of this gizmo, updated with its current orientation and
    highlighted rings
    */
    InputGizmo* getInputGizmo();

    /**
    Builds the rotation given by three angles: a rotation around the X axis,
    then one around the Y axis and then one around the Z axis, all of them
    around the axes of the world.
    */
    static Matrix4x4d createRotationFromAnglesInDegrees(double xDegrees,
                                                        double yDegrees,
                                                        double zDegrees);

    /**
    Inverse of `createRotationFromAnglesInDegrees`: the angles of the
    rotation around the X, Y and Z axes that give an orientation. The angle
    around Y is in [-90, 90] and the other ones are in (-180, 180]. When the
    Y rotation is +-90 degrees there are infinite solutions (gimbal lock) and
    the rotation around Z is taken as 0.
    @param rotation rotation matrix (its translation is ignored)
    @param outAngles the angles, in degrees, around the X, Y and Z axes
    */
    static void extractAnglesInDegrees(const Matrix4x4d& rotation,
                                       double outAngles[3]);
};

#endif
