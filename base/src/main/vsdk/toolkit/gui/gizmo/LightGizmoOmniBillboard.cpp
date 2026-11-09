#include <cmath>

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/media/Calligraphic2DBuffer.h"
#include "vsdk/toolkit/gui/gizmo/LightGizmoOmniBillboard.h"
static const int NUMBER_OF_SIDES = 32;
static const int NUMBER_OF_RAYS = 8;
static const double CIRCLE_RADIUS = 0.2;
static const double RAY_INNER_RADIUS = 0.3;
static const double RAY_OUTER_RADIUS = 0.5;

/// Apparent size of the gizmo (its full width), as a fraction of the
/// smaller dimension of the viewport
static const double VIEWPORT_FRACTION = 0.05;

double LightGizmoOmniBillboard::calculateWorldHalfSize(
    const Camera* camera, const Vector3Dd& position, int viewportWidth,
    int viewportHeight)
{
    double targetPixels = VIEWPORT_FRACTION *
        (viewportWidth < viewportHeight ? viewportWidth : viewportHeight);
    double safeHeight = viewportHeight > 1 ? viewportHeight : 1;

    if ( camera == nullptr ) {
        return std::fmax(0.05, targetPixels / safeHeight);
    }

    if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
        double worldViewHeight = 2.0 / camera->getOrthogonalZoom();
        double worldPerPixel = worldViewHeight / safeHeight;
        return std::fmax(1e-5, 0.5 * targetPixels * worldPerPixel);
    }

    Vector3Dd toLight = position.subtract(camera->getPosition());
    double depth = std::fabs(toLight.dotProduct(camera->getFront()));
    depth = std::fmax(depth, camera->getNearPlaneDistance());

    double fovRadians = camera->getFov() * M_PI / 180.0;
    double worldViewHeightAtDepth = 2.0 * depth * std::tan(fovRadians / 2.0);
    double worldPerPixel = worldViewHeightAtDepth / safeHeight;
    return std::fmax(1e-5, 0.5 * targetPixels * worldPerPixel);
}

Calligraphic2DBuffer LightGizmoOmniBillboard::createLinePattern()
{
    Calligraphic2DBuffer lines;

    const double cx = 0.5;
    const double cy = 0.5;

    for (int i = 0; i < NUMBER_OF_SIDES; i++) {
        double a0 = 2.0 * M_PI * (double)i / (double)NUMBER_OF_SIDES;
        double a1 = 2.0 * M_PI * (double)(i + 1) / (double)NUMBER_OF_SIDES;

        double x0 = cx + CIRCLE_RADIUS * std::cos(a0);
        double y0 = cy + CIRCLE_RADIUS * std::sin(a0);
        double x1 = cx + CIRCLE_RADIUS * std::cos(a1);
        double y1 = cy + CIRCLE_RADIUS * std::sin(a1);

        lines.add2DLine(x0, y0, x1, y1);
    }

    for (int i = 0; i < NUMBER_OF_RAYS; i++) {
        double a = 2.0 * M_PI * (double)i / (double)NUMBER_OF_RAYS;

        double x0 = cx + RAY_INNER_RADIUS * std::cos(a);
        double y0 = cy + RAY_INNER_RADIUS * std::sin(a);
        double x1 = cx + RAY_OUTER_RADIUS * std::cos(a);
        double y1 = cy + RAY_OUTER_RADIUS * std::sin(a);

        lines.add2DLine(Vector3Dd(x0, y0, 0.0), Vector3Dd(x1, y1, 0.0));
    }

    return lines;
}
