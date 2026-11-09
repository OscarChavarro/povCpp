#ifndef __GIZMO_VERTEX_ARRAY_BUILDER__
#define __GIZMO_VERTEX_ARRAY_BUILDER__

#include "java/util/Collections.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
Packs the geometry the gizmos generate (arrays of `Vector3Dd` points in world
space, or in the canonical space of the gizmo) into the flat `float` arrays
every rasterization technology consumes as vertex attributes.

This class does not depend on any particular visualization technology: it only
defines the two interleaving-free layouts the gizmo renderers use, positions as
`{x, y, z}` per vertex, and colors as `{r, g, b}` or `{r, g, b, a}` per vertex.

Unlike the Java version, whose `put*` methods write at a vertex index of a
preallocated array, the `put*` methods here append the vertex at the end of
the array.
*/
class GizmoVertexArrayBuilder {
public:
    /**
    Appends a point to a positions array.
    @param positions array with 3 floats per vertex
    @param point point to write
    */
    static void putVertex(java::ArrayList<float>& positions, const Vector3Dd& point);

    /**
    Appends a color to an opaque colors array.
    @param colors array with 3 floats per vertex
    @param color color to write
    */
    static void putRgb(java::ArrayList<float>& colors, const ColorRgb& color);

    /**
    Appends a color to a colors array with opacity.
    @param colors array with 4 floats per vertex
    @param color color to write
    @param alpha opacity of the vertex, in [0, 1]
    */
    static void putRgba(java::ArrayList<float>& colors, const ColorRgb& color,
                        double alpha);

    /**
    @param points points of the primitive, in the order they are drawn
    @return a positions array with the given points
    */
    static java::ArrayList<float> buildPositions(
        const java::ArrayList<Vector3Dd>& points);

    /**
    @param numberOfVertexes number of vertexes of the primitive
    @param color color of every vertex
    @return an opaque colors array, with 3 floats per vertex
    */
    static java::ArrayList<float> buildRgbColors(long numberOfVertexes, const ColorRgb& color);

    /**
    @param numberOfVertexes number of vertexes of the primitive
    @param color color of every vertex
    @param alpha opacity of every vertex, in [0, 1]
    @return a colors array with opacity, with 4 floats per vertex
    */
    static java::ArrayList<float> buildRgbaColors(long numberOfVertexes,
        const ColorRgb& color, double alpha);
};

#endif
