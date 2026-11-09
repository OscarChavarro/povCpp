#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/volume/VoxelVolume.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/FunctionalExplicitSurfaceVoxelization.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshGroupVoxelization.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshVoxelization.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/Voxelization.h"
void Voxelization::doVoxelization(
    Geometry& geometry,
    VoxelVolume& vv,
    const Matrix4x4d& M,
    ProgressMonitor* reporter)
{
    if ( TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(&geometry) ) {
        TriangleMeshVoxelization::doVoxelization(*mesh, vv, M, reporter);
        return;
    }
    if ( TriangleMeshGroup* meshGroup = dynamic_cast<TriangleMeshGroup*>(&geometry) ) {
        TriangleMeshGroupVoxelization::doVoxelization(*meshGroup, vv, M, reporter);
        return;
    }
    if ( FunctionalExplicitSurface* surface = dynamic_cast<FunctionalExplicitSurface*>(&geometry) ) {
        FunctionalExplicitSurfaceVoxelization::doVoxelization(*surface, vv, M, reporter);
        return;
    }

    int nx = vv.getXSize();
    int ny = vv.getYSize();
    int nz = vv.getZSize();
    int nmax;
    double* minmax = geometry.getMinMax();
    double greaterScale, sx, sy, sz;

    sx = minmax[3]-minmax[0];
    sy = minmax[4]-minmax[1];
    sz = minmax[5]-minmax[2];
    greaterScale = sx;
    if ( sy > greaterScale ) {
        greaterScale = sy;
    }
    if ( sz > greaterScale ) {
        greaterScale = sz;
    }

    delete [] minmax;

    nmax = nx;
    if ( ny > nmax ) nmax = ny;
    if ( nz > nmax ) nmax = nz;
    int containmentStatus;
    int x, y, z;
    Vector3Dd p;
    Vector3Dd transformedP;

    if ( reporter != 0 ) {
        reporter->begin();
    }
    for ( x = 0; x < nx; x++ ) {
        for ( y = 0; y < ny; y++ ) {
            if ( reporter != 0 ) {
                reporter->update(0, nx*ny, x*ny);
            }
            for ( z = 0; z < nz; z++ ) {
                p = vv.getVoxelPosition(x, y, z);
                transformedP = M.multiply(p);
                containmentStatus = geometry.doContainmentTest(
                        transformedP, (1/((double)nmax) * greaterScale));
                if ( containmentStatus == Geometry::INSIDE ||
                     containmentStatus == Geometry::LIMIT ) {
                    vv.putVoxel(x, y, z, (char)-1);
                }
            }
        }
    }
    if ( reporter != 0 ) {
        reporter->end();
    }
}
