#include <cfloat>
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/gui/gizmo/ReferenceFrameGizmo.h"
#include "vsdk/toolkit/gui/gizmo/RotateGizmo.h"
#include "vsdk/toolkit/gui/viewport/ViewportElementScaler.h"

const double RotateGizmo::DEFAULT_LINE_WIDTH = 1.0;
const double RotateGizmo::ARC_STEP_IN_DEGREES = 3.0;

namespace {
/// Distance of the label of the rotation arc from the center of the gizmo,
/// relative to the radius of the rings
const double ARC_LABEL_DISTANCE = 1.2;

/// Radius of the rings, and tolerance to point at them with the cursor,
/// relative to the apparent size of the gizmo (1.0 is the size in pixels)
const double RING_RADIUS = 0.8;
const double PICK_TOLERANCE = 0.06;
const double MAX_TUBE_TO_RING_RADIUS_RATIO = 0.5;

/// The camera ring is a little bigger than the other three, so it does
/// not overlap them (see the reference image of 3D Studio Max)
const double CAMERA_RING_RADIUS_FACTOR = 1.15;
const ColorRgb CAMERA_RING_COLOR(0.6, 0.6, 0.6);

/// An axis ring is only clipped to its near half when it is seen at some
/// angle: when its axis is almost aligned with the view direction (it is
/// seen face on, like the camera ring), it is drawn and picked whole,
/// because every one of its points is then equally close to the camera
const double AXIS_VIEW_ALIGNMENT_THRESHOLD = 0.98;

/// Rotation of the local frame of each torus (whose axis is Z) so its
/// axis is the axis of its ring
const double RING_TILT[3][4] = {
    {M_PI / 2.0, 0, 1, 0},
    {M_PI / 2.0, -1, 0, 0},
    {0, 0, 0, 1}
};

double toRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

double toDegrees(double radians)
{
    return radians * 180.0 / M_PI;
}
}

RotateGizmo::RotateGizmo(Camera* cam)
    : camera(nullptr),
      baseApparentSizeInPixels(DEFAULT_APPARENT_SIZE_IN_PIXELS),
      apparentSizeInPixels(DEFAULT_APPARENT_SIZE_IN_PIXELS),
      currentScale(1.0), arcRing(-1), arcStartAngle(0.0), arcSweep(0.0),
      inputGizmo(INPUT_FIELD_COUNT, ANGLE_DECIMALS, ANGLE_INTEGER_DIGITS,
                 InputGizmoValueChangeRules::forRotation()),
      persistentSelection(NULL_GROUP), volatileSelection(NULL_GROUP)
{
    ReferenceFrameGizmo axisColorSource;

    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        axisColors[ring] = axisColorSource.getAxisColor(ring);
        inputGizmo.setFieldColor(ring, axisColors[ring]);
        ringModels[ring] = new Torus(RING_RADIUS, PICK_TOLERANCE);
        ringInstances[ring] = new SimpleBody();
        ringInstances[ring]->setGeometry(ringModels[ring]);
        baseRingLineWidths[ring] = DEFAULT_LINE_WIDTH;
        ringLineWidths[ring] = DEFAULT_LINE_WIDTH;
    }
    inputGizmo.setFieldColor(CAMERA_INPUT_FIELD_INDEX, CAMERA_RING_COLOR);
    baseCameraRingLineWidth = DEFAULT_LINE_WIDTH;
    cameraRingLineWidth = DEFAULT_LINE_WIDTH;
    cameraRingModel = new Torus(RING_RADIUS * CAMERA_RING_RADIUS_FACTOR,
                                PICK_TOLERANCE);
    cameraRingInstance = new SimpleBody();
    cameraRingInstance->setGeometry(cameraRingModel);
    setCamera(cam);
}

RotateGizmo::~RotateGizmo()
{
    // Each instance owns its torus model
    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        delete ringInstances[ring];
    }
    delete cameraRingInstance;
}

//= Camera, size and line widths ==========================================

void RotateGizmo::setCamera(Camera* cam)
{
    camera = cam;
}

Camera* RotateGizmo::getCamera() const
{
    return camera;
}

int RotateGizmo::getApparentSizeInPixels() const
{
    return apparentSizeInPixels;
}

void RotateGizmo::setApparentSizeInPixels(int size)
{
    apparentSizeInPixels = size;
}

int RotateGizmo::getBaseApparentSizeInPixels() const
{
    return baseApparentSizeInPixels;
}

void RotateGizmo::setBaseApparentSizeInPixels(int size)
{
    if ( size > 0 ) {
        baseApparentSizeInPixels = size;
    }
}

double RotateGizmo::getRingLineWidth(int ring) const
{
    checkRing(ring);
    return ringLineWidths[ring];
}

void RotateGizmo::setRingLineWidth(int ring, double lineWidth)
{
    checkRing(ring);
    if ( lineWidth > 0.0 ) {
        ringLineWidths[ring] = lineWidth;
    }
}

double RotateGizmo::getBaseRingLineWidth(int ring) const
{
    checkRing(ring);
    return baseRingLineWidths[ring];
}

void RotateGizmo::setBaseRingLineWidth(int ring, double lineWidth)
{
    checkRing(ring);
    if ( lineWidth > 0.0 ) {
        baseRingLineWidths[ring] = lineWidth;
    }
}

double RotateGizmo::getCameraRingLineWidth() const
{
    return cameraRingLineWidth;
}

void RotateGizmo::setCameraRingLineWidth(double lineWidth)
{
    if ( lineWidth > 0.0 ) {
        cameraRingLineWidth = lineWidth;
    }
}

double RotateGizmo::getBaseCameraRingLineWidth() const
{
    return baseCameraRingLineWidth;
}

void RotateGizmo::setBaseCameraRingLineWidth(double lineWidth)
{
    if ( lineWidth > 0.0 ) {
        baseCameraRingLineWidth = lineWidth;
    }
}

void RotateGizmo::applyScale(const ViewportElementScaler* scaler)
{
    if ( scaler == nullptr ) {
        return;
    }
    setApparentSizeInPixels(scaler->scaleSize(baseApparentSizeInPixels));
    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        ringLineWidths[ring] = scaler->scaleLength(baseRingLineWidths[ring]);
    }
    cameraRingLineWidth = scaler->scaleLength(baseCameraRingLineWidth);
}

double RotateGizmo::getCurrentScale() const
{
    return currentScale;
}

double RotateGizmo::getRingLineWidthInWorldUnits(int ring) const
{
    checkRing(ring);
    return ringLineWidths[ring] * currentScale / apparentSizeInPixels;
}

double RotateGizmo::getCameraRingLineWidthInWorldUnits() const
{
    return cameraRingLineWidth * currentScale / apparentSizeInPixels;
}

double RotateGizmo::getRingRadius() const
{
    return RING_RADIUS * currentScale;
}

double RotateGizmo::getCameraRingRadius() const
{
    return RING_RADIUS * CAMERA_RING_RADIUS_FACTOR * currentScale;
}

//= Transformation ========================================================

Vector3Dd RotateGizmo::getPosition() const
{
    return transformationMatrix.extractTranslation();
}

void RotateGizmo::setPosition(const Vector3Dd& p)
{
    setTransformationMatrix(transformationMatrix.withTranslation(p));
}

void RotateGizmo::setTransformationMatrix(
    const Matrix4x4d& transformationMatrix)
{
    this->transformationMatrix = transformationMatrix;
    updateGeometryState();
}

Matrix4x4d RotateGizmo::getTransformationMatrix() const
{
    return transformationMatrix;
}

void RotateGizmo::updateGeometryState()
{
    if ( camera == nullptr ) {
        return;
    }
    updateScale();

    double ringRadius = getRingRadius();
    Matrix4x4d rotation = transformationMatrix.withoutTranslation();
    Vector3Dd position = getPosition();

    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        // The tube covers the width of the drawn ring, at least
        double tubeRadius = std::fmax(PICK_TOLERANCE * currentScale,
            getRingLineWidthInWorldUnits(ring) / 2);

        ringModels[ring]->setMajorRadius(ringRadius);
        ringModels[ring]->setMinorRadius(
            std::fmin(tubeRadius, ringRadius * MAX_TUBE_TO_RING_RADIUS_RATIO));

        const double* tilt = RING_TILT[ring];
        Matrix4x4d ringRotation = rotation.multiply(
            Matrix4x4d().axisRotation(tilt[0], tilt[1], tilt[2], tilt[3]));

        ringInstances[ring]->setRotation(ringRotation);
        ringInstances[ring]->setPosition(position);
    }

    // The camera ring: its axis is the front vector of the camera, not
    // one of the axes of the frame of the gizmo
    double cameraRingRadius = getCameraRingRadius();
    double cameraTubeRadius = std::fmax(PICK_TOLERANCE * currentScale,
        getCameraRingLineWidthInWorldUnits() / 2);
    Vector3Dd right = getCameraPlaneRightDirection();
    Vector3Dd up = getCameraPlaneUpDirection();
    Vector3Dd front = getCameraAxisDirection();

    cameraRingModel->setMajorRadius(cameraRingRadius);
    cameraRingModel->setMinorRadius(
        std::fmin(cameraTubeRadius,
                  cameraRingRadius * MAX_TUBE_TO_RING_RADIUS_RATIO));
    cameraRingInstance->setRotation(rotationFromBasis(right, up, front));
    cameraRingInstance->setPosition(position);
}

void RotateGizmo::updateScale()
{
    // Calculates the scale that makes the gizmo look as big as its apparent
    // size, from its camera. It is kept as it was if the size in pixels can
    // not be measured.
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

//= Geometry of the rings =================================================

Torus* RotateGizmo::getRingModel(int ring) const
{
    checkRing(ring);
    return ringModels[ring];
}

SimpleBody* RotateGizmo::getRingInstance(int ring) const
{
    checkRing(ring);
    return ringInstances[ring];
}

Torus* RotateGizmo::getCameraRingModel() const
{
    return cameraRingModel;
}

SimpleBody* RotateGizmo::getCameraRingInstance() const
{
    return cameraRingInstance;
}

Vector3Dd RotateGizmo::getAxisDirection(int ring) const
{
    checkRing(ring);

    Matrix4x4d rotation = transformationMatrix.withoutTranslation();

    return rotation.multiply(unitAxis(ring)).normalized();
}

Vector3Dd RotateGizmo::getCameraAxisDirection() const
{
    return camera->getFront().normalized();
}

Vector3Dd RotateGizmo::getCameraPlaneRightDirection() const
{
    Vector3Dd front = getCameraAxisDirection();
    Vector3Dd approximateUp = camera->getUp().normalized();

    return approximateUp.crossProduct(front).normalized();
}

Vector3Dd RotateGizmo::getCameraPlaneUpDirection() const
{
    return getCameraAxisDirection().crossProduct(
        getCameraPlaneRightDirection()).normalized();
}

bool RotateGizmo::isPointOnVisibleHemisphere(const Vector3Dd& point) const
{
    return visibilityMetric(point, getPosition()) < VSDK::EPSILON;
}

bool RotateGizmo::needsHemisphereClipping(const Vector3Dd& axis,
                                          const Vector3Dd& center) const
{
    Vector3Dd viewDirection =
        camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ?
        camera->getFront() : center.subtract(camera->getPosition()).normalized();

    return std::fabs(axis.normalized().dotProduct(viewDirection)) <
        AXIS_VIEW_ALIGNMENT_THRESHOLD;
}

double RotateGizmo::visibilityMetric(const Vector3Dd& point,
                                     const Vector3Dd& center) const
{
    Vector3Dd normal = point.subtract(center);
    double length = normal.length();

    if ( length < VSDK::EPSILON ) {
        return -1.0;
    }
    normal = normal.multiply(1.0 / length);

    Vector3Dd viewDirection =
        camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ?
        camera->getFront() : point.subtract(camera->getPosition()).normalized();

    return normal.dotProduct(viewDirection);
}

java::ArrayList<java::ArrayList<Vector3Dd> >
RotateGizmo::buildRingStrips(int ring) const
{
    checkRing(ring);

    Vector3Dd center = getPosition();
    Vector3Dd axis = getAxisDirection(ring);
    Matrix4x4d rotation = transformationMatrix.withoutTranslation();
    // The plane of the ring is spanned by the next two axes (X: YZ,
    // Y: ZX, Z: XY), so the ring goes around its axis counterclockwise
    Vector3Dd u = rotation.multiply(unitAxis((ring + 1) % RING_COUNT)).normalized();
    Vector3Dd v = rotation.multiply(unitAxis((ring + 2) % RING_COUNT)).normalized();
    double radius = getRingRadius();
    double halfWidth = getRingLineWidthInWorldUnits(ring) / 2;

    if ( !needsHemisphereClipping(axis, center) ) {
        java::ArrayList<java::ArrayList<Vector3Dd> > whole;

        whole.add(buildFullRingStrip(center, axis, u, v, radius, halfWidth));
        return whole;
    }
    return buildVisibleRingArcs(center, axis, u, v, radius, halfWidth);
}

java::ArrayList<Vector3Dd> RotateGizmo::buildCameraRingStrip() const
{
    Vector3Dd center = getPosition();
    Vector3Dd axis = getCameraAxisDirection();
    Vector3Dd u = getCameraPlaneRightDirection();
    Vector3Dd v = getCameraPlaneUpDirection();
    double halfWidth = getCameraRingLineWidthInWorldUnits() / 2;

    return buildFullRingStrip(center, axis, u, v, getCameraRingRadius(),
                              halfWidth);
}

java::ArrayList<Vector3Dd> RotateGizmo::buildFullRingStrip(
    const Vector3Dd& center, const Vector3Dd& axis, const Vector3Dd& u,
    const Vector3Dd& v, double radius, double halfWidth) const
{
    java::ArrayList<Vector3Dd> strip;

    for ( int i = 0; i <= RING_SEGMENTS; i++ ) {
        double angle = 2 * M_PI * (i % RING_SEGMENTS) / RING_SEGMENTS;
        double cosValue = std::cos(angle);
        double sinValue = std::sin(angle);
        Vector3Dd point = center.add(u.multiply(radius * cosValue)).add(
            v.multiply(radius * sinValue));
        Vector3Dd tangent = v.multiply(cosValue).subtract(u.multiply(sinValue));
        Vector3Dd side = sideAcross(point, tangent, axis, halfWidth);

        strip.add(point.add(side));
        strip.add(point.subtract(side));
    }
    return strip;
}

java::ArrayList<java::ArrayList<Vector3Dd> > RotateGizmo::buildVisibleRingArcs(
    const Vector3Dd& center, const Vector3Dd& axis, const Vector3Dd& u,
    const Vector3Dd& v, double radius, double halfWidth) const
{
    int n = RING_SEGMENTS;
    java::ArrayList<Vector3Dd> points;
    java::ArrayList<double> metrics;

    for ( int i = 0; i <= n; i++ ) {
        double angle = 2 * M_PI * (i % n) / n;
        Vector3Dd point = center.add(u.multiply(radius * std::cos(angle)))
            .add(v.multiply(radius * std::sin(angle)));

        points.add(point);
        metrics.add(visibilityMetric(point, center));
    }

    java::ArrayList<java::ArrayList<Vector3Dd> > arcs;
    java::ArrayList<Vector3Dd> current;

    for ( int i = 0; i < n; i++ ) {
        bool visibleA = metrics[i] < 0;
        bool visibleB = metrics[i + 1] < 0;

        if ( visibleA && current.size() == 0 ) {
            current.add(points[i]);
        }
        if ( visibleA != visibleB ) {
            Vector3Dd boundary = interpolateOnCircle(
                center, points[i], points[i + 1], metrics[i], metrics[i + 1],
                radius);

            current.add(boundary);
            if ( visibleA ) {
                arcs.add(toRibbonStrip(current, axis, halfWidth));
                current.clear();
            }
            else {
                current.clear();
                current.add(boundary);
            }
        }
        else if ( visibleA ) {
            current.add(points[i + 1]);
        }
    }
    if ( current.size() > 1 ) {
        arcs.add(toRibbonStrip(current, axis, halfWidth));
    }
    return arcs;
}

Vector3Dd RotateGizmo::interpolateOnCircle(const Vector3Dd& center,
    const Vector3Dd& a, const Vector3Dd& b, double metricA, double metricB,
    double radius)
{
    double denominator = metricA - metricB;
    double t = std::fabs(denominator) < VSDK::EPSILON ?
        0.5 : metricA / denominator;

    t = std::fmax(0.0, std::fmin(1.0, t));

    Vector3Dd blend = a.add(b.subtract(a).multiply(t));
    Vector3Dd fromCenter = blend.subtract(center);
    double length = fromCenter.length();

    if ( length < VSDK::EPSILON ) {
        return blend;
    }
    return center.add(fromCenter.multiply(radius / length));
}

java::ArrayList<Vector3Dd> RotateGizmo::toRibbonStrip(
    const java::ArrayList<Vector3Dd>& polyline, const Vector3Dd& axis,
    double halfWidth) const
{
    int count = (int)polyline.size();
    java::ArrayList<Vector3Dd> strip;

    for ( int i = 0; i < count; i++ ) {
        Vector3Dd point = polyline.get(i);
        Vector3Dd previous = polyline.get(i - 1 > 0 ? i - 1 : 0);
        Vector3Dd next = polyline.get(i + 1 < count - 1 ? i + 1 : count - 1);
        Vector3Dd tangent = next.subtract(previous);
        Vector3Dd side = sideAcross(point, tangent, axis, halfWidth);

        strip.add(point.add(side));
        strip.add(point.subtract(side));
    }
    return strip;
}

Vector3Dd RotateGizmo::sideAcross(const Vector3Dd& point,
    const Vector3Dd& tangent, const Vector3Dd& axis, double halfWidth) const
{
    Vector3Dd view;

    if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
        view = camera->getFront();
    }
    else {
        view = point.subtract(camera->getPosition());
    }

    Vector3Dd side = tangent.crossProduct(view);

    if ( side.length() < VSDK::EPSILON ) {
        // Ring pointing to the viewer, or degenerate tangent: any
        // direction across the ring is valid
        side = axis;
    }
    return side.normalized().multiply(halfWidth);
}

Vector3Dd RotateGizmo::unitAxis(int axis)
{
    return Vector3Dd(axis == 0 ? 1 : 0, axis == 1 ? 1 : 0, axis == 2 ? 1 : 0);
}

Matrix4x4d RotateGizmo::rotationFromBasis(const Vector3Dd& xAxis,
    const Vector3Dd& yAxis, const Vector3Dd& zAxis)
{
    const double values[4][4] = {
        {xAxis.x(), yAxis.x(), zAxis.x(), 0},
        {xAxis.y(), yAxis.y(), zAxis.y(), 0},
        {xAxis.z(), yAxis.z(), zAxis.z(), 0},
        {0, 0, 0, 1}
    };

    return Matrix4x4d(values);
}

//= Selection =============================================================

int RotateGizmo::pickRing(const Ray& ray) const
{
    double nearestDistance = DBL_MAX;
    int nearestRing = NULL_GROUP;
    Vector3Dd center = getPosition();

    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        Ray* hit = ringInstances[ring]->doIntersectionFirstHit(
            ray.withT(DBL_MAX));

        if ( hit == nullptr || hit->getT() >= nearestDistance ) {
            delete hit;
            continue;
        }

        double t = hit->getT();
        delete hit;
        Vector3Dd hitPoint = ray.getOrigin().add(
            ray.getDirection().multiply(t));

        if ( needsHemisphereClipping(getAxisDirection(ring), center) &&
             !isPointOnVisibleHemisphere(hitPoint) ) {
            continue;
        }
        nearestDistance = t;
        nearestRing = groupOfRing(ring);
    }

    Ray* cameraHit = cameraRingInstance->doIntersectionFirstHit(
        ray.withT(DBL_MAX));

    if ( cameraHit != nullptr && cameraHit->getT() < nearestDistance ) {
        nearestRing = CAMERA_RING_GROUP;
    }
    delete cameraHit;
    return nearestRing;
}

int RotateGizmo::groupOfRing(int ring)
{
    checkRing(ring);
    return ring + 1;
}

int RotateGizmo::getPersistentSelection() const
{
    return persistentSelection;
}

void RotateGizmo::setPersistentSelection(int selection)
{
    persistentSelection = selection;
}

int RotateGizmo::getVolatileSelection() const
{
    return volatileSelection;
}

void RotateGizmo::setVolatileSelection(int selection)
{
    volatileSelection = selection;
}

int RotateGizmo::getCurrentSelection() const
{
    if ( volatileSelection == NULL_GROUP ) {
        return persistentSelection;
    }
    return volatileSelection;
}

bool RotateGizmo::isRingHighlighted(int ring) const
{
    checkRing(ring);
    return getCurrentSelection() == groupOfRing(ring);
}

bool RotateGizmo::isCameraRingHighlighted() const
{
    return getCurrentSelection() == CAMERA_RING_GROUP;
}

ColorRgb RotateGizmo::getRingColor(int ring) const
{
    checkRing(ring);
    return isRingHighlighted(ring) ? InputGizmo::HIGHLIGHT_COLOR :
        axisColors[ring];
}

ColorRgb RotateGizmo::getCameraRingColor() const
{
    return isCameraRingHighlighted() ? InputGizmo::HIGHLIGHT_COLOR :
        CAMERA_RING_COLOR;
}

//= Rotation arc ==========================================================

void RotateGizmo::setArc(int ring, const Vector3Dd& u, const Vector3Dd& v,
                         double startAngle, double sweep)
{
    checkRingOrCamera(ring);
    arcRing = ring;
    arcU = u;
    arcV = v;
    arcStartAngle = startAngle;
    arcSweep = sweep;
}

void RotateGizmo::clearArc()
{
    arcRing = -1;
}

bool RotateGizmo::isArcVisible() const
{
    return arcRing >= 0;
}

int RotateGizmo::getArcRing() const
{
    return arcRing;
}

double RotateGizmo::getArcSweep() const
{
    return arcSweep;
}

double RotateGizmo::getArcSweepInDegrees() const
{
    return toDegrees(arcSweep);
}

ColorRgb RotateGizmo::getArcColor() const
{
    if ( arcRing == CAMERA_RING_INDEX ) {
        return CAMERA_RING_COLOR;
    }
    return axisColors[arcRing > 0 ? arcRing : 0];
}

java::ArrayList<Vector3Dd> RotateGizmo::buildArcFan() const
{
    java::ArrayList<Vector3Dd> fan;

    if ( !isArcVisible() || std::fabs(arcSweep) < VSDK::EPSILON ) {
        return fan;
    }

    double sweep = std::fmax(-2 * M_PI, std::fmin(2 * M_PI, arcSweep));
    int segments = (int)std::ceil(std::fabs(toDegrees(sweep)) /
                                  ARC_STEP_IN_DEGREES);
    if ( segments < 1 ) {
        segments = 1;
    }

    fan.add(getPosition());
    for ( int i = 0; i <= segments; i++ ) {
        fan.add(pointOfArc(arcStartAngle + sweep * i / segments, 1.0));
    }
    return fan;
}

bool RotateGizmo::getArcLabelPosition(Vector3Dd* outPosition) const
{
    if ( !isArcVisible() ) {
        return false;
    }
    *outPosition = pointOfArc(arcStartAngle + arcSweep / 2,
                              ARC_LABEL_DISTANCE);
    return true;
}

double RotateGizmo::arcRingRadius() const
{
    return arcRing == CAMERA_RING_INDEX ? getCameraRingRadius() :
        getRingRadius();
}

Vector3Dd RotateGizmo::pointOfArc(double angle, double radiusFactor) const
{
    double radius = arcRingRadius() * radiusFactor;

    return getPosition().add(arcU.multiply(std::cos(angle) * radius))
        .add(arcV.multiply(std::sin(angle) * radius));
}

//= Numeric input =========================================================

InputGizmo* RotateGizmo::getInputGizmo()
{
    double angles[3];
    extractAnglesInDegrees(transformationMatrix, angles);

    for ( int ring = 0; ring < RING_COUNT; ring++ ) {
        inputGizmo.setValue(ring, angles[ring]);
        inputGizmo.setFieldHighlighted(ring, isRingHighlighted(ring));
    }
    inputGizmo.setValue(CAMERA_INPUT_FIELD_INDEX, 0.0);
    inputGizmo.setFieldHighlighted(CAMERA_INPUT_FIELD_INDEX,
                                   isCameraRingHighlighted());
    return &inputGizmo;
}

Matrix4x4d RotateGizmo::createRotationFromAnglesInDegrees(double xDegrees,
                                                          double yDegrees,
                                                          double zDegrees)
{
    Matrix4x4d rx = Matrix4x4d().axisRotation(toRadians(xDegrees), 1, 0, 0);
    Matrix4x4d ry = Matrix4x4d().axisRotation(toRadians(yDegrees), 0, 1, 0);
    Matrix4x4d rz = Matrix4x4d().axisRotation(toRadians(zDegrees), 0, 0, 1);

    return rz.multiply(ry.multiply(rx));
}

void RotateGizmo::extractAnglesInDegrees(const Matrix4x4d& rotation,
                                         double outAngles[3])
{
    double sinY = std::fmax(-1.0, std::fmin(1.0, -rotation.get(2, 0)));
    double y = std::asin(sinY);
    double x;
    double z;

    if ( std::fabs(sinY) < 1.0 - 1.0e-9 ) {
        x = std::atan2(rotation.get(2, 1), rotation.get(2, 2));
        z = std::atan2(rotation.get(1, 0), rotation.get(0, 0));
    }
    else {
        x = std::atan2(-rotation.get(1, 2), rotation.get(1, 1));
        z = 0;
    }
    outAngles[0] = toDegrees(x);
    outAngles[1] = toDegrees(y);
    outAngles[2] = toDegrees(z);
}

void RotateGizmo::checkRing(int ring)
{
    if ( ring < 0 || ring >= RING_COUNT ) {
        throw VSDKFatalException(java::String("Invalid ring: ") +
            java::String::valueOf(ring) +
            ". It must be 0, 1 or 2 for the X, Y or Z axis.");
    }
}

void RotateGizmo::checkRingOrCamera(int ring)
{
    if ( ring < 0 || ring > RING_COUNT ) {
        throw VSDKFatalException(java::String("Invalid ring: ") +
            java::String::valueOf(ring) +
            ". It must be 0, 1 or 2 for the X, Y or Z axis, or " +
            java::String::valueOf(CAMERA_RING_INDEX) +
            " for the camera ring.");
    }
}
