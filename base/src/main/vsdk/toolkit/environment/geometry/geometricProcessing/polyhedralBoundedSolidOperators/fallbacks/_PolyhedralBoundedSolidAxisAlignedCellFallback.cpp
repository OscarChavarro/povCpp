#include "java/util/Collections.h"
#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidAxisAlignedCellFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidAxisAlignedCellFallback CellFallback;
typedef _PolyhedralBoundedSolidFallbackGeometry FallbackGeometry;

//= Builder =========================================================

CellFallback::AxisAlignedCellBooleanBuilder::AxisAlignedCellBooleanBuilder(
    const java::ArrayList<double>& inX,
    const java::ArrayList<double>& inY,
    const java::ArrayList<double>& inZ)
    : solid(new PolyhedralBoundedSolid()), xCoordinates(inX),
      yCoordinates(inY), zCoordinates(inZ), nextVertexId(1), nextFaceId(1)
{
}

java::String CellFallback::AxisAlignedCellBooleanBuilder::vertexKey(
    int ix, int iy, int iz)
{
    return java::String::valueOf(ix) + ":" + java::String::valueOf(iy) + ":" +
        java::String::valueOf(iz);
}

_PolyhedralBoundedSolidVertex* CellFallback::AxisAlignedCellBooleanBuilder::vertexAt(
    int ix, int iy, int iz)
{
    java::String key = vertexKey(ix, iy, iz);
    _PolyhedralBoundedSolidVertex** found = vertices.get(key);
    if ( found != nullptr ) {
        return (*found);
    }

    _PolyhedralBoundedSolidVertex* vertex = new _PolyhedralBoundedSolidVertex(
        Vector3Dd(xCoordinates[ix], yCoordinates[iy], zCoordinates[iz]),
        nextVertexId);
    solid->getVerticesList().add(vertex);
    solid->setMaxVertexId(nextVertexId);
    nextVertexId++;
    vertices.put(key, vertex);
    return vertex;
}

java::String CellFallback::AxisAlignedCellBooleanBuilder::edgeKey(
    _PolyhedralBoundedSolidVertex* a, _PolyhedralBoundedSolidVertex* b)
{
    if ( a->id < b->id ) {
        return java::String::valueOf(a->id) + ":" + java::String::valueOf(b->id);
    }
    return java::String::valueOf(b->id) + ":" + java::String::valueOf(a->id);
}

void CellFallback::AxisAlignedCellBooleanBuilder::attachEdge(
    _PolyhedralBoundedSolidHalfEdge* he,
    _PolyhedralBoundedSolidVertex* a,
    _PolyhedralBoundedSolidVertex* b)
{
    java::String key = edgeKey(a, b);
    _PolyhedralBoundedSolidEdge* edge;
    _PolyhedralBoundedSolidEdge** found = edges.get(key);

    if ( found == nullptr ) {
        edge = new _PolyhedralBoundedSolidEdge();
        solid->getEdgesList().add(edge);
        edge->rightHalf = he;
        edges.put(key, edge);
    }
    else {
        edge = (*found);
        if ( edge->leftHalf == 0 ) {
            edge->leftHalf = he;
        }
        else if ( edge->rightHalf == 0 ) {
            edge->rightHalf = he;
        }
    }
    he->parentEdge = edge;
}

void CellFallback::AxisAlignedCellBooleanBuilder::addQuad(const int corners[4][3])
{
    _PolyhedralBoundedSolidFace* face;
    _PolyhedralBoundedSolidLoop* loop;
    _PolyhedralBoundedSolidHalfEdge* halfEdges[4];
    _PolyhedralBoundedSolidVertex* faceVertices[4];
    int i;

    face = new _PolyhedralBoundedSolidFace(solid, nextFaceId);
    solid->getPolygonsList().add(face);
    solid->setMaxFaceId(nextFaceId);
    nextFaceId++;
    loop = new _PolyhedralBoundedSolidLoop(face);

    for ( i = 0; i < 4; i++ ) {
        faceVertices[i] = vertexAt(corners[i][0], corners[i][1], corners[i][2]);
        halfEdges[i] = new _PolyhedralBoundedSolidHalfEdge(faceVertices[i], loop);
        loop->halfEdgesList.add(halfEdges[i]);
        if ( faceVertices[i]->emanatingHalfEdge == 0 ) {
            faceVertices[i]->emanatingHalfEdge = halfEdges[i];
        }
    }
    loop->boundaryStartHalfEdge = halfEdges[0];

    for ( i = 0; i < 4; i++ ) {
        attachEdge(halfEdges[i], faceVertices[i], faceVertices[(i + 1) % 4]);
    }
}

PolyhedralBoundedSolid* CellFallback::AxisAlignedCellBooleanBuilder::result()
{
    return solid;
}

//= Fallback ========================================================

bool CellFallback::isAxisAlignedEdge(_PolyhedralBoundedSolidEdge* edge)
{
    int changingAxes;

    if ( edge == 0 || edge->rightHalf == 0 || edge->leftHalf == 0 ) {
        return false;
    }
    Vector3Dd a = edge->rightHalf->startingVertex->position;
    Vector3Dd b = edge->leftHalf->startingVertex->position;
    changingAxes = 0;
    if ( !FallbackGeometry::sameCoordinate(a.x(), b.x()) ) {
        changingAxes++;
    }
    if ( !FallbackGeometry::sameCoordinate(a.y(), b.y()) ) {
        changingAxes++;
    }
    if ( !FallbackGeometry::sameCoordinate(a.z(), b.z()) ) {
        changingAxes++;
    }
    return changingAxes <= 1;
}

bool CellFallback::isAxisAlignedSolid(PolyhedralBoundedSolid* solid)
{
    long int i;

    if ( solid == 0 || solid->getEdgesList().size() <= 0 ) {
        return false;
    }
    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        if ( !isAxisAlignedEdge(solid->getEdgesList().get(i)) ) {
            return false;
        }
    }
    return true;
}

int CellFallback::classifyPointForAxisAlignedFallback(
    PolyhedralBoundedSolid* solid, const Vector3Dd& point)
{
    java::ArrayList<double> distances;
    long int i;
    int hits;
    double eps;

    if ( solid == 0 || solid->getPolygonsList().size() <= 0 ) {
        return Geometry::OUTSIDE;
    }

    eps = numericContext.bigEpsilon();
    Ray ray(point, Vector3Dd(1.0, 0.371, 0.137));
    hits = 0;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = face->getContainingPlane();
        if ( plane == 0 ) {
            continue;
        }
        Ray rayCopy(ray);
        Ray* hit = plane->doIntersectionFirstHit(rayCopy);
        delete plane;
        if ( hit == 0 ) {
            continue;
        }
        double t = hit->getT();
        Vector3Dd p = hit->getOrigin().add(hit->getDirection().multiply(t));
        delete hit;
        if ( t <= eps ) {
            continue;
        }
        if ( face->testPointInside(p, eps) != Geometry::INSIDE ) {
            continue;
        }

        bool duplicate = false;
        for ( size_t j = 0; j < distances.size(); j++ ) {
            if ( std::fabs(distances[j] - t) <= eps ) {
                duplicate = true;
                break;
            }
        }
        if ( !duplicate ) {
            distances.add(t);
            hits++;
        }
    }

    return (hits % 2) == 1 ? Geometry::INSIDE : Geometry::OUTSIDE;
}

bool CellFallback::axisAlignedCellSelected(bool insideA, bool insideB, int op)
{
    if ( op == UNION ) {
        return insideA || insideB;
    }
    if ( op == INTERSECTION ) {
        return insideA && insideB;
    }
    return insideA && !insideB;
}

void CellFallback::addAxisAlignedBoundaryQuad(
    AxisAlignedCellBooleanBuilder& builder,
    int axis, bool positiveSide, int ix, int iy, int iz)
{
    if ( axis == 0 && !positiveSide ) {
        const int c[4][3] = {
            {ix, iy, iz}, {ix, iy, iz + 1},
            {ix, iy + 1, iz + 1}, {ix, iy + 1, iz} };
        builder.addQuad(c);
    }
    else if ( axis == 0 ) {
        const int c[4][3] = {
            {ix + 1, iy, iz}, {ix + 1, iy + 1, iz},
            {ix + 1, iy + 1, iz + 1}, {ix + 1, iy, iz + 1} };
        builder.addQuad(c);
    }
    else if ( axis == 1 && !positiveSide ) {
        const int c[4][3] = {
            {ix, iy, iz}, {ix + 1, iy, iz},
            {ix + 1, iy, iz + 1}, {ix, iy, iz + 1} };
        builder.addQuad(c);
    }
    else if ( axis == 1 ) {
        const int c[4][3] = {
            {ix, iy + 1, iz}, {ix, iy + 1, iz + 1},
            {ix + 1, iy + 1, iz + 1}, {ix + 1, iy + 1, iz} };
        builder.addQuad(c);
    }
    else if ( axis == 2 && !positiveSide ) {
        const int c[4][3] = {
            {ix, iy, iz}, {ix, iy + 1, iz},
            {ix + 1, iy + 1, iz}, {ix + 1, iy, iz} };
        builder.addQuad(c);
    }
    else {
        const int c[4][3] = {
            {ix, iy, iz + 1}, {ix + 1, iy, iz + 1},
            {ix + 1, iy + 1, iz + 1}, {ix, iy + 1, iz + 1} };
        builder.addQuad(c);
    }
}

/**
Adds the boundary quads of the occupied cells of a `nx * ny * nz` grid
(flattened as `(ix*ny + iy)*nz + iz`), in the Java emission order.
*/
void CellFallback::addOccupiedCellQuads(AxisAlignedCellBooleanBuilder& builder,
    const java::ArrayList<bool>& occupied, int nx, int ny, int nz)
{
    int ix;
    int iy;
    int iz;

#define OCC(x, y, z) occupied[((size_t)(x)*ny + (y))*nz + (z)]
    for ( ix = 0; ix < nx; ix++ ) {
        for ( iy = 0; iy < ny; iy++ ) {
            for ( iz = 0; iz < nz; iz++ ) {
                if ( !OCC(ix, iy, iz) ) {
                    continue;
                }
                if ( ix == 0 || !OCC(ix - 1, iy, iz) ) {
                    addAxisAlignedBoundaryQuad(builder, 0, false, ix, iy, iz);
                }
                if ( ix == nx - 1 || !OCC(ix + 1, iy, iz) ) {
                    addAxisAlignedBoundaryQuad(builder, 0, true, ix, iy, iz);
                }
                if ( iy == 0 || !OCC(ix, iy - 1, iz) ) {
                    addAxisAlignedBoundaryQuad(builder, 1, false, ix, iy, iz);
                }
                if ( iy == ny - 1 || !OCC(ix, iy + 1, iz) ) {
                    addAxisAlignedBoundaryQuad(builder, 1, true, ix, iy, iz);
                }
                if ( iz == 0 || !OCC(ix, iy, iz - 1) ) {
                    addAxisAlignedBoundaryQuad(builder, 2, false, ix, iy, iz);
                }
                if ( iz == nz - 1 || !OCC(ix, iy, iz + 1) ) {
                    addAxisAlignedBoundaryQuad(builder, 2, true, ix, iy, iz);
                }
            }
        }
    }
#undef OCC
}

PolyhedralBoundedSolid* CellFallback::buildAxisAlignedCellBooleanFallback(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    int op)
{
    long int i;
    int ix;
    int iy;
    int iz;

    if ( !isAxisAlignedSolid(inSolidA) || !isAxisAlignedSolid(inSolidB) ) {
        return 0;
    }

    java::ArrayList<double> xCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(inSolidA, 0);
    java::ArrayList<double> yCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(inSolidA, 1);
    java::ArrayList<double> zCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(inSolidA, 2);
    for ( i = 0; i < inSolidB->getVerticesList().size(); i++ ) {
        Vector3Dd p = inSolidB->getVerticesList().get(i)->position;
        FallbackGeometry::addUniqueCoordinate(xCoordinates, p.x());
        FallbackGeometry::addUniqueCoordinate(yCoordinates, p.y());
        FallbackGeometry::addUniqueCoordinate(zCoordinates, p.z());
    }

    if ( xCoordinates.size() < 2 || yCoordinates.size() < 2 ||
         zCoordinates.size() < 2 ||
         xCoordinates.size() > 16 || yCoordinates.size() > 16 ||
         zCoordinates.size() > 16 ) {
        return 0;
    }

    int nx = (int)xCoordinates.size() - 1;
    int ny = (int)yCoordinates.size() - 1;
    int nz = (int)zCoordinates.size() - 1;
    java::ArrayList<bool> occupied = java::Collections::nCopies<bool>((size_t)nx * ny * nz, false);
    for ( ix = 0; ix < nx; ix++ ) {
        for ( iy = 0; iy < ny; iy++ ) {
            for ( iz = 0; iz < nz; iz++ ) {
                Vector3Dd sample(
                    (xCoordinates[ix] + xCoordinates[ix + 1]) * 0.5,
                    (yCoordinates[iy] + yCoordinates[iy + 1]) * 0.5,
                    (zCoordinates[iz] + zCoordinates[iz + 1]) * 0.5);
                bool insideA = classifyPointForAxisAlignedFallback(
                    inSolidA, sample) == Geometry::INSIDE;
                bool insideB = classifyPointForAxisAlignedFallback(
                    inSolidB, sample) == Geometry::INSIDE;
                occupied[((size_t)ix*ny + iy)*nz + iz] =
                    axisAlignedCellSelected(insideA, insideB, op);
            }
        }
    }

    AxisAlignedCellBooleanBuilder builder(xCoordinates, yCoordinates, zCoordinates);
    addOccupiedCellQuads(builder, occupied, nx, ny, nz);
    return builder.result();
}

java::ArrayList<double> CellFallback::uniformCoordinates(double min, double max,
    int divisions)
{
    java::ArrayList<double> coordinates;
    int i;

    for ( i = 0; i <= divisions; i++ ) {
        coordinates.add(min + (max - min) * i / divisions);
    }
    return coordinates;
}

PolyhedralBoundedSolid* CellFallback::buildUniformSampledCellBooleanFallback(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    int op)
{
    const int divisions = 12;
    bool anyOccupied;
    int ix;
    int iy;
    int iz;

    if ( inSolidA == 0 || inSolidB == 0 ||
         inSolidA->getVerticesList().size() <= 0 ||
         inSolidB->getVerticesList().size() <= 0 ) {
        return 0;
    }

    double* bounds = inSolidA->getMinMax();
    if ( op == UNION ) {
        double* boundsB = inSolidB->getMinMax();

        bounds[0] = std::fmin(bounds[0], boundsB[0]);
        bounds[1] = std::fmin(bounds[1], boundsB[1]);
        bounds[2] = std::fmin(bounds[2], boundsB[2]);
        bounds[3] = std::fmax(bounds[3], boundsB[3]);
        bounds[4] = std::fmax(bounds[4], boundsB[4]);
        bounds[5] = std::fmax(bounds[5], boundsB[5]);
        delete[] boundsB;
    }

    java::ArrayList<double> xCoordinates = uniformCoordinates(bounds[0], bounds[3], divisions);
    java::ArrayList<double> yCoordinates = uniformCoordinates(bounds[1], bounds[4], divisions);
    java::ArrayList<double> zCoordinates = uniformCoordinates(bounds[2], bounds[5], divisions);
    delete[] bounds;
    java::ArrayList<bool> occupied = java::Collections::nCopies<bool>((size_t)divisions * divisions * divisions, false);
    anyOccupied = false;

    for ( ix = 0; ix < divisions; ix++ ) {
        for ( iy = 0; iy < divisions; iy++ ) {
            for ( iz = 0; iz < divisions; iz++ ) {
                Vector3Dd sample(
                    (xCoordinates[ix] + xCoordinates[ix + 1]) * 0.5,
                    (yCoordinates[iy] + yCoordinates[iy + 1]) * 0.5,
                    (zCoordinates[iz] + zCoordinates[iz + 1]) * 0.5);
                bool insideA = classifyPointForAxisAlignedFallback(
                    inSolidA, sample) == Geometry::INSIDE;
                bool insideB = classifyPointForAxisAlignedFallback(
                    inSolidB, sample) == Geometry::INSIDE;
                bool selected = axisAlignedCellSelected(insideA, insideB, op);
                occupied[((size_t)ix*divisions + iy)*divisions + iz] = selected;
                anyOccupied |= selected;
            }
        }
    }

    if ( !anyOccupied ) {
        return 0;
    }

    AxisAlignedCellBooleanBuilder builder(xCoordinates, yCoordinates, zCoordinates);
    addOccupiedCellQuads(builder, occupied, divisions, divisions, divisions);
    return builder.result();
}
