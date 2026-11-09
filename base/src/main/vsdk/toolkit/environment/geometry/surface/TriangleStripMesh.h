#ifndef __TRIANGLE_STRIP_MESH__
#define __TRIANGLE_STRIP_MESH__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class Vertex;
class Ray;
class RayHit;

class TriangleStripMesh : public Surface {
private:
    // Basic mesh data model
    java::String name;
    java::ArrayList<Vertex> vertexes;
    java::ArrayList< java::ArrayList<int> > strips;

    double* calculateMinMaxPositions();

public:
    TriangleStripMesh();

    virtual double* getMinMax();

    const java::ArrayList<Vertex>& getVertexes() const;
    Vertex getVertexAt(int index) const;
    void setVertexes(const java::ArrayList<Vertex>& vertexes);

    void setStrips(const java::ArrayList< java::ArrayList<int> >& indexes);
    java::ArrayList< java::ArrayList<int> >& getStrips();

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);
};

#endif
