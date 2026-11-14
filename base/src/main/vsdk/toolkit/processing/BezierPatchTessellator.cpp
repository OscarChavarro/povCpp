#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/processing/BezierPatchTessellator.h"

double BezierPatchTessellator::bernstein(int k, double t)
{
    const double s = 1.0 - t;
    if (k == 0) return s*s*s;
    if (k == 1) return 3.0*t*s*s;
    if (k == 2) return 3.0*t*t*s;
    return t*t*t;
}

double BezierPatchTessellator::bernsteinDerivative(int k, double t)
{
    const double s = 1.0 - t;
    if (k == 0) return -3.0*s*s;
    if (k == 1) return 3.0*s*s - 6.0*t*s;
    if (k == 2) return 6.0*t*s - 3.0*t*t;
    return 3.0*t*t;
}

Vector3Dd BezierPatchTessellator::surfaceNormal(const SubdividedBezierPatch& patch, double u, double v)
{
    Vector3Dd su(0, 0, 0), sv(0, 0, 0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vector3Dd& p = patch.getControlPoint(i, j);
            su = su.add(p.multiply(bernsteinDerivative(i, u)*bernstein(j, v)));
            sv = sv.add(p.multiply(bernstein(i, u)*bernsteinDerivative(j, v)));
        }
    }
    const Vector3Dd n = su.crossProduct(sv);
    const double size = n.length();
    if (!(size > 1e-300) || !java::Double::isFinite(size)) return Vector3Dd(0, 0, 0);
    return n.multiply(1.0/size);
}

TriangleMesh* BezierPatchTessellator::tessellate(const SubdividedBezierPatch& patch,
    int uSteps, int vSteps)
{
    if (uSteps < 1 || vSteps < 1 || uSteps > MAXIMUM_STEPS || vSteps > MAXIMUM_STEPS) return nullptr;
    const int columns = vSteps + 1;
    const int vertexCount = (uSteps + 1)*columns;
    const int triangleCount = 2*uSteps*vSteps;
    TriangleMesh* mesh = new TriangleMesh();
    mesh->initVertexPositionsArray(vertexCount);
    mesh->initVertexNormalsArray();
    mesh->initVertexUvsArray();
    java::ArrayList<double>& positions = mesh->getVertexPositions();
    java::ArrayList<double>& normals = mesh->getVertexNormals();
    java::ArrayList<double>& uvs = mesh->getVertexUvs();
    for (int i = 0; i <= uSteps; i++) {
        const double u = (double)i/uSteps;
        for (int j = 0; j <= vSteps; j++) {
            const double v = (double)j/vSteps;
            const int index = i*columns + j;
            const Vector3Dd p = patch.evaluate(u, v);
            const Vector3Dd n = surfaceNormal(patch, u, v);
            positions[index*3] = p.x();
            positions[index*3 + 1] = p.y();
            positions[index*3 + 2] = p.z();
            normals[index*3] = n.x();
            normals[index*3 + 1] = n.y();
            normals[index*3 + 2] = n.z();
            uvs[index*2] = u;
            uvs[index*2 + 1] = v;
        }
    }
    mesh->initTriangleArrays(triangleCount);
    java::ArrayList<int>& indices = mesh->getTriangleIndexes();
    java::ArrayList<double>& faceNormals = mesh->getTriangleNormals();
    java::ArrayList<Vector3Dd> faceSums;
    faceSums.reserve(vertexCount);
    for (int index = 0; index < vertexCount; index++) faceSums.add(Vector3Dd(0, 0, 0));
    int t = 0;
    for (int i = 0; i < uSteps; i++) {
        for (int j = 0; j < vSteps; j++) {
            const int a = i*columns + j, b = (i + 1)*columns + j;
            const int c = (i + 1)*columns + j + 1, d = i*columns + j + 1;
            // (a, b, c) runs along +u then +v: its normal is u x v.
            const int corners[2][3] = {{a, b, c}, {a, c, d}};
            for (int h = 0; h < 2; h++, t++) {
                Vector3Dd p[3];
                for (int k = 0; k < 3; k++) {
                    const int v = corners[h][k];
                    indices[t*3 + k] = v;
                    p[k] = Vector3Dd(positions[v*3], positions[v*3 + 1], positions[v*3 + 2]);
                }
                Vector3Dd n = p[1].subtract(p[0]).crossProduct(p[2].subtract(p[0]));
                const double size = n.length();
                if (size > 0) n = n.multiply(1.0/size);
                faceNormals[t*3] = n.x();
                faceNormals[t*3 + 1] = n.y();
                faceNormals[t*3 + 2] = n.z();
                for (int k = 0; k < 3; k++) faceSums[corners[h][k]] = faceSums[corners[h][k]].add(n);
            }
        }
    }
    // Where S_u x S_v vanishes (a collapsed edge), use the triangles' normals.
    for (int index = 0; index < vertexCount; index++) {
        if (normals[index*3] != 0 || normals[index*3 + 1] != 0 || normals[index*3 + 2] != 0) continue;
        const double size = faceSums[index].length();
        if (!(size > 0)) continue;
        const Vector3Dd n = faceSums[index].multiply(1.0/size);
        normals[index*3] = n.x();
        normals[index*3 + 1] = n.y();
        normals[index*3 + 2] = n.z();
    }
    return mesh;
}
