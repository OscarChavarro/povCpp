#ifndef VSDK_TESSELLATION_MESH_CHECK_H
#define VSDK_TESSELLATION_MESH_CHECK_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class TriangleMesh;

// Properties of tessellation results checked by the tests.
class TessellationMeshCheck {
public:
    // Every directed edge appears once and its reverse once: the mesh is
    // closed and its triangles are consistently wound.
    static bool isClosedAndOriented(const TriangleMesh& mesh);
    // Signed volume enclosed by the triangles (positive when they face out).
    static double enclosedVolume(const TriangleMesh& mesh);
    static double area(const TriangleMesh& mesh);
    static Vector3Dd vertex(const TriangleMesh& mesh, int index);
    // Vertices minus edges plus triangles.
    static long int eulerCharacteristic(const TriangleMesh& mesh);
};

#endif
