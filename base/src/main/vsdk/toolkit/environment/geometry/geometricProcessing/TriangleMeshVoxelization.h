#ifndef __TRIANGLE_MESH_VOXELIZATION__
#define __TRIANGLE_MESH_VOXELIZATION__

class TriangleMesh;
class VoxelVolume;
class Matrix4x4d;
class ProgressMonitor;

class TriangleMeshVoxelization {
public:
    static void doVoxelization(
        TriangleMesh& mesh,
        VoxelVolume& vv,
        const Matrix4x4d& M,
        ProgressMonitor* reporter);
};

#endif
