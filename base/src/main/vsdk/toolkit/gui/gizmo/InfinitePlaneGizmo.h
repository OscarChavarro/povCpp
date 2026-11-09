#ifndef __INFINITE_PLANE_GIZMO__
#define __INFINITE_PLANE_GIZMO__

#include <ctime>
#include <pthread.h>

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"

class InfinitePlaneGizmo {
public:
    static const double DEFAULT_DISABLE_TIME;
    static const ColorRgb DEFAULT_FRAME_COLOR;

    class PlaneSnapshot {
    public:
        InfinitePlane plane;
        Vector3Dd point;
        Vector3Dd normal;

        PlaneSnapshot();
        PlaneSnapshot(const InfinitePlane& plane, const Vector3Dd& point, const Vector3Dd& normal);
    };

    InfinitePlaneGizmo();
    ~InfinitePlaneGizmo();

    void setPlane(const InfinitePlane& plane, const Vector3Dd& point, const Vector3Dd& normal);
    void setPlane(const Vector3Dd& point, const Vector3Dd& normal);
    PlaneSnapshot* acquireSnapshot();
    void update();
    InfinitePlane getPlane() const;
    Vector3Dd getPoint() const;
    Vector3Dd getNormal() const;
    bool isVisible() const;
    void setVisible(bool visible);
    double getDisableAfterElapsedSeconds() const;
    void setDisableAfterElapsedSeconds(double disableAfterElapsedSeconds);
    std::time_t getLastDataTime() const;
    std::time_t getPreviousDataTime() const;
    ColorRgb getFrameColor() const;
    void setFrameColor(const ColorRgb& frameColor);

private:
    pthread_mutex_t pendingMutex;
    PlaneSnapshot* pendingSnapshot;

    InfinitePlane currentPlane;
    Vector3Dd currentPoint;
    Vector3Dd currentNormal;

    std::time_t lastDataTime;
    std::time_t previousDataTime;
    bool visible;
    double disableAfterElapsedSeconds;
    ColorRgb frameColor;

    void recordDataArrival();
    bool inactivityThresholdExceeded() const;
};

#endif
