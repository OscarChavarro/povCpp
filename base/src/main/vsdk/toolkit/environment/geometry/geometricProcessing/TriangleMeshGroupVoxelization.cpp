#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshGroupVoxelization.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshVoxelization.h"
void TriangleMeshGroupVoxelization::doVoxelization(
    TriangleMeshGroup& meshGroup,
    VoxelVolume& vv,
    const Matrix4x4d& M,
    ProgressMonitor* reporter)
{
    java::ArrayList<TriangleMesh>& meshes = meshGroup.getMeshes();
    for (long int i = 0; i < meshes.size(); i++) {
        TriangleMeshVoxelization::doVoxelization(meshes[i], vv, M, reporter);
    }
}
