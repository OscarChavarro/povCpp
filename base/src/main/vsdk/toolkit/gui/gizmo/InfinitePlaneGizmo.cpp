#include "java/lang/RuntimeException.h"

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/gui/gizmo/InfinitePlaneGizmo.h"

const double InfinitePlaneGizmo::DEFAULT_DISABLE_TIME = 2.0;
const ColorRgb InfinitePlaneGizmo::DEFAULT_FRAME_COLOR = ColorRgb(1, 1, 1);

InfinitePlaneGizmo::PlaneSnapshot::PlaneSnapshot()
    : plane(Vector3Dd(0, 0, 1), Vector3Dd(0, 0, 0)),
      point(0, 0, 0),
      normal(0, 0, 1)
{
}

InfinitePlaneGizmo::PlaneSnapshot::PlaneSnapshot(
    const InfinitePlane& plane, const Vector3Dd& point, const Vector3Dd& normal)
    : plane(plane),
      point(point),
      normal(normal.normalized())
{
    if ( normal.length() < VSDK::EPSILON ) {
        throw java::RuntimeException("normal cannot be null or zero");
    }
}

InfinitePlaneGizmo::InfinitePlaneGizmo()
    : pendingSnapshot(0),
      currentPlane(Vector3Dd(0, 0, 1), Vector3Dd(0, 0, 0)),
      currentPoint(0, 0, 0),
      currentNormal(0, 0, 1),
      lastDataTime(std::time(0)),
      previousDataTime(std::time(0)),
      visible(true),
      disableAfterElapsedSeconds(DEFAULT_DISABLE_TIME),
      frameColor(DEFAULT_FRAME_COLOR)
{
    pthread_mutex_init(&pendingMutex, 0);
}

InfinitePlaneGizmo::~InfinitePlaneGizmo()
{
    pthread_mutex_lock(&pendingMutex);
    delete pendingSnapshot;
    pendingSnapshot = 0;
    pthread_mutex_unlock(&pendingMutex);
    pthread_mutex_destroy(&pendingMutex);
}

void InfinitePlaneGizmo::setPlane(const InfinitePlane& plane, const Vector3Dd& point, const Vector3Dd& normal)
{
    if ( normal.length() < VSDK::EPSILON ) {
        return;
    }

    PlaneSnapshot* snap = new PlaneSnapshot(plane, point, normal);
    pthread_mutex_lock(&pendingMutex);
    delete pendingSnapshot;
    pendingSnapshot = snap;
    pthread_mutex_unlock(&pendingMutex);
    visible = true;
    recordDataArrival();
}

void InfinitePlaneGizmo::setPlane(const Vector3Dd& point, const Vector3Dd& normal)
{
    if ( normal.length() < VSDK::EPSILON ) {
        return;
    }
    setPlane(InfinitePlane(normal, point), point, normal);
}

InfinitePlaneGizmo::PlaneSnapshot* InfinitePlaneGizmo::acquireSnapshot()
{
    pthread_mutex_lock(&pendingMutex);
    PlaneSnapshot* snap = pendingSnapshot;
    pendingSnapshot = 0;
    pthread_mutex_unlock(&pendingMutex);

    if ( snap == 0 ) {
        return 0;
    }

    currentPlane = InfinitePlane(snap->plane);
    currentPoint = snap->point;
    currentNormal = snap->normal.normalized();
    return snap;
}

void InfinitePlaneGizmo::update()
{
    if ( inactivityThresholdExceeded() ) {
        visible = false;
    }
    previousDataTime = lastDataTime;
}

InfinitePlane InfinitePlaneGizmo::getPlane() const
{
    return currentPlane;
}

Vector3Dd InfinitePlaneGizmo::getPoint() const
{
    return currentPoint;
}

Vector3Dd InfinitePlaneGizmo::getNormal() const
{
    return currentNormal;
}

bool InfinitePlaneGizmo::isVisible() const
{
    return visible;
}

void InfinitePlaneGizmo::setVisible(bool visible)
{
    this->visible = visible;
}

double InfinitePlaneGizmo::getDisableAfterElapsedSeconds() const
{
    return disableAfterElapsedSeconds;
}

void InfinitePlaneGizmo::setDisableAfterElapsedSeconds(double disableAfterElapsedSeconds)
{
    this->disableAfterElapsedSeconds = disableAfterElapsedSeconds;
}

std::time_t InfinitePlaneGizmo::getLastDataTime() const
{
    return lastDataTime;
}

std::time_t InfinitePlaneGizmo::getPreviousDataTime() const
{
    return previousDataTime;
}

ColorRgb InfinitePlaneGizmo::getFrameColor() const
{
    return frameColor;
}

void InfinitePlaneGizmo::setFrameColor(const ColorRgb& frameColor)
{
    this->frameColor = frameColor;
}

void InfinitePlaneGizmo::recordDataArrival()
{
    previousDataTime = lastDataTime;
    lastDataTime = std::time(0);
}

bool InfinitePlaneGizmo::inactivityThresholdExceeded() const
{
    double elapsed = std::difftime(std::time(0), lastDataTime);
    return disableAfterElapsedSeconds > 0.0 && elapsed > disableAfterElapsedSeconds;
}
