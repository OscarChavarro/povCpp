#ifndef __FUNCTIONAL_EXPLICIT_SURFACE_VOXELIZATION__
#define __FUNCTIONAL_EXPLICIT_SURFACE_VOXELIZATION__

class FunctionalExplicitSurface;
class VoxelVolume;
class Matrix4x4d;
class ProgressMonitor;

class FunctionalExplicitSurfaceVoxelization {
public:
    static void doVoxelization(
        FunctionalExplicitSurface& surface,
        VoxelVolume& vv,
        const Matrix4x4d& M,
        ProgressMonitor* reporter);
};

#endif
