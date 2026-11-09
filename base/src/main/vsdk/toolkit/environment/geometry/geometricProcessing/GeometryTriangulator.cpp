#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/QuadMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/GeometryTriangulator.h"
bool GeometryTriangulator::exportToTriangleMeshGroup(Geometry* geometry, TriangleMeshGroup& outGroup)
{
    if ( geometry == 0 ) {
        return false;
    }

    if ( TriangleMeshGroup* group = dynamic_cast<TriangleMeshGroup*>(geometry) ) {
        outGroup = *group;
        return true;
    }

    if ( TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(geometry) ) {
        outGroup.getMeshes().clear();
        outGroup.addMesh(*mesh);
        return true;
    }

    if ( QuadMesh* quadMesh = dynamic_cast<QuadMesh*>(geometry) ) {
        TriangleMeshGroup* tmp = quadMesh->exportToTriangleMeshGroup();
        if ( tmp == 0 ) return false;
        outGroup = *tmp;
        delete tmp;
        return true;
    }

    if ( FunctionalExplicitSurface* surface = dynamic_cast<FunctionalExplicitSurface*>(geometry) ) {
        TriangleMesh* mesh = surface->getInternalTriangleMesh();
        if ( mesh == 0 ) return false;
        outGroup.getMeshes().clear();
        outGroup.addMesh(*mesh);
        return true;
    }

    return false;
}
