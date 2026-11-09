#ifndef __GIZMO_SOLID_TESSELLATOR__
#define __GIZMO_SOLID_TESSELLATOR__

#include "java/util/Collections.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class SimpleBody;

/**
Tessellates, in world space, the simple solids the gizmos are built with (the
shafts and heads of their axes, the small cubes at their tips and their plane
handles), so the technology-dependent renderers only have to send the resulting
points to the rasterizer.

Each element of a gizmo is a `SimpleBody` whose geometry is expressed in
its own local frame, and grows along its local +Z (see
`localTransform(SimpleBody*)`); every method here receives that local
frame as a matrix and returns points already transformed by it.

The primitives returned follow these conventions:

- A *strip* is a sequence of points for a triangle strip.
- A *fan* is a sequence of points for a triangle fan, with its center or apex
  as the first point.
*/
class GizmoSolidTessellator {
public:
    /// Number of subdivisions used around the axis of a revolution solid
    static const int SLICES = 16;

    /// Number of faces of a box, as given by `buildBoxFaceStrips`
    static const int BOX_NUMBER_OF_FACES = 6;

    /**
    @param element element of a gizmo
    @return the matrix that takes the local frame of the element to world
    space
    */
    static Matrix4x4d localTransform(const SimpleBody* element);

    /**
    Tessellates the lateral surface of a shaft: a truncated cone from the
    local origin, growing along the local +Z axis.

    @param local local frame of the element
    @param baseRadius radius at the local origin
    @param topRadius radius at the local height
    @param height length of the shaft, along the local +Z axis
    @return strip with the lateral surface
    */
    static java::ArrayList<Vector3Dd> buildShaftStrip(const Matrix4x4d& local,
        double baseRadius, double topRadius, double height);

    /**
    Tessellates the lateral surface of a cone whose base is at the local
    origin and whose apex is at the local height, over the +Z axis.

    @param local local frame of the element
    @param radius radius of the base of the cone
    @param height distance from the base to the apex
    @return fan with the apex as its first point
    */
    static java::ArrayList<Vector3Dd> buildConeSideFan(const Matrix4x4d& local,
        double radius, double height);

    /**
    Tessellates the base of a cone built by `buildConeSideFan`, with
    the opposite orientation, as it is seen from the other side.

    @param local local frame of the element
    @param radius radius of the base of the cone
    @return fan with the center of the base as its first point
    */
    static java::ArrayList<Vector3Dd> buildConeBaseFan(const Matrix4x4d& local, double radius);

    /**
    Tessellates a box centered at the local origin, as its 6 faces.

    @param local local frame of the element
    @param size lengths of the sides of the box, over each local axis
    @param outFaces one strip of 4 corners per face of the box
    */
    static void buildBoxFaceStrips(const Matrix4x4d& local, const Vector3Dd& size,
        Vector3Dd outFaces[BOX_NUMBER_OF_FACES][4]);

    /**
    Tessellates a rectangle centered at the local origin, over the local XY
    plane.

    @param local local frame of the element
    @param sizeX length of the rectangle over the local X axis
    @param sizeY length of the rectangle over the local Y axis
    @return strip with the 4 corners of the rectangle
    */
    static java::ArrayList<Vector3Dd> buildPlaneQuad(const Matrix4x4d& local,
        double sizeX, double sizeY);
};

#endif
