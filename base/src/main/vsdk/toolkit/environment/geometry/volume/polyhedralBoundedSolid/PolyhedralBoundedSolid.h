#ifndef __POLYHEDRAL_BOUNDED_SOLID__
#define __POLYHEDRAL_BOUNDED_SOLID__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class Ray;
class RayHit;
class Vector3Dd;
class InfinitePlane;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidVertex;

class PolyhedralBoundedSolid : public Solid {
public:
    static const int PLUS = 1;
    static const int MINUS = 0;

private:
    java::ArrayList<_PolyhedralBoundedSolidFace*> polygonsList;
    java::ArrayList<_PolyhedralBoundedSolidEdge*> edgesList;
    java::ArrayList<_PolyhedralBoundedSolidVertex*> verticesList;
    int maxVertexId;
    int maxFaceId;
    bool modelIsValid;

    InfinitePlane** queryPlaneCache;
    double* queryFaceAabb;
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext* queryNumericContext;
    int queryFaceCount;

    void computeFaceAabb(_PolyhedralBoundedSolidFace* face, int index);
    bool rayReachesFaceAabb(
        const Vector3Dd& origin,
        double dirX,
        double dirY,
        double dirZ,
        double maxT,
        int faceIndex,
        double pad);
    void clearVisibilityQueryCache();

public:
    PolyhedralBoundedSolid();
    virtual ~PolyhedralBoundedSolid() override;

    _PolyhedralBoundedSolidFace* findFace(int id);
    _PolyhedralBoundedSolidVertex* findVertex(int id);

    java::ArrayList<_PolyhedralBoundedSolidFace*>& getPolygonsList();
    void setPolygonsList(java::ArrayList<_PolyhedralBoundedSolidFace*>& polygonsList);
    java::ArrayList<_PolyhedralBoundedSolidEdge*>& getEdgesList();
    void setEdgesList(java::ArrayList<_PolyhedralBoundedSolidEdge*>& edgesList);
    java::ArrayList<_PolyhedralBoundedSolidVertex*>& getVerticesList();
    void setVerticesList(java::ArrayList<_PolyhedralBoundedSolidVertex*>& verticesList);

    int getMaxVertexId() const;
    void setMaxVertexId(int maxVertexId);
    int getMaxFaceId() const;
    void setMaxFaceId(int maxFaceId);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
    virtual double* getMinMax() override;

    bool isValid() const;
    void setValidationState(bool flag);

    void merge(PolyhedralBoundedSolid* other);

    void beginVisibilityQueries();
    void endVisibilityQueries();
    bool visibilityQueriesActive() const;
    InfinitePlane* cachedFacePlane(int faceIndex);
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext queryToleranceContext();
    bool queryRayReachesFace(
        const Vector3Dd& origin,
        double dirX,
        double dirY,
        double dirZ,
        double maxT,
        int faceIndex,
        double pad);
    bool queryPointNearFace(
        const Vector3Dd& point,
        int faceIndex,
        double pad);

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;
    virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p) override;
    static int compareValue(double a, double b, double tolerance);
    void revert();

    /**
    @return a textual dump of the vertices, edges, faces and loops of the
    solid, as Java `toString`
    */
    java::String toString();

    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;
};

#endif
