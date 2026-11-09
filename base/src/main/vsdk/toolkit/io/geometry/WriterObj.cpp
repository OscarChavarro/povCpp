#include <cmath>

#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/WriterCommon.h"
#include "vsdk/toolkit/io/geometry/WriterObj.h"

namespace {

void line(java::OutputStream& os, const java::String& text)
{
    PersistenceElement::writeAsciiLine(os, text.c_str());
}

java::String d(double value)
{
    return java::Double::toString(value);
}

long long exportMesh(java::OutputStream& os, TriangleMesh* mesh,
                     long long offset)
{
    int nv = mesh->getNumVertices();
    int nt = mesh->getNumTriangles();
    java::ArrayList<double>& vp = mesh->getVertexPositions();
    java::ArrayList<double>& vn = mesh->getVertexNormals();
    java::ArrayList<double>& vuv = mesh->getVertexUvs();
    bool withNormals = vn.size() > 0;
    bool withUvs = vuv.size() > 0;
    int i;
    Vector3Dd p;
    Vector3Dd n;
    Matrix4x4d R;

    R = R.axisRotation(-M_PI / 2.0, Vector3Dd(1, 0, 0));

    //-----------------------------------------------------------------
    line(os, java::String("# ") + java::String::valueOf(nv) +
         " vertex positions");
    for ( i = 0; i < nv; i++ ) {
        p = R.multiply(Vector3Dd(vp[3*i], vp[3*i+1], vp[3*i+2]));
        line(os, java::String("v ") + d(p.x()) + " " + d(p.y()) + " " +
             d(p.z()));
    }

    //-----------------------------------------------------------------
    line(os, java::String("# ") + java::String::valueOf(nv) +
         " vertex texture coordinates");
    for ( i = 0; i < nv && withUvs; i++ ) {
        line(os, java::String("vt ") + d(vuv[2*i]) + " " + d(vuv[2*i+1]));
    }

    //-----------------------------------------------------------------
    line(os, java::String("# ") + java::String::valueOf(nv) +
         " vertex normals");
    for ( i = 0; i < nv && withNormals; i++ ) {
        n = R.multiply(Vector3Dd(vn[3*i], vn[3*i+1], vn[3*i+2]));
        line(os, java::String("vn ") + d(n.x()) + " " + d(n.y()) + " " +
             d(n.z()));
    }

    //-----------------------------------------------------------------
    java::ArrayList<int>& t = mesh->getTriangleIndexes();

    line(os, java::String("# ") + java::String::valueOf(nt) + " triangles");
    long long n0;
    long long n1;
    long long n2;
    line(os, "o NewObject");
    for ( i = 0; i < nt; i++ ) {
        n0 = t[3*i] + offset + 1;
        n1 = t[3*i+1] + offset + 1;
        n2 = t[3*i+2] + offset + 1;
        java::String a = java::String::valueOf(n0);
        java::String b = java::String::valueOf(n1);
        java::String c = java::String::valueOf(n2);
        if ( withUvs && withNormals ) {
            line(os, java::String("f ") +
                 a + "/" + a + "/" + a + " " +
                 b + "/" + b + "/" + b + " " +
                 c + "/" + c + "/" + c);
        }
        else {
            line(os, java::String("f ") + a + " " + b + " " + c);
        }
    }

    return offset + nv;
}

}

void WriterObj::exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene)
{
    //-----------------------------------------------------------------
    line(outputStream, "# OBJ File generated with VitralSDK.");
    line(outputStream, "# http://sophia.javeriana.edu.co/~ochavarr");
    //-----------------------------------------------------------------
    java::ArrayList<SimpleBody*>& objs = scene->getSimpleBodies();
    long long baseVertexStart = 0;
    int i;

    for ( i = 0; i < objs.size(); i++ ) {
        TriangleMesh* mesh = writerMeshOf(objs.get(i)->getGeometry());

        //-----------------------------------------------------------------
        if ( mesh != nullptr ) {
            // As in the Java version, the offset returned by exportMesh
            // already includes the previous one
            baseVertexStart += exportMesh(outputStream, mesh, baseVertexStart);
        }
    }
}
