#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/gizmo/GizmoVertexArrayBuilder.h"

void GizmoVertexArrayBuilder::putVertex(java::ArrayList<float>& positions,
                                        const Vector3Dd& point)
{
    positions.add((float)point.x());
    positions.add((float)point.y());
    positions.add((float)point.z());
}

void GizmoVertexArrayBuilder::putRgb(java::ArrayList<float>& colors,
                                     const ColorRgb& color)
{
    colors.add((float)color.r());
    colors.add((float)color.g());
    colors.add((float)color.b());
}

void GizmoVertexArrayBuilder::putRgba(java::ArrayList<float>& colors,
                                      const ColorRgb& color, double alpha)
{
    colors.add((float)color.r());
    colors.add((float)color.g());
    colors.add((float)color.b());
    colors.add((float)alpha);
}

java::ArrayList<float> GizmoVertexArrayBuilder::buildPositions(
    const java::ArrayList<Vector3Dd>& points)
{
    java::ArrayList<float> positions;

    for ( long i = 0; i < points.size(); i++ ) {
        putVertex(positions, points.get(i));
    }
    return positions;
}

java::ArrayList<float> GizmoVertexArrayBuilder::buildRgbColors(
    long numberOfVertexes, const ColorRgb& color)
{
    java::ArrayList<float> colors;

    for ( long i = 0; i < numberOfVertexes; i++ ) {
        putRgb(colors, color);
    }
    return colors;
}

java::ArrayList<float> GizmoVertexArrayBuilder::buildRgbaColors(
    long numberOfVertexes, const ColorRgb& color, double alpha)
{
    java::ArrayList<float> colors;

    for ( long i = 0; i < numberOfVertexes; i++ ) {
        putRgba(colors, color, alpha);
    }
    return colors;
}
