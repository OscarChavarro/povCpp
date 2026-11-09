#include <cfloat>
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/gui/gizmo/ReferenceFrameGizmo.h"
#include "vsdk/toolkit/gui/gizmo/ScaleGizmo.h"
#include "vsdk/toolkit/gui/viewport/ViewportElementScaler.h"

const int ScaleGizmo::BAND_GROUPS[3] = {
    ScaleGizmo::XY_GROUP, ScaleGizmo::YZ_GROUP, ScaleGizmo::XZ_GROUP};
const double ScaleGizmo::DEFAULT_LINE_WIDTH = 1.0;
const ColorRgb ScaleGizmo::HANDLE_FILL_COLOR(0.88, 0.88, 0.88);

namespace {
/// Indexes of `elements`, in the order `getElements` gives them
const int X_SHAFT_ELEMENT = 1;
const int X_TIP_ELEMENT = 2;
const int Y_SHAFT_ELEMENT = 3;
const int Y_TIP_ELEMENT = 4;
const int Z_SHAFT_ELEMENT = 5;
const int Z_TIP_ELEMENT = 6;

/// Total length of an axis (shaft and tip together), and shape of its
/// parts, relative to the apparent size of the gizmo. The radius of the
/// shaft is not here: it follows `lineWidth`, like the contour of the
/// flat handles, so both look consistent (see `getLineWidthInWorldUnits`)
const double AXIS_LENGTH = 0.8;
const double TIP_SIZE = 0.06;
/// Fractions of the length of an axis where the parallel sides of a
/// two-axis band meet it. The inner one is also the boundary the uniform
/// handle reaches, so band and uniform handle are flush
const double BAND_INNER_REACH = 0.52;
const double BAND_OUTER_REACH = 0.73;
}

ScaleGizmo::ScaleGizmo(Camera* cam)
    : inputGizmo(AXIS_COUNT, InputGizmo::DECIMALS, 1,
                 InputGizmoValueChangeRules::forScale())
{
    init(cam);
}

ScaleGizmo::ScaleGizmo()
    : inputGizmo(AXIS_COUNT, InputGizmo::DECIMALS, 1,
                 InputGizmoValueChangeRules::forScale())
{
    init(nullptr);
}

void ScaleGizmo::init(Camera* cam)
{
    camera = nullptr;
    scale = Vector3Dd(1, 1, 1);
    baseApparentSizeInPixels = DEFAULT_APPARENT_SIZE_IN_PIXELS;
    apparentSizeInPixels = DEFAULT_APPARENT_SIZE_IN_PIXELS;
    lineWidth = DEFAULT_LINE_WIDTH;
    currentScale = 1.0;
    persistentSelection = NULL_GROUP;
    volatileSelection = NULL_GROUP;

    ReferenceFrameGizmo axisColorSource;

    for ( int axis = 0; axis < AXIS_COUNT; axis++ ) {
        axisColors[axis] = axisColorSource.getAxisColor(axis);
        inputGizmo.setFieldColor(axis, axisColors[axis]);
    }

    double initialShaftRadius = getLineWidthInWorldUnits() / 2;
    shaftModel = new Cone(initialShaftRadius, initialShaftRadius,
                          AXIS_LENGTH - TIP_SIZE);
    tipModel = new Box(TIP_SIZE, TIP_SIZE, TIP_SIZE);

    for ( int i = 0; i < 6; i++ ) {
        elements.add(new SimpleBody());
    }

    setCamera(cam);
}

ScaleGizmo::~ScaleGizmo()
{
    long i;
    for ( i = 0; i < elements.size(); i++ ) {
        delete elements.get(i);
    }
    delete shaftModel;
    delete tipModel;
}

//= Camera and size =======================================================

void ScaleGizmo::setCamera(Camera* cam)
{
    camera = cam;
}

Camera* ScaleGizmo::getCamera() const
{
    return camera;
}

int ScaleGizmo::getApparentSizeInPixels() const
{
    return apparentSizeInPixels;
}

void ScaleGizmo::setApparentSizeInPixels(int size)
{
    apparentSizeInPixels = size;
}

int ScaleGizmo::getBaseApparentSizeInPixels() const
{
    return baseApparentSizeInPixels;
}

void ScaleGizmo::setBaseApparentSizeInPixels(int size)
{
    if ( size > 0 ) {
        baseApparentSizeInPixels = size;
    }
}

double ScaleGizmo::getLineWidth() const
{
    return lineWidth;
}

void ScaleGizmo::setLineWidth(double lineWidth)
{
    if ( lineWidth > 0.0 ) {
        this->lineWidth = lineWidth;
    }
}

void ScaleGizmo::applyScale(const ViewportElementScaler* scaler)
{
    if ( scaler == nullptr ) {
        return;
    }
    setApparentSizeInPixels(scaler->scaleSize(baseApparentSizeInPixels));
    setLineWidth(scaler->scaleLength(DEFAULT_LINE_WIDTH));
}

double ScaleGizmo::getCurrentScale() const
{
    return currentScale;
}

double ScaleGizmo::getLineWidthInWorldUnits() const
{
    return lineWidth * currentScale / apparentSizeInPixels;
}

//= Transformation and scale factors ======================================

Vector3Dd ScaleGizmo::getPosition() const
{
    return transformationMatrix.extractTranslation();
}

void ScaleGizmo::setPosition(const Vector3Dd& p)
{
    setTransformationMatrix(transformationMatrix.withTranslation(p));
}

void ScaleGizmo::setTransformationMatrix(
    const Matrix4x4d& transformationMatrix)
{
    this->transformationMatrix = transformationMatrix;
    updateGeometryState();
}

Matrix4x4d ScaleGizmo::getTransformationMatrix() const
{
    return transformationMatrix;
}

Vector3Dd ScaleGizmo::getScale() const
{
    return scale;
}

void ScaleGizmo::setScale(const Vector3Dd& scale)
{
    this->scale = scale;
}

void ScaleGizmo::updateGeometryState()
{
    updateScale();
    calculateGeometryState();
}

void ScaleGizmo::updateScale()
{
    if ( camera == nullptr ) {
        return;
    }
    camera->updateVectors();

    Vector3Dd p = getPosition();
    Vector3Dd right = camera->getLeft().multiply(-1).normalized();
    Vector3Dd a;
    Vector3Dd b;

    if ( camera->projectPointUsingRayMethod(p, &a) &&
         camera->projectPointUsingRayMethod(p.add(right), &b) ) {
        double factor = a.subtract(b).length();

        if ( factor > VSDK::EPSILON ) {
            currentScale = ((double)apparentSizeInPixels)/factor;
        }
    }
}

//= Selection =============================================================

int ScaleGizmo::getPersistentSelection() const
{
    return persistentSelection;
}

void ScaleGizmo::setPersistentSelection(int selection)
{
    persistentSelection = selection;
}

int ScaleGizmo::getVolatileSelection() const
{
    return volatileSelection;
}

void ScaleGizmo::setVolatileSelection(int selection)
{
    volatileSelection = selection;
}

int ScaleGizmo::getCurrentSelection() const
{
    if ( volatileSelection == NULL_GROUP ) {
        return persistentSelection;
    }
    return volatileSelection;
}

bool ScaleGizmo::groupIncludesAxis(int group, int axis)
{
    checkAxis(axis);
    switch ( group ) {
      case X_AXIS_GROUP: return axis == 0;
      case Y_AXIS_GROUP: return axis == 1;
      case Z_AXIS_GROUP: return axis == 2;
      case XY_GROUP: return axis == 0 || axis == 1;
      case YZ_GROUP: return axis == 1 || axis == 2;
      case XZ_GROUP: return axis == 0 || axis == 2;
      case UNIFORM_GROUP: return true;
      default: return false;
    }
}

bool ScaleGizmo::isAxisHighlighted(int axis) const
{
    return groupIncludesAxis(getCurrentSelection(), axis);
}

bool ScaleGizmo::isBandHighlighted(int group) const
{
    return getCurrentSelection() == group;
}

bool ScaleGizmo::isUniformHighlighted() const
{
    return getCurrentSelection() == UNIFORM_GROUP;
}

ColorRgb ScaleGizmo::getAxisDisplayColor(int axis) const
{
    checkAxis(axis);
    return isAxisHighlighted(axis) ? InputGizmo::HIGHLIGHT_COLOR :
        axisColors[axis];
}

java::ArrayList<ScaleGizmo::ContourSegment>
ScaleGizmo::buildContourSegments() const
{
    java::ArrayList<ContourSegment> segments;

    for ( int i = 0; i < BAND_GROUPS_COUNT; i++ ) {
        int group = BAND_GROUPS[i];
        int axes[2];
        Vector3Dd quad[4];

        bandAxes(group, axes);
        buildBandQuad(group, quad);

        // The inner edge is shared with the uniform handle, so either of
        // the two being selected highlights it
        addContourEdge(segments, quad[0], quad[2], axes,
            isBandHighlighted(group) || isUniformHighlighted());
        addContourEdge(segments, quad[1], quad[3], axes,
                       isBandHighlighted(group));
    }
    return segments;
}

void ScaleGizmo::addContourEdge(java::ArrayList<ContourSegment>& segments,
                                const Vector3Dd& a, const Vector3Dd& b,
                                const int axes[2], bool highlighted) const
{
    Vector3Dd middle = a.add(b).multiply(0.5);
    ColorRgb colorA = highlighted ? InputGizmo::HIGHLIGHT_COLOR :
        axisColors[axes[0]];
    ColorRgb colorB = highlighted ? InputGizmo::HIGHLIGHT_COLOR :
        axisColors[axes[1]];

    segments.add(ContourSegment(a, middle, colorA));
    segments.add(ContourSegment(middle, b, colorB));
}

int ScaleGizmo::pickElement(const Ray& ray) const
{
    double nearestDistance = DBL_MAX;
    int nearestGroup = NULL_GROUP;
    int index = 1;
    long i;

    for ( i = 0; i < elements.size(); i++ ) {
        SimpleBody* element = elements.get(i);
        Ray* hit = element->getGeometry() != nullptr ?
            element->doIntersectionFirstHit(ray.withT(DBL_MAX)) : nullptr;

        if ( hit != nullptr && hit->getT() < nearestDistance ) {
            nearestDistance = hit->getT();
            nearestGroup = groupOfElement(index);
        }
        delete hit;
        index++;
    }

    for ( int j = 0; j < BAND_GROUPS_COUNT; j++ ) {
        int group = BAND_GROUPS[j];
        Vector3Dd quad[4];
        double t;

        buildBandQuad(group, quad);
        // The quad, as the two triangles of its strip
        if ( nearestOfTwoTriangles(ray, quad[0], quad[1], quad[2],
                                   quad[1], quad[3], quad[2], &t) &&
             t < nearestDistance ) {
            nearestDistance = t;
            nearestGroup = group;
        }
    }

    java::ArrayList<Vector3Dd> triangles = buildUniformTriangles();

    for ( int k = 0; k + 2 < triangles.size(); k += 3 ) {
        double t;

        if ( rayTriangleT(ray.getOrigin(), ray.getDirection(),
                          triangles[k], triangles[k + 1], triangles[k + 2],
                          &t) &&
             t < nearestDistance ) {
            nearestDistance = t;
            nearestGroup = UNIFORM_GROUP;
        }
    }

    return nearestGroup;
}

int ScaleGizmo::groupOfElement(int element)
{
    switch ( element ) {
      case X_SHAFT_ELEMENT: case X_TIP_ELEMENT: return X_AXIS_GROUP;
      case Y_SHAFT_ELEMENT: case Y_TIP_ELEMENT: return Y_AXIS_GROUP;
      case Z_SHAFT_ELEMENT: case Z_TIP_ELEMENT: return Z_AXIS_GROUP;
      default: return NULL_GROUP;
    }
}

//= Geometry ==============================================================

java::ArrayList<SimpleBody*>& ScaleGizmo::getElements()
{
    return elements;
}

Vector3Dd ScaleGizmo::getAxisDirection(int axis) const
{
    checkAxis(axis);

    Matrix4x4d rotation = transformationMatrix.withoutTranslation();

    return rotation.multiply(unitAxis(axis)).normalized();
}

Vector3Dd ScaleGizmo::getTipPosition(int axis) const
{
    double tipSize = currentScale*TIP_SIZE;

    return axisPoint(axis, 1.0).subtract(
        getAxisDirection(axis).multiply(tipSize/2));
}

Vector3Dd ScaleGizmo::axisPoint(int axis, double reach) const
{
    checkAxis(axis);

    return getPosition().add(
        getAxisDirection(axis).multiply(reach*currentScale*AXIS_LENGTH));
}

void ScaleGizmo::bandAxes(int group, int outAxes[2])
{
    switch ( group ) {
      case XY_GROUP:
        outAxes[0] = 0;
        outAxes[1] = 1;
        break;
      case YZ_GROUP:
        outAxes[0] = 1;
        outAxes[1] = 2;
        break;
      case XZ_GROUP:
        outAxes[0] = 0;
        outAxes[1] = 2;
        break;
      default:
        throw VSDKFatalException(java::String("Invalid two-axis group: ") +
            java::String::valueOf(group) +
            ". It must be XY_GROUP, YZ_GROUP or XZ_GROUP.");
    }
}

void ScaleGizmo::buildBandQuad(int group, Vector3Dd outQuad[4]) const
{
    int axes[2];
    bandAxes(group, axes);

    outQuad[0] = axisPoint(axes[0], BAND_INNER_REACH);
    outQuad[1] = axisPoint(axes[0], BAND_OUTER_REACH);
    outQuad[2] = axisPoint(axes[1], BAND_INNER_REACH);
    outQuad[3] = axisPoint(axes[1], BAND_OUTER_REACH);
}

java::ArrayList<Vector3Dd> ScaleGizmo::buildUniformTriangles() const
{
    Vector3Dd origin = getPosition();
    java::ArrayList<Vector3Dd> vertices;

    for ( int i = 0; i < BAND_GROUPS_COUNT; i++ ) {
        int axes[2];
        bandAxes(BAND_GROUPS[i], axes);

        vertices.add(origin);
        vertices.add(axisPoint(axes[0], BAND_INNER_REACH));
        vertices.add(axisPoint(axes[1], BAND_INNER_REACH));
    }
    return vertices;
}

void ScaleGizmo::calculateGeometryState()
{
    Matrix4x4d R = transformationMatrix.withoutTranslation();
    Vector3Dd origin = getPosition();
    double totalLength = currentScale*AXIS_LENGTH;
    double tipSize = currentScale*TIP_SIZE;
    double shaftLength = std::fmax(0.0, totalLength - tipSize);

    double shaftRadius = getLineWidthInWorldUnits() / 2;
    shaftModel->setBottomRadius(shaftRadius);
    shaftModel->setTopRadius(shaftRadius);
    shaftModel->setHeight(shaftLength);
    tipModel->setSize(tipSize, tipSize, tipSize);

    for ( int axis = 0; axis < AXIS_COUNT; axis++ ) {
        SimpleMaterial material = createMaterial(getAxisDisplayColor(axis));
        Matrix4x4d eleR = R.multiply(axisTilt(axis));
        Matrix4x4d eleRi = eleR.invert();

        SimpleBody* shaft = elements.get(2*axis);

        shaft->setGeometryReference(shaftModel);
        shaft->setMaterial(new SimpleMaterial(material));
        shaft->setRotation(eleR);
        shaft->setRotationInverse(eleRi);
        shaft->setPosition(origin);

        SimpleBody* tip = elements.get(2*axis + 1);

        tip->setGeometryReference(tipModel);
        tip->setMaterial(new SimpleMaterial(material));
        tip->setRotation(eleR);
        tip->setRotationInverse(eleRi);
        tip->setPosition(getTipPosition(axis));
    }
}

SimpleMaterial ScaleGizmo::createMaterial(const ColorRgb& c)
{
    SimpleMaterial m;

    m = m.withAmbient(ColorRgb(0.2, 0.2, 0.2));
    m = m.withDiffuse(c);
    m = m.withSpecular(ColorRgb(1, 1, 1));
    return m;
}

Matrix4x4d ScaleGizmo::axisTilt(int axis)
{
    switch ( axis ) {
      case 0: return Matrix4x4d().axisRotation(M_PI / 2.0, 0, 1, 0);
      case 1: return Matrix4x4d().axisRotation(M_PI / 2.0, -1, 0, 0);
      default: return Matrix4x4d();
    }
}

bool ScaleGizmo::nearestOfTwoTriangles(const Ray& ray,
    const Vector3Dd& a1, const Vector3Dd& b1, const Vector3Dd& c1,
    const Vector3Dd& a2, const Vector3Dd& b2, const Vector3Dd& c2,
    double* outT)
{
    double t1;
    double t2;
    bool hit1 = rayTriangleT(ray.getOrigin(), ray.getDirection(),
                             a1, b1, c1, &t1);
    bool hit2 = rayTriangleT(ray.getOrigin(), ray.getDirection(),
                             a2, b2, c2, &t2);

    if ( !hit1 && !hit2 ) {
        return false;
    }
    if ( !hit1 ) {
        *outT = t2;
    }
    else if ( !hit2 ) {
        *outT = t1;
    }
    else {
        *outT = std::fmin(t1, t2);
    }
    return true;
}

bool ScaleGizmo::rayTriangleT(const Vector3Dd& origin, const Vector3Dd& dir,
    const Vector3Dd& a, const Vector3Dd& b, const Vector3Dd& c, double* outT)
{
    // Ray-triangle intersection (Moller-Trumbore), used to pick the flat
    // handles, which are not backed by a `SimpleBody`.
    Vector3Dd edge1 = b.subtract(a);
    Vector3Dd edge2 = c.subtract(a);
    Vector3Dd h = dir.crossProduct(edge2);
    double det = edge1.dotProduct(h);

    if ( std::fabs(det) < VSDK::EPSILON ) {
        return false;
    }

    double invDet = 1.0/det;
    Vector3Dd s = origin.subtract(a);
    double u = invDet*s.dotProduct(h);

    if ( u < 0.0 || u > 1.0 ) {
        return false;
    }

    Vector3Dd q = s.crossProduct(edge1);
    double v = invDet*dir.dotProduct(q);

    if ( v < 0.0 || u + v > 1.0 ) {
        return false;
    }

    double t = invDet*edge2.dotProduct(q);

    if ( t > VSDK::EPSILON ) {
        *outT = t;
        return true;
    }
    return false;
}

Vector3Dd ScaleGizmo::unitAxis(int axis)
{
    return Vector3Dd(axis == 0 ? 1 : 0, axis == 1 ? 1 : 0, axis == 2 ? 1 : 0);
}

void ScaleGizmo::checkAxis(int axis)
{
    if ( axis < 0 || axis >= AXIS_COUNT ) {
        throw VSDKFatalException(java::String("Invalid axis: ") +
            java::String::valueOf(axis) +
            ". It must be 0, 1 or 2 for the X, Y or Z axis.");
    }
}

//= Numeric input =========================================================

InputGizmo* ScaleGizmo::getInputGizmo()
{
    inputGizmo.setValue(0, scale.x());
    inputGizmo.setValue(1, scale.y());
    inputGizmo.setValue(2, scale.z());
    for ( int axis = 0; axis < AXIS_COUNT; axis++ ) {
        inputGizmo.setFieldHighlighted(axis, isAxisHighlighted(axis));
    }
    return &inputGizmo;
}

//= Events ================================================================

bool ScaleGizmo::processMouseEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processKeyPressedEvent(const KeyEvent& keyEvent)
{
    if ( inputGizmo.consumesKey(keyEvent) ) {
        // Boxes not being edited must start from the current scale
        // factors, not from whatever they showed the last time they were
        // synced (see getInputGizmo)
        getInputGizmo();
        inputGizmo.processKeyPressedEvent(keyEvent);
        if ( inputGizmo.consumeCommit() ) {
            java::ArrayList<double> values = inputGizmo.getValuesWithEdits();

            inputGizmo.cancelEditing();
            scale = Vector3Dd(values[0], values[1], values[2]);
            return true;
        }
        return false;
    }

    char unicodeId = keyEvent.unicodeId;
    int keycode = keyEvent.keycode;
    double deltaMov = 1.1;
    bool updateNeeded = false;
    Vector3Dd s = scale;

    if ( unicodeId != KeyEvent::KEY_NONE ) {
        switch ( unicodeId ) {
          case 'x':
            s = s.withX(s.x() / deltaMov);
            updateNeeded = true;
            break;
          case 'X':
            s = s.withX(s.x() * deltaMov);
            updateNeeded = true;
            break;
          case 'y':
            s = s.withY(s.y() / deltaMov);
            updateNeeded = true;
            break;
          case 'Y':
            s = s.withY(s.y() * deltaMov);
            updateNeeded = true;
            break;
          case 'z':
            s = s.withZ(s.z() / deltaMov);
            updateNeeded = true;
            break;
          case 'Z':
            s = s.withZ(s.z() * deltaMov);
            updateNeeded = true;
            break;
          default:
            break;
        }
    }
    else {
        switch ( keycode ) {
          case KeyEvent::KEY_UP:
          case KeyEvent::KEY_RIGHT:
            s = Vector3Dd(s.x() * deltaMov, s.y() * deltaMov, s.z() * deltaMov);
            updateNeeded = true;
            break;
          case KeyEvent::KEY_LEFT:
          case KeyEvent::KEY_DOWN:
            s = Vector3Dd(s.x() / deltaMov, s.y() / deltaMov, s.z() / deltaMov);
            updateNeeded = true;
            break;
          default:
            break;
        }
    }

    scale = s;
    return updateNeeded;
}

bool ScaleGizmo::processKeyReleasedEvent(const KeyEvent&)
{
    return false;
}

bool ScaleGizmo::processMousePressedEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processMouseReleasedEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processMouseClickedEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processMouseMovedEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processMouseDraggedEvent(const MouseEvent&)
{
    return false;
}

bool ScaleGizmo::processMouseWheelEvent(const MouseEvent&)
{
    return false;
}
