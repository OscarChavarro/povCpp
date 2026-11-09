#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/WriterCommon.h"
#include "vsdk/toolkit/io/geometry/WriterVtk.h"

namespace {

void line(java::OutputStream& os, const java::String& text)
{
    PersistenceElement::writeAsciiLine(os, text.c_str());
}

void exportMesh(java::OutputStream& os, TriangleMesh* mesh)
{
    java::ArrayList<double>& v = mesh->getVertexPositions();
    java::ArrayList<double>& n = mesh->getVertexNormals();
    java::ArrayList<int>& t = mesh->getTriangleIndexes();
    long vLength = v.size();
    long tLength = t.size();

    line(os, "DATASET POLYDATA");

    //-----------------------------------------------------------------
    line(os, java::String("POINTS ") + java::String::valueOf(vLength/3) +
         " float");
    long i;
    float val;
    for ( i = 0; i < vLength; i++ ) {
        val = (float)(v[i]*1000.0);
        PersistenceElement::writeFloatBE(os, val);
    }
    line(os, "");

    //-----------------------------------------------------------------
    line(os, java::String("POLYGONS ") + java::String::valueOf(tLength / 3) +
         " " + java::String::valueOf((tLength/3)*4));
    int p = 3;
    for ( i = 0; i < tLength/3; i++ ) {
        PersistenceElement::writeLongBE(os, p);
        PersistenceElement::writeLongBE(os, t[3*i+0]);
        PersistenceElement::writeLongBE(os, t[3*i+1]);
        PersistenceElement::writeLongBE(os, t[3*i+2]);
    }
    line(os, "");

    //-----------------------------------------------------------------
    if ( n.size() > 0 ) {
        line(os, java::String("CELL_DATA ") +
             java::String::valueOf(tLength / 3));
        line(os, java::String("POINT_DATA ") +
             java::String::valueOf(vLength/3));
        line(os, "NORMALS Normals float");
        for ( i = 0; i < vLength/3; i++ ) {
            PersistenceElement::writeFloatBE(os, (float)(n[3*i+0]));
            PersistenceElement::writeFloatBE(os, (float)(n[3*i+1]));
            PersistenceElement::writeFloatBE(os, (float)(n[3*i+2]));
        }
        line(os, "");
    }
    os.flush();
}

}

void WriterVtk::exportEnvironment(java::OutputStream& outputStream,
                                  SimpleScene* scene)
{
    // C++ port note: the Java version wraps the stream in a
    // BufferedOutputStream and closes it after the mesh; here the stream is
    // written directly and flushed, and the caller closes it.
    java::OutputStream& bos = outputStream;

    //-----------------------------------------------------------------
    line(bos, "# vtk DataFile Version 3.0");
    line(bos, "vtk output");
    line(bos, "BINARY");

    //-----------------------------------------------------------------
    java::ArrayList<SimpleBody*>& objs = scene->getSimpleBodies();
    bool exported = false;
    int i;

    for ( i = 0; i < objs.size(); i++ ) {
        TriangleMesh* mesh = writerMeshOf(objs.get(i)->getGeometry());
        if ( mesh == nullptr ) {
            Logger::reportMessage("WriterVtk", Logger::WARNING,
                "WriterVtk.exportEnvironment",
                "Current writer implementation only supports writing of triangle meshes. Object skipped.");
        }

        //-----------------------------------------------------------------
        if ( mesh != nullptr ) {
            if ( !exported ) {
                exportMesh(bos, mesh);
                exported = true;
            }
            else {
                Logger::reportMessage("WriterVtk", Logger::WARNING,
                    "WriterVtk.exportEnvironment",
                    "Current writer implementation only supports writing ONE triangle meshes. Only first mesh exported, remaining meshes skipped.");
            }
        }
    }
}
