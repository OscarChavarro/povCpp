#ifndef __TRIANGLE_MESH_GROUP__
#define __TRIANGLE_MESH_GROUP__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
class Ray;
class RayHit;

class TriangleMeshGroup : public Surface {
private:
    java::ArrayList<TriangleMesh> meshes;
    int intersectionMeshIndex;
    int intersectionTriangleIndex;

    double* computeMinMaxPositions();

public:
    TriangleMeshGroup();
    TriangleMeshGroup(const java::ArrayList<TriangleMesh>& meshes);
    TriangleMeshGroup(const TriangleMeshGroup& group);

    java::ArrayList<TriangleMesh>& getMeshes();
    void setMeshes(const java::ArrayList<TriangleMesh>& meshes);
    void addMesh(const TriangleMesh& mesh);
    TriangleMesh& getMeshAt(int index);

    virtual double* getMinMax();
    void calculateMinMaxPositions();

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    int* doIntersectionInformation();

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);

    java::String toString() const;

    TriangleMeshGroup* exportToTriangleMeshGroup();
};

#endif
