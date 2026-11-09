#ifndef __TRIANGLE_MESH__
#define __TRIANGLE_MESH__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class Vertex;
class Triangle;
class SimpleMaterial;
class Image;
class Ray;
class RayHit;
class TriangleMeshGroup;
class InfinitePlane;

class TriangleMesh : public Surface {
private:
    java::String name;

    java::ArrayList<double> vertexPositions;
    java::ArrayList<double> vertexNormals;
    java::ArrayList<double> vertexBinormals;
    java::ArrayList<double> vertexTangents;
    java::ArrayList<double> vertexColors;
    java::ArrayList<bool> vertexSelections;
    java::ArrayList<double> vertexUvs;

    java::ArrayList< java::ArrayList<int> > incidentTrianglesPerVertexArray;

    java::ArrayList<int> triangleIndexes;
    java::ArrayList<double> triangleNormals;

    java::ArrayList<SimpleMaterial*> materials;
    java::ArrayList<Image*> textures;

    java::ArrayList< java::ArrayList<int> > textureRanges;
    java::ArrayList< java::ArrayList<int> > materialRanges;

    int intersectionTriangleIndex;
    bool ownsMaterials;

    static bool intersectTriangle(const Ray& ray, const Vector3Dd& v0, const Vector3Dd& v1, const Vector3Dd& v2, double& t, double& u, double& v);
    void interpolateTriangleData(int triangleIndex, double u, double v, RayHit* outHit, const Vector3Dd& rayDirection);
    void fillMaterialAndTexture(int triangleIndex, RayHit* outHit);
    bool doIntersectionInternal(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);

    double* calculateMinMaxPositions();

    void appendVertices(const java::ArrayList<double>& ev);
    void appendTriangles(const java::ArrayList<int>& et);

    void simpleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                           java::ArrayList<int>& extraTriangles, int nv, int i,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void halfTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                         java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                         const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);
    void doubleTriangleCut(InfinitePlane& p, java::ArrayList<double>& extraVertices,
                           java::ArrayList<int>& extraTriangles, int nv, int i, int j,
                           const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p3);

public:
    TriangleMesh();
    TriangleMesh(const TriangleMesh& other);
    ~TriangleMesh();

    TriangleMesh* clone() const;

    java::String getName() const;
    void setName(const java::String& name);

    void getVertexAt(int i, Vertex& vertex) const;

    void initVertexPositionsArray(int n);
    void initVertexNormalsArray();
    void initVertexBinormalsArray();
    void initVertexTangentsArray();
    void initVertexColorsArray();
    void initVertexUvsArray();
    void initIncidentTrianglesPerVertexArray();

    void detachColors();
    void detachNormals();
    void detachUvs();

    void setVertexes(const java::ArrayList<Vertex>& vertexes, bool withNormals, bool withBinormals, bool withTangents, bool withUvs);

    void initTriangleArrays(int n);
    void setTriangles(const java::ArrayList<Triangle>& triangles);

    void setTextures(const java::ArrayList<Image*>& textures);
    void setMaterials(const java::ArrayList<SimpleMaterial*>& materials);
    void setOwnsMaterials(bool owns);

    void setVertexAt(int i, const Vertex& vertex);
    void setTriangleAt(int i, const Triangle& triangle);

    int getNumVertices() const;
    int getNumTriangles() const;

    java::ArrayList<bool>& getVertexSelections();

    java::ArrayList<double>& getVertexPositions();
    java::ArrayList<double>& getVertexNormals();
    java::ArrayList<double>& getVertexBinormals();
    java::ArrayList<double>& getVertexTangents();
    java::ArrayList<double>& getVertexColors();
    java::ArrayList<double>& getVertexUvs();
    java::ArrayList<int>& getTriangleIndexes();
    java::ArrayList<double>& getTriangleNormals();

    java::ArrayList<SimpleMaterial*>& getMaterials();
    java::ArrayList<Image*>& getTextures();
    Image* getTextureAt(int index) const;
    void setTextureAt(int index, Image* image);

    java::ArrayList< java::ArrayList<int> >& getTextureRanges();
    java::ArrayList<int> getTextureRangeAt(int spanRange) const;
    void setTextureRanges(const java::ArrayList< java::ArrayList<int> >& ranges);

    java::ArrayList< java::ArrayList<int> >& getMaterialRanges();
    java::ArrayList<int> getMaterialRangeAt(int spanRange) const;
    void setMaterialRanges(const java::ArrayList< java::ArrayList<int> >& ranges);

    void calculateNormals();
    void reorientateNormals();

    int doIntersectionInformation() const;

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit, int* outTriangleIndex);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
    virtual double* getMinMax();

    TriangleMeshGroup* exportToTriangleMeshGroup();

    void compact();
    java::String toString() const;

    void removeSelectedVertices();
    void slice(InfinitePlane& p);
};

#endif
