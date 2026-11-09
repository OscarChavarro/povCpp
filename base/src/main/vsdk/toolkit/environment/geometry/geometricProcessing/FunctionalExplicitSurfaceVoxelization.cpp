#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/FunctionalExplicitSurfaceVoxelization.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshVoxelization.h"
void FunctionalExplicitSurfaceVoxelization::doVoxelization(
    FunctionalExplicitSurface& surface,
    VoxelVolume& vv,
    const Matrix4x4d& M,
    ProgressMonitor* reporter)
{
    TriangleMesh* mesh = surface.getInternalTriangleMesh();
    if ( mesh == 0 ) return;
    TriangleMeshVoxelization::doVoxelization(*mesh, vv, M, reporter);
}
