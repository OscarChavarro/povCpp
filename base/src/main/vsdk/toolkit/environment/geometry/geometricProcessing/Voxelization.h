#ifndef __VOXELIZATION__
#define __VOXELIZATION__

class Geometry;
class VoxelVolume;
class Matrix4x4d;
class ProgressMonitor;

class Voxelization {
public:
    static void doVoxelization(
        Geometry& geometry,
        VoxelVolume& vv,
        const Matrix4x4d& M,
        ProgressMonitor* reporter);
};

#endif
