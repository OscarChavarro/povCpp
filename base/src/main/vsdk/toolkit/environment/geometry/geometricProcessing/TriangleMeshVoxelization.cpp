#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/volume/VoxelVolume.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshVoxelization.h"
void TriangleMeshVoxelization::doVoxelization(
    TriangleMesh& mesh,
    VoxelVolume& vv,
    const Matrix4x4d& M,
    ProgressMonitor*)
{
    Matrix4x4d Minv = M.inverse();
    double triangleMinmax[6];
    java::ArrayList<int>& triangleIndices = mesh.getTriangleIndexes();
    java::ArrayList<double>& vertexPositions = mesh.getVertexPositions();

    for (int t = 0; t < mesh.getNumTriangles(); t++) {
        Vector3Dd p0Geom(vertexPositions[3*triangleIndices[3*t+0]+0], vertexPositions[3*triangleIndices[3*t+0]+1], vertexPositions[3*triangleIndices[3*t+0]+2]);
        Vector3Dd p1Geom(vertexPositions[3*triangleIndices[3*t+1]+0], vertexPositions[3*triangleIndices[3*t+1]+1], vertexPositions[3*triangleIndices[3*t+1]+2]);
        Vector3Dd p2Geom(vertexPositions[3*triangleIndices[3*t+2]+0], vertexPositions[3*triangleIndices[3*t+2]+1], vertexPositions[3*triangleIndices[3*t+2]+2]);

        Vector3Dd p0Volume = Minv.multiply(p0Geom);
        Vector3Dd p1Volume = Minv.multiply(p1Geom);
        Vector3Dd p2Volume = Minv.multiply(p2Geom);
        Triangle::minMax(p0Volume, p1Volume, p2Volume, triangleMinmax);

        int minI = vv.getNearestIFromX(triangleMinmax[0]);
        int minJ = vv.getNearestJFromY(triangleMinmax[1]);
        int minK = vv.getNearestKFromZ(triangleMinmax[2]);
        int maxI = vv.getNearestIFromX(triangleMinmax[3]);
        int maxJ = vv.getNearestJFromY(triangleMinmax[4]);
        int maxK = vv.getNearestKFromZ(triangleMinmax[5]);

        double distanceTolerance = 2.0 / (double)vv.getXSize();
        for (int i = minI; i <= maxI; i++) {
            for (int j = minJ; j <= maxJ; j++) {
                for (int k = minK; k <= maxK; k++) {
                    Vector3Dd pVolume = vv.getVoxelPosition(i, j, k);
                    int status = Triangle::containmentTest(p0Volume, p1Volume, p2Volume, pVolume, distanceTolerance);
                    if (status != TriangleMesh::OUTSIDE) vv.putVoxel(i, j, k, (char)255);
                }
            }
        }
    }
}
