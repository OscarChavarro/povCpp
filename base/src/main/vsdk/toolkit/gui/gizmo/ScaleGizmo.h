#ifndef __SCALE_GIZMO__
#define __SCALE_GIZMO__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"
#include "vsdk/toolkit/gui/gizmo/Gizmo.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"

class Box;
class Camera;
class Cone;
class Ray;
class SimpleBody;
class SimpleMaterial;
class ViewportElementScaler;

/**
Gizmo to specify the scale of an object along the X, Y and Z axes of its
frame (in the style of 3ds Max's scale gizmo, and following the structure of
`TranslateGizmo` and `RotateGizmo`).

- Geometric model: each axis is a straight cylinder (see `getElements`) from
  the origin of the frame out along it, capped by a small cube at its tip
  (both easy to point at with the cursor). The primitives are owned by the
  gizmo and referenced by the element bodies.
- Two-axis handles: for each pair of axes, a flat trapezoidal band (a quad,
  see `buildBandQuad`) contained in the plane those two axes span (XY, YZ or
  XZ). Picking one scales both of its axes (see `XY_GROUP`, `YZ_GROUP`,
  `XZ_GROUP`).
- Uniform (all-axis) handle: three triangles (see `buildUniformTriangles`),
  one in each of those same planes, all of them sharing the vertex at the
  origin of the frame and reaching the inner side of the bands, which they
  are flush with. Picking any of them scales the three axes together (see
  `UNIFORM_GROUP`).
- Size: like `TranslateGizmo` and `RotateGizmo`, the gizmo keeps an apparent
  size in pixels from its camera, scaled for the resolution of the screen
  (see `applyScale`).
- Selection: the handle under the cursor is the volatile selection and the
  one clicked is the persistent selection (see `pickElement`,
  `ScaleGizmoInteractionTechnique`). The axes the current selection scales
  are drawn yellow instead of with their own color.
- Flat handles are not drawn as surfaces: only their contour is (see
  `buildContourSegments`), with each of its edges split in two halves, each
  one with the color of the axis it ends at. The handle that is the current
  selection draws its contour yellow, and only it fills its interior, with a
  translucent gray (see `HANDLE_FILL_COLOR`).
- Numeric input: the scale factors (see `getScale`) are shown and edited with
  a nested `InputGizmo` (see `getInputGizmo`), stepped by
  `InputGizmoValueChangeRules::forScale()`.
- Letter-driven scaling: `x`/`X`, `y`/`Y` and `z`/`Z` shrink/grow a single
  axis, and the arrow keys (when the input gizmo does not consume them)
  shrink/grow every axis uniformly.

The transformation matrix of the gizmo has the orientation and the position
of the frame its handles belong to; the scale factors themselves are kept
apart (see `getScale`). The camera is referenced, not owned.
*/
class ScaleGizmo : public Gizmo {
public:
    /// Number of axes, and of the values of the input gizmo
    static const int AXIS_COUNT = 3;

    /// Selection groups
    static const int NULL_GROUP = 0;
    static const int X_AXIS_GROUP = 1;
    static const int Y_AXIS_GROUP = 2;
    static const int Z_AXIS_GROUP = 3;
    /// Two-axis handle: the trapezoidal band in the XY plane
    static const int XY_GROUP = 4;
    /// Two-axis handle: the trapezoidal band in the YZ plane
    static const int YZ_GROUP = 5;
    /// Two-axis handle: the trapezoidal band in the XZ plane
    static const int XZ_GROUP = 6;
    /// All-axis handle: the three triangles that share the origin of the
    /// frame, one in each of the XY, YZ and XZ planes
    static const int UNIFORM_GROUP = 7;

    /// The two-axis groups, in the order `buildBandQuad` and the renderers
    /// walk them
    static const int BAND_GROUPS[3];
    static const int BAND_GROUPS_COUNT = 3;

    /// Size and contour width designed for legacy resolutions
    static const int DEFAULT_APPARENT_SIZE_IN_PIXELS = 100;
    static const double DEFAULT_LINE_WIDTH;

    /// Color the interior of the handle currently selected is filled with
    static const ColorRgb HANDLE_FILL_COLOR;

    /**
    A straight piece of the contour of the flat handles, in world space, with
    the color it must be drawn with.
    */
    class ContourSegment {
    private:
        Vector3Dd startPoint;
        Vector3Dd endPoint;
        ColorRgb segmentColor;

    public:
        ContourSegment() {}
        ContourSegment(const Vector3Dd& start, const Vector3Dd& end,
                       const ColorRgb& color)
            : startPoint(start), endPoint(end), segmentColor(color) {}
        const Vector3Dd& start() const { return startPoint; }
        const Vector3Dd& end() const { return endPoint; }
        const ColorRgb& color() const { return segmentColor; }
    };

private:
    Matrix4x4d transformationMatrix;
    Camera* camera;

    /// Scale factors shown (and edited) by the gizmo; kept apart from
    /// `transformationMatrix`, which only carries the orientation and
    /// position of its frame
    Vector3Dd scale;

    /// Apparent size (in legacy resolution pixels) the user has chosen
    int baseApparentSizeInPixels;
    /// Apparent size in pixels of the screen currently in use
    int apparentSizeInPixels;
    /// Width, in pixels, of the contour of the flat handles
    double lineWidth;
    double currentScale;

    /// Geometric model based in primitive instancing: primitive concretions.
    Cone* shaftModel;
    Box* tipModel;

    /// Geometric model based in primitive instancing: primitive instances,
    /// always of size 6, in the order given by the `*_ELEMENT` constants
    java::ArrayList<SimpleBody*> elements;

    ColorRgb axisColors[AXIS_COUNT];

    /// Interaction state
    InputGizmo inputGizmo;
    int persistentSelection;
    int volatileSelection;

    void init(Camera* cam);
    void updateScale();
    void addContourEdge(java::ArrayList<ContourSegment>& segments,
                        const Vector3Dd& a, const Vector3Dd& b,
                        const int axes[2], bool highlighted) const;
    static int groupOfElement(int element);
    void calculateGeometryState();
    static SimpleMaterial createMaterial(const ColorRgb& c);
    static Matrix4x4d axisTilt(int axis);
    static bool nearestOfTwoTriangles(const Ray& ray,
        const Vector3Dd& a1, const Vector3Dd& b1, const Vector3Dd& c1,
        const Vector3Dd& a2, const Vector3Dd& b2, const Vector3Dd& c2,
        double* outT);
    static bool rayTriangleT(const Vector3Dd& origin, const Vector3Dd& dir,
        const Vector3Dd& a, const Vector3Dd& b, const Vector3Dd& c,
        double* outT);
    static Vector3Dd unitAxis(int axis);
    static void checkAxis(int axis);

    ScaleGizmo(const ScaleGizmo& other);
    ScaleGizmo& operator=(const ScaleGizmo& other);

public:
    /**
    @param cam camera the gizmo is seen from, or null if it is not known yet
    (see `setCamera`)
    */
    explicit ScaleGizmo(Camera* cam);

    /**
    Creates a gizmo with no camera yet (see `setCamera`); its apparent size
    can not be computed until one is set.
    */
    ScaleGizmo();
    virtual ~ScaleGizmo();

    //= Camera and size ===================================================
    void setCamera(Camera* cam);
    Camera* getCamera() const;
    int getApparentSizeInPixels() const;
    void setApparentSizeInPixels(int size);
    int getBaseApparentSizeInPixels() const;
    void setBaseApparentSizeInPixels(int size);
    double getLineWidth() const;
    void setLineWidth(double lineWidth);

    /**
    Sets the apparent size and the contour width of the gizmo to the values
    that make it look proportional to the screen resolution. The new size is
    used the next time the geometry state is updated.
    */
    void applyScale(const ViewportElementScaler* scaler);
    double getCurrentScale() const;

    /**
    @return the width of the shaft of the axes and of the contour of the flat
    handles, converted from pixels to world units, as seen from its camera at
    its current apparent size
    */
    double getLineWidthInWorldUnits() const;

    //= Transformation and scale factors ===================================
    Vector3Dd getPosition() const;
    void setPosition(const Vector3Dd& p);

    /**
    Sets the frame (orientation and position) of the gizmo and recalculates
    its geometry.
    */
    void setTransformationMatrix(const Matrix4x4d& transformationMatrix);
    Matrix4x4d getTransformationMatrix() const;

    /**
    @return the scale factors shown (and edited) by the gizmo, along the X, Y
    and Z axes of its frame
    */
    Vector3Dd getScale() const;
    void setScale(const Vector3Dd& scale);

    /**
    Recalculates the apparent size and the geometry of the gizmo from its
    current transformation, apparent size, selection and camera. The size is
    kept as it was if it can not be measured, or there is no camera yet.
    */
    void updateGeometryState();

    //= Selection ==========================================================
    int getPersistentSelection() const;
    void setPersistentSelection(int selection);
    int getVolatileSelection() const;
    void setVolatileSelection(int selection);
    int getCurrentSelection() const;

    /**
    @return true if scaling with `group` changes `axis`
    */
    static bool groupIncludesAxis(int group, int axis);
    bool isAxisHighlighted(int axis) const;
    bool isBandHighlighted(int group) const;
    bool isUniformHighlighted() const;
    ColorRgb getAxisDisplayColor(int axis) const;

    /**
    Gives the contour of the flat handles, which are drawn as outlines and
    not as surfaces: the inner and the outer edge of each two-axis band.
    Each edge is split at its middle point, so each half takes the color of
    the axis it ends at, unless the handle it belongs to is the current
    selection: then the whole edge is yellow.
    PRE: the transformation matrix of the gizmo has been set.
    */
    java::ArrayList<ContourSegment> buildContourSegments() const;

    /**
    Finds the handle a ray points at, using the geometric model of the
    gizmo: the axis cylinders and cubes, the two-axis bands and the
    triangles of the uniform handle.
    @return the `*_GROUP` constant of the nearest handle hit by the ray, or
    `NULL_GROUP` if it hits none
    */
    int pickElement(const Ray& ray) const;

    //= Geometry ===========================================================
    java::ArrayList<SimpleBody*>& getElements();
    Vector3Dd getAxisDirection(int axis) const;

    /**
    @return the position, in world space, of the tip (the center of its
    cube) of the axis
    */
    Vector3Dd getTipPosition(int axis) const;

    /**
    @return the point of the axis at that fraction of its length, in world
    space
    */
    Vector3Dd axisPoint(int axis, double reach) const;

    /**
    @param group `XY_GROUP`, `YZ_GROUP` or `XZ_GROUP`
    @param outAxes the two axes (0, 1 or 2) whose plane contains the handles
    of the group
    */
    static void bandAxes(int group, int outAxes[2]);

    /**
    Gives the trapezoidal band of a two-axis handle.
    @param outQuad the 4 vertices of the quad, in world space, in the order
    of a triangle strip (inner and outer point of the first axis, then the
    ones of the second one)
    */
    void buildBandQuad(int group, Vector3Dd outQuad[4]) const;

    /**
    Gives the three triangles of the uniform (all-axis) handle.
    @return the 9 vertices (3 consecutive per triangle), in world space
    */
    java::ArrayList<Vector3Dd> buildUniformTriangles() const;

    //= Numeric input ======================================================
    InputGizmo* getInputGizmo();

    //= Events =============================================================
    bool processMouseEvent(const MouseEvent& mouseEvent);

    /**
    Lets the numeric input gizmo use the key first; if it does not consume
    the key, `x`/`X`, `y`/`Y` and `z`/`Z` shrink/grow a single axis, and the
    arrow keys shrink/grow every axis uniformly.
    @return true if the scale factors changed
    */
    bool processKeyPressedEvent(const KeyEvent& keyEvent);
    bool processKeyReleasedEvent(const KeyEvent& keyEvent);
    bool processMousePressedEvent(const MouseEvent& e);
    bool processMouseReleasedEvent(const MouseEvent& e);
    bool processMouseClickedEvent(const MouseEvent& e);
    bool processMouseMovedEvent(const MouseEvent& e);
    bool processMouseDraggedEvent(const MouseEvent& e);
    bool processMouseWheelEvent(const MouseEvent& e);
};

#endif
