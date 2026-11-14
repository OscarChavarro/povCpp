#include "java/util/ArrayList.txx"
#include "java/util/TreeMap.txx"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "TessellationMeshCheck.h"

Vector3Dd TessellationMeshCheck::vertex(const TriangleMesh& mesh, int index)
{
    const java::ArrayList<double>& p = mesh.getVertexPositionsView();
    return Vector3Dd(p[index*3], p[index*3 + 1], p[index*3 + 2]);
}

bool TessellationMeshCheck::isClosedAndOriented(const TriangleMesh& mesh)
{
    const java::ArrayList<int>& t = mesh.getTriangleIndexesView();
    const long int n = mesh.getVertexPositionsView().size()/3;
    java::TreeMap<long int, int> directed;
    for (long int i = 0; i < t.size(); i += 3) {
        for (int k = 0; k < 3; k++) {
            const long int a = t[i + k], b = t[i + (k + 1)%3];
            if (a == b) return false;
            // A repeated directed edge is a fold or a third sheet on the edge.
            if (!directed.put(a*n + b, 1)) return false;
        }
    }
    for (long int e = 0; e < directed.size(); e++) {
        const long int key = directed.getKey(e);
        if (!directed.containsKey((key%n)*n + key/n)) return false;
    }
    return !directed.isEmpty();
}

double TessellationMeshCheck::enclosedVolume(const TriangleMesh& mesh)
{
    const java::ArrayList<int>& t = mesh.getTriangleIndexesView();
    double volume = 0;
    for (long int i = 0; i < t.size(); i += 3) {
        const Vector3Dd a = vertex(mesh, t[i]), b = vertex(mesh, t[i + 1]), c = vertex(mesh, t[i + 2]);
        volume += a.dotProduct(b.crossProduct(c))/6.0;
    }
    return volume;
}

double TessellationMeshCheck::area(const TriangleMesh& mesh)
{
    const java::ArrayList<int>& t = mesh.getTriangleIndexesView();
    double total = 0;
    for (long int i = 0; i < t.size(); i += 3) {
        const Vector3Dd a = vertex(mesh, t[i]), b = vertex(mesh, t[i + 1]), c = vertex(mesh, t[i + 2]);
        total += b.subtract(a).crossProduct(c.subtract(a)).length()/2.0;
    }
    return total;
}

long int TessellationMeshCheck::eulerCharacteristic(const TriangleMesh& mesh)
{
    const java::ArrayList<int>& t = mesh.getTriangleIndexesView();
    const long int n = mesh.getVertexPositionsView().size()/3;
    java::TreeMap<long int, int> edges;
    for (long int i = 0; i < t.size(); i += 3) {
        for (int k = 0; k < 3; k++) {
            long int a = t[i + k], b = t[i + (k + 1)%3];
            if (a > b) { const long int s = a; a = b; b = s; }
            edges.put(a*n + b, 1);
        }
    }
    return n - edges.size() + (long int)(t.size()/3);
}
