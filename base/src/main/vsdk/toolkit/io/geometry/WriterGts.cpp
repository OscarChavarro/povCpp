#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/WriterCommon.h"
#include "vsdk/toolkit/io/geometry/WriterGts.h"

namespace {

class WriterGtsEdge {
public:
    int from;
    int to;
    WriterGtsEdge() : from(0), to(0) {}
};

class WriterGtsTriangle {
public:
    int point0;
    int point1;
    int point2;
    WriterGtsTriangle() : point0(0), point1(0), point2(0) {}
};

void line(java::OutputStream& os, const java::String& text)
{
    PersistenceElement::writeAsciiLine(os, text.c_str());
}

int addEdge(java::ArrayList<WriterGtsEdge>& edges, int from, int to)
{
    int i;

    for ( i = 0; i < edges.size(); i++ ) {
        const WriterGtsEdge& e = edges[i];
        if ( (e.from == from && e.to == to) ||
             (e.from == to && e.to == from) ) {
            return i;
        }
    }

    i = (int)edges.size();
    WriterGtsEdge e;
    e.from = from;
    e.to = to;
    edges.add(e);
    return i;
}

long long exportMesh(java::OutputStream& os, TriangleMesh* mesh,
                     long long offset)
{
    int nv = mesh->getNumVertices();
    int nt = mesh->getNumTriangles();
    java::ArrayList<WriterGtsEdge> edges;
    java::ArrayList<WriterGtsTriangle> triangles;
    int i;

    //- Compute edges -------------------------------------------------
    java::ArrayList<int>& t = mesh->getTriangleIndexes();
    for ( i = 0; i < nt; i++ ) {
        WriterGtsTriangle tt;
        tt.point0 = addEdge(edges, t[3*i], t[3*i+1]);
        tt.point1 = addEdge(edges, t[3*i+1], t[3*i+2]);
        tt.point2 = addEdge(edges, t[3*i+2], t[3*i]);
        triangles.add(tt);
    }

    //- Write GTS header ----------------------------------------------
    line(os, java::String::valueOf(nv) + " " +
         java::String::valueOf((long)edges.size()) + " " +
         java::String::valueOf((long)triangles.size()) +
         " GtsSurface GtsFace GtsEdge GtsVertex");

    //- Write vertices ------------------------------------------------
    java::ArrayList<double>& v = mesh->getVertexPositions();
    for ( i = 0; i < nv; i++ ) {
        line(os, java::Double::toString(v[3*i]) + " " +
             java::Double::toString(v[3*i+1]) + " " +
             java::Double::toString(v[3*i+2]));
    }

    //- Write edges ---------------------------------------------------
    for ( i = 0; i < edges.size(); i++ ) {
        const WriterGtsEdge& e = edges[i];
        line(os, java::String::valueOf(e.from+1) + " " +
             java::String::valueOf(e.to+1));
    }

    //- Write triangles -----------------------------------------------
    for ( i = 0; i < triangles.size(); i++ ) {
        const WriterGtsTriangle& tt = triangles[i];
        line(os, java::String::valueOf(tt.point0+1) + " " +
             java::String::valueOf(tt.point1+1) + " " +
             java::String::valueOf(tt.point2+1));
    }

    return offset + nv;
}

}

void WriterGts::exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene)
{
    java::ArrayList<SimpleBody*>& objs = scene->getSimpleBodies();
    long long baseVertexStart = 0;
    int i;

    for ( i = 0; i < objs.size(); i++ ) {
        TriangleMesh* mesh = writerMeshOf(objs.get(i)->getGeometry());

        //-----------------------------------------------------------------
        if ( mesh != nullptr ) {
            baseVertexStart += exportMesh(outputStream, mesh, baseVertexStart);
        }
    }
}
