#ifndef __TRANSLATE_GIZMO__
#define __TRANSLATE_GIZMO__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/gizmo/Gizmo.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"
#include "vsdk/toolkit/gui/gizmo/TranslateGizmoLineSegment.h"

class Arrow;
class Box;
class Camera;
class Cone;
class ParametricCurve;
class SimpleBody;
class SimpleMaterial;
class ViewportElementScaler;

/**
Translation gizmo. Its geometric model is based in primitive instancing:
the primitive concretions (arrow, cylinder, box, cone) are owned by the
gizmo and referenced (not owned) by the element bodies (see
`SimpleBody::setGeometryReference`). The camera is referenced, not owned.
*/
class TranslateGizmo : public Gizmo {
public:
    /// Internal element selection state
    static const int X_AXIS_ELEMENT = 1;
    static const int Y_AXIS_ELEMENT = 2;
    static const int Z_AXIS_ELEMENT = 3;
    static const int XYY_SEGMENT_ELEMENT = 4;
    static const int XYX_SEGMENT_ELEMENT = 5;
    static const int YZZ_SEGMENT_ELEMENT = 6;
    static const int YZY_SEGMENT_ELEMENT = 7;
    static const int XZZ_SEGMENT_ELEMENT = 8;
    static const int XZX_SEGMENT_ELEMENT = 9;
    static const int XY_BOX_ELEMENT = 10;
    static const int YZ_BOX_ELEMENT = 11;
    static const int XZ_BOX_ELEMENT = 12;

    static const int NULL_GROUP = 0;
    static const int X_AXIS_GROUP = 1;
    static const int Y_AXIS_GROUP = 2;
    static const int Z_AXIS_GROUP = 3;
    static const int XY_PLANE_GROUP = 4;
    static const int YZ_PLANE_GROUP = 5;
    static const int XZ_PLANE_GROUP = 6;

    /// Size and line width designed for legacy resolutions
    static const int DEFAULT_APPARENT_SIZE_IN_PIXELS = 100;
    static const double DEFAULT_LINE_WIDTH;

private:
    /// Internal transformation state
    Matrix4x4d transformationMatrix;
    bool hasTransformationMatrix;
    Camera* camera;

    /// Geometric model based in primitive instancing: primitive concretions
    Arrow* arrowModel;
    Cone* cylinderModel;
    Box* boxModel;
    Cone* coneModel;
    ParametricCurve* lineModel3dsmax;
    ParametricCurve* segmentModel3dsmax;

    /// Geometric model based in primitive instancing: primitive instances
    /// This list is always of size 12, and its elements follow the order
    /// indicated in the values of the *_ELEMENT constants of this class.
    java::ArrayList<SimpleBody*> elements;
    java::ArrayList<SimpleBody*> elementInstances3dsmax;

    /// Apparent size (in legacy resolution pixels) the user has chosen
    int baseApparentSizeInPixels;
    /// Apparent size in pixels of the screen currently in use
    int apparentSizeInPixels;
    /// Width, in pixels, of the lines of the gizmo
    double lineWidth;

    /// Interaction state
    InputGizmo inputGizmo;
    int persistentSelection;
    int volatileSelection;

    bool selectedResizing;
    double currentScale;

    static SimpleMaterial createMaterial(double r, double g, double b);
    void placeElement(SimpleBody* r, const Matrix4x4d& rotation,
                      const Matrix4x4d& subR, const Vector3Dd& subP,
                      bool positionFromElementRotation) const;

    TranslateGizmo(const TranslateGizmo& other);
    TranslateGizmo& operator=(const TranslateGizmo& other);

public:
    explicit TranslateGizmo(Camera* cam);
    virtual ~TranslateGizmo();

    int getApparentSizeInPixels() const;
    void setApparentSizeInPixels(int du);

    /**
    @return the apparent size the gizmo is designed to have in legacy
    resolutions, in pixels; it is the size chosen by the user, before scaling
    it for the resolution of the screen
    */
    int getBaseApparentSizeInPixels() const;

    /**
    @param size apparent size of the gizmo in legacy resolutions, in pixels;
    not positive values are ignored. It is used the next time `applyScale`
    is called
    */
    void setBaseApparentSizeInPixels(int size);
    double getLineWidth() const;

    /**
    @param lineWidth width, in pixels, of the lines that draw the gizmo; not
    positive values are ignored
    */
    void setLineWidth(double lineWidth);

    /**
    Sets the apparent size and the line width of the gizmo to the values
    that make it look proportional to the screen resolution: the base values
    (designed for legacy resolutions) multiplied by the scale of the given
    scaler. The new size is used by the gizmo the next time its
    transformation is set.
    */
    void applyScale(const ViewportElementScaler* scaler);

    /**
    @return the width of the lines of the gizmo, converted from pixels to
    world units, as seen from its camera at its current apparent size
    */
    double getLineWidthInWorldUnits() const;

    /**
    Gives the straight lines drawn by the gizmo: the shaft of each axis arrow
    (from the gap around the origin up to the base of its head) and the
    segments that delimit the plane handles. Lines hidden because they point
    to the viewer of an orthogonal camera are not included, and the color of
    the lines of the selected group is yellow.
    PRE: the transformation matrix of the gizmo has been set.
    @return the lines of the gizmo, in world space
    */
    java::ArrayList<TranslateGizmoLineSegment> getLineSegments() const;

    /**
    Builds the geometry to draw a line of the gizmo as a line with thickness
    seen from the camera of the gizmo: a triangle strip (a rectangle of 4
    vertices, in strip order) in world space, facing the camera. Its width is
    `getLineWidth()` pixels, and it is extended half of that width at both
    ends (square caps), so lines that meet at a corner have no gaps.
    @param segment line to draw, as given by `getLineSegments()`
    @param outStrip the 4 vertices of the triangle strip
    @return false if the line has no length (the Java version returns null)
    */
    bool buildLineStrip(const TranslateGizmoLineSegment& segment,
                        Vector3Dd outStrip[4]) const;

    /**
    @return the factor applied to the gizmo geometry so it keeps its apparent
    size in pixels, as seen from its camera
    */
    double getCurrentScale() const;
    void setCamera(Camera* cam);
    Camera* getCamera() const;
    java::ArrayList<SimpleBody*>& getElements();
    java::ArrayList<SimpleBody*>& getElements3dsmax();

    /**
    This method updates the data structure contained in the `elements`
    array starting from the given parameters.
    - translation is the position of the center of the gizmo
    - rotation is the rotation matrix containing the orientation of the gizmo
    - if autosize is false, initialdu and camera parameters are not used, and
      the gizmo doesn't change its current size. If autosize is true, the
      gizmo size is changed such as from the current camera, the gizmo
      projection fit a 2D area of initialdu * initialdv pixels.
    */
    void calculateGeometryState(const Vector3Dd& translation,
                                const Matrix4x4d& rotation, bool autosize,
                                int initialdu, Camera* camera);
    Vector3Dd getPosition() const;
    void setPosition(const Vector3Dd& p);
    void setTransformationMatrix(const Matrix4x4d& transformationMatrix);
    Matrix4x4d getTransformationMatrix() const;

    /**
    Recalculates the geometry of the elements of the gizmo from its current
    transformation, apparent size, resizing state, selection and camera.
    PRE: the transformation matrix of the gizmo has been set.
    */
    void updateGeometryState();

    /**
    @return the selected group (one of the `*_GROUP` constants) chosen by the
    user with a click, or `NULL_GROUP`
    */
    int getPersistentSelection() const;
    void setPersistentSelection(int selection);

    /**
    @return the group (one of the `*_GROUP` constants) currently under the
    cursor, or `NULL_GROUP`
    */
    int getVolatileSelection() const;
    void setVolatileSelection(int selection);

    /**
    @return the group (one of the `*_GROUP` constants) that is highlighted
    and manipulated: the one under the cursor, or the chosen one if the
    cursor is not over any group
    */
    int getCurrentSelection() const;

    /**
    @return the input gizmo that shows (and lets the user type) the
    coordinates of this gizmo, updated with its current position and
    highlighted axes
    */
    InputGizmo* getInputGizmo();

    /**
    @param axis 0, 1 or 2 for the X, Y or Z axis
    @return true if the axis is highlighted (drawn yellow) because the group
    currently selected moves along it: the axis itself, or a plane that
    contains it
    */
    bool isAxisHighlighted(int axis) const;

    /**
    @return true if the size of the gizmo is recalculated to keep its
    apparent size in pixels, false if it is kept while it is being dragged
    */
    bool isSelectedResizing() const;
    void setSelectedResizing(bool selectedResizing);
};

#endif
