#include "java/util/Collections.h"
#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include <cfloat>
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidOrthogonalProfileFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidAxisAlignedCellFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidOrthogonalProfileFallback ProfileFallback;
typedef _PolyhedralBoundedSolidFallbackGeometry FallbackGeometry;

//= Operand spec ====================================================

bool ProfileFallback::OrthogonalProfileOperandSpec::contains(
    double x, double y, double z) const
{
    double eps = numericContext.bigEpsilon();

    if ( type == PROFILE_X_EXTRUDED_YZ ) {
        return x >= bounds[0] - eps &&
               x <= bounds[3] + eps &&
               pointInsideYZProfile(yzProfile, y, z);
    }

    return y >= bounds[1] - eps &&
           y <= bounds[4] + eps &&
           z >= bounds[2] - eps &&
           z <= bounds[5] + eps &&
           x >= bounds[0] - eps &&
           x <= rightXAtZ(z) + eps;
}

double ProfileFallback::OrthogonalProfileOperandSpec::rightXAtZ(double z) const
{
    size_t i;
    double eps = numericContext.bigEpsilon();

    if ( rightBoundaryZ.isEmpty() ) {
        return bounds[3];
    }
    if ( z <= rightBoundaryZ[0] + eps ) {
        return rightBoundaryX[0];
    }
    for ( i = 0; i + 1 < rightBoundaryZ.size(); i++ ) {
        double z0 = rightBoundaryZ[i];
        double z1 = rightBoundaryZ[i + 1];
        double x0 = rightBoundaryX[i];
        double x1 = rightBoundaryX[i + 1];
        if ( z <= z1 + eps ) {
            if ( FallbackGeometry::sameCoordinate(z0, z1) ) {
                return x0;
            }
            double t = (z - z0) / (z1 - z0);
            return x0 + (x1 - x0) * t;
        }
    }
    return rightBoundaryX[rightBoundaryX.size() - 1];
}

//= Boolean spec ====================================================

double ProfileFallback::OrthogonalProfileBooleanFallbackSpec::xAtBoundary(
    int boundary, double z) const
{
    if ( boundary == 0 ) {
        return xMin;
    }
    if ( boundary == 1 ) {
        return yExtruded.rightXAtZ(z);
    }
    return xMax;
}

Vector3Dd ProfileFallback::OrthogonalProfileBooleanFallbackSpec::point(
    int boundary, int iy, int iz) const
{
    double z = zCoordinates[iz];
    return Vector3Dd(xAtBoundary(boundary, z), yCoordinates[iy], z);
}

//= Builder =========================================================

ProfileFallback::ProfileCellBooleanBuilder::ProfileCellBooleanBuilder()
    : solid(new PolyhedralBoundedSolid()), nextVertexId(1), nextFaceId(1)
{
}

long long ProfileFallback::ProfileCellBooleanBuilder::coordinateKey(double value)
{
    // As Java `Math.round`
    return (long long)std::floor(value * 1000000000000.0 + 0.5);
}

java::String ProfileFallback::ProfileCellBooleanBuilder::vertexKey(
    const Vector3Dd& point)
{
    return java::String::valueOf(coordinateKey(point.x())) + ":" +
        java::String::valueOf(coordinateKey(point.y())) + ":" +
        java::String::valueOf(coordinateKey(point.z()));
}

_PolyhedralBoundedSolidVertex* ProfileFallback::ProfileCellBooleanBuilder::vertexAt(
    const Vector3Dd& point)
{
    java::String key = vertexKey(point);
    _PolyhedralBoundedSolidVertex** found = vertices.get(key);
    if ( found != nullptr ) {
        return (*found);
    }

    _PolyhedralBoundedSolidVertex* vertex =
        new _PolyhedralBoundedSolidVertex(point, nextVertexId);
    solid->getVerticesList().add(vertex);
    solid->setMaxVertexId(nextVertexId);
    nextVertexId++;
    vertices.put(key, vertex);
    return vertex;
}

java::String ProfileFallback::ProfileCellBooleanBuilder::edgeKey(
    _PolyhedralBoundedSolidVertex* a, _PolyhedralBoundedSolidVertex* b)
{
    if ( a->id < b->id ) {
        return java::String::valueOf(a->id) + ":" + java::String::valueOf(b->id);
    }
    return java::String::valueOf(b->id) + ":" + java::String::valueOf(a->id);
}

void ProfileFallback::ProfileCellBooleanBuilder::attachEdge(
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

bool ProfileFallback::ProfileCellBooleanBuilder::degenerateQuad(
    const Vector3Dd corners[4])
{
    for ( int i = 0; i < 4; i++ ) {
        if ( FallbackGeometry::sameProfilePoint(corners[i], corners[(i + 1) % 4]) ) {
            return true;
        }
    }
    return false;
}

void ProfileFallback::ProfileCellBooleanBuilder::addQuad(const Vector3Dd corners[4])
{
    _PolyhedralBoundedSolidHalfEdge* halfEdges[4];
    _PolyhedralBoundedSolidVertex* faceVertices[4];
    int i;

    if ( degenerateQuad(corners) ) {
        return;
    }

    _PolyhedralBoundedSolidFace* face =
        new _PolyhedralBoundedSolidFace(solid, nextFaceId);
    solid->getPolygonsList().add(face);
    solid->setMaxFaceId(nextFaceId);
    nextFaceId++;
    _PolyhedralBoundedSolidLoop* loop = new _PolyhedralBoundedSolidLoop(face);

    for ( i = 0; i < 4; i++ ) {
        faceVertices[i] = vertexAt(corners[i]);
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

PolyhedralBoundedSolid* ProfileFallback::ProfileCellBooleanBuilder::result()
{
    return solid;
}

//= Fallback ========================================================

bool ProfileFallback::pointInsideYZProfile(const java::ArrayList<Vector3Dd>& profile,
    double y, double z)
{
    bool inside;
    size_t i;
    size_t j;

    if ( profile.size() < 3 ) {
        return false;
    }

    inside = false;
    j = profile.size() - 1;
    for ( i = 0; i < profile.size(); i++ ) {
        double yi = profile[i].y();
        double zi = profile[i].z();
        double yj = profile[j].y();
        double zj = profile[j].z();
        if ( ((zi > z) != (zj > z)) &&
             y < (yj - yi) * (z - zi) / (zj - zi) + yi ) {
            inside = !inside;
        }
        j = i;
    }
    return inside;
}

bool ProfileFallback::createXExtrudedYZSpec(PolyhedralBoundedSolid* solid,
    const java::ArrayList<double>& xCoordinates,
    const java::ArrayList<double>& yCoordinates,
    const java::ArrayList<double>& zCoordinates,
    OrthogonalProfileOperandSpec& outSpec)
{
    if ( xCoordinates.size() != 2 || yCoordinates.size() != 4 ||
         zCoordinates.size() != 3 || solid->getVerticesList().size() != 16 ) {
        return false;
    }
    double* bounds = solid->getMinMax();
    java::ArrayList<Vector3Dd> profile =
        FallbackGeometry::extractProfileAtX(solid, bounds[0]);
    if ( profile.size() < 3 ) {
        profile = FallbackGeometry::extractProfileAtX(solid, bounds[3]);
    }
    if ( profile.size() < 3 ) {
        delete[] bounds;
        return false;
    }
    outSpec.type = PROFILE_X_EXTRUDED_YZ;
    for ( int i = 0; i < 6; i++ ) {
        outSpec.bounds[i] = bounds[i];
    }
    delete[] bounds;
    outSpec.yzProfile = profile;
    outSpec.rightBoundaryZ.clear();
    outSpec.rightBoundaryX.clear();
    return true;
}

bool ProfileFallback::createYExtrudedXZSpec(PolyhedralBoundedSolid* solid,
    const java::ArrayList<double>& xCoordinates,
    const java::ArrayList<double>& yCoordinates,
    const java::ArrayList<double>& zCoordinates,
    OrthogonalProfileOperandSpec& outSpec)
{
    java::ArrayList<double> rightZ;
    java::ArrayList<double> rightX;
    size_t i;

    if ( xCoordinates.size() != 3 || yCoordinates.size() != 2 ||
         zCoordinates.size() != 3 || solid->getVerticesList().size() != 10 ) {
        return false;
    }

    for ( i = 0; i < zCoordinates.size(); i++ ) {
        double z = zCoordinates[i];
        double maxX = -DBL_MAX;
        long int j;

        for ( j = 0; j < solid->getVerticesList().size(); j++ ) {
            Vector3Dd p = solid->getVerticesList().get(j)->position;
            if ( FallbackGeometry::sameCoordinate(p.z(), z) && p.x() > maxX ) {
                maxX = p.x();
            }
        }
        if ( maxX <= -DBL_MAX / 2.0 ) {
            return false;
        }
        rightZ.add(z);
        rightX.add(maxX);
    }

    double* bounds = solid->getMinMax();
    outSpec.type = PROFILE_Y_EXTRUDED_XZ;
    for ( int k = 0; k < 6; k++ ) {
        outSpec.bounds[k] = bounds[k];
    }
    delete[] bounds;
    outSpec.yzProfile.clear();
    outSpec.rightBoundaryZ = rightZ;
    outSpec.rightBoundaryX = rightX;
    return true;
}

bool ProfileFallback::createOrthogonalProfileSpec(PolyhedralBoundedSolid* solid,
    OrthogonalProfileOperandSpec& outSpec)
{
    java::ArrayList<double> xCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(solid, 0);
    java::ArrayList<double> yCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(solid, 1);
    java::ArrayList<double> zCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(solid, 2);

    if ( createYExtrudedXZSpec(solid, xCoordinates, yCoordinates, zCoordinates,
             outSpec) ) {
        return true;
    }
    return createXExtrudedYZSpec(solid, xCoordinates, yCoordinates,
        zCoordinates, outSpec);
}

bool ProfileFallback::prepareOrthogonalProfileBooleanFallbackSpec(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    OrthogonalProfileBooleanFallbackSpec& outSpec)
{
    OrthogonalProfileOperandSpec specA;
    OrthogonalProfileOperandSpec specB;
    long int i;

    if ( !createOrthogonalProfileSpec(inSolidA, specA) ||
         !createOrthogonalProfileSpec(inSolidB, specB) ||
         specA.type == specB.type ) {
        return false;
    }
    const OrthogonalProfileOperandSpec& yExtruded =
        specA.type == PROFILE_Y_EXTRUDED_XZ ? specA : specB;
    const OrthogonalProfileOperandSpec& xExtruded =
        specA.type == PROFILE_X_EXTRUDED_YZ ? specA : specB;

    if ( !FallbackGeometry::sameCoordinate(yExtruded.bounds[0], xExtruded.bounds[0]) ||
         !FallbackGeometry::sameCoordinate(yExtruded.bounds[1], xExtruded.bounds[1]) ||
         !FallbackGeometry::sameCoordinate(yExtruded.bounds[2], xExtruded.bounds[2]) ||
         !FallbackGeometry::sameCoordinate(yExtruded.bounds[4], xExtruded.bounds[4]) ||
         !FallbackGeometry::sameCoordinate(yExtruded.bounds[5], xExtruded.bounds[5]) ||
         yExtruded.bounds[3] >= xExtruded.bounds[3] - numericContext.bigEpsilon() ) {
        return false;
    }

    java::ArrayList<double> yCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(inSolidA, 1);
    java::ArrayList<double> zCoordinates =
        FallbackGeometry::uniqueVertexCoordinates(inSolidA, 2);
    for ( i = 0; i < inSolidB->getVerticesList().size(); i++ ) {
        Vector3Dd p = inSolidB->getVerticesList().get(i)->position;
        FallbackGeometry::addUniqueCoordinate(yCoordinates, p.y());
        FallbackGeometry::addUniqueCoordinate(zCoordinates, p.z());
    }
    if ( yCoordinates.size() < 2 || zCoordinates.size() < 2 ||
         yCoordinates.size() > 8 || zCoordinates.size() > 8 ) {
        return false;
    }

    outSpec.operandA = specA;
    outSpec.operandB = specB;
    outSpec.yExtruded = yExtruded;
    outSpec.xMin = xExtruded.bounds[0];
    outSpec.xMax = xExtruded.bounds[3];
    outSpec.yCoordinates = yCoordinates;
    outSpec.zCoordinates = zCoordinates;
    return true;
}

bool ProfileFallback::profileCellSelected(
    const OrthogonalProfileBooleanFallbackSpec& spec,
    int op, int zone, int iy, int iz)
{
    double y = (spec.yCoordinates[iy] + spec.yCoordinates[iy + 1]) * 0.5;
    double z = (spec.zCoordinates[iz] + spec.zCoordinates[iz + 1]) * 0.5;
    double x0 = spec.xAtBoundary(zone, z);
    double x1 = spec.xAtBoundary(zone + 1, z);
    if ( x1 <= x0 + numericContext.bigEpsilon() ) {
        return false;
    }
    double x = (x0 + x1) * 0.5;
    bool insideA = spec.operandA.contains(x, y, z);
    bool insideB = spec.operandB.contains(x, y, z);
    return _PolyhedralBoundedSolidAxisAlignedCellFallback::axisAlignedCellSelected(
        insideA, insideB, op);
}

void ProfileFallback::addProfileBoundaryQuad(ProfileCellBooleanBuilder& builder,
    const OrthogonalProfileBooleanFallbackSpec& spec,
    int zone, int iy, int iz, int side)
{
    int left = zone;
    int right = zone + 1;

    if ( side == 0 ) {
        const Vector3Dd c[4] = {
            spec.point(left, iy, iz), spec.point(left, iy, iz + 1),
            spec.point(left, iy + 1, iz + 1), spec.point(left, iy + 1, iz) };
        builder.addQuad(c);
    }
    else if ( side == 1 ) {
        const Vector3Dd c[4] = {
            spec.point(right, iy, iz), spec.point(right, iy + 1, iz),
            spec.point(right, iy + 1, iz + 1), spec.point(right, iy, iz + 1) };
        builder.addQuad(c);
    }
    else if ( side == 2 ) {
        const Vector3Dd c[4] = {
            spec.point(left, iy, iz), spec.point(right, iy, iz),
            spec.point(right, iy, iz + 1), spec.point(left, iy, iz + 1) };
        builder.addQuad(c);
    }
    else if ( side == 3 ) {
        const Vector3Dd c[4] = {
            spec.point(left, iy + 1, iz), spec.point(left, iy + 1, iz + 1),
            spec.point(right, iy + 1, iz + 1), spec.point(right, iy + 1, iz) };
        builder.addQuad(c);
    }
    else if ( side == 4 ) {
        const Vector3Dd c[4] = {
            spec.point(left, iy, iz), spec.point(left, iy + 1, iz),
            spec.point(right, iy + 1, iz), spec.point(right, iy, iz) };
        builder.addQuad(c);
    }
    else {
        const Vector3Dd c[4] = {
            spec.point(left, iy, iz + 1), spec.point(right, iy, iz + 1),
            spec.point(right, iy + 1, iz + 1), spec.point(left, iy + 1, iz + 1) };
        builder.addQuad(c);
    }
}

PolyhedralBoundedSolid* ProfileFallback::buildOrthogonalProfileBooleanFallback(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    int op)
{
    OrthogonalProfileBooleanFallbackSpec spec;
    int zone;
    int iy;
    int iz;

    if ( !prepareOrthogonalProfileBooleanFallbackSpec(inSolidA, inSolidB, spec) ) {
        return 0;
    }

    int ny = (int)spec.yCoordinates.size() - 1;
    int nz = (int)spec.zCoordinates.size() - 1;
    java::ArrayList<bool> occupied = java::Collections::nCopies<bool>((size_t)2 * ny * nz, false);
#define OCC(z0, y0, w0) occupied[((size_t)(z0)*ny + (y0))*nz + (w0)]
    for ( zone = 0; zone < 2; zone++ ) {
        for ( iy = 0; iy < ny; iy++ ) {
            for ( iz = 0; iz < nz; iz++ ) {
                OCC(zone, iy, iz) = profileCellSelected(spec, op, zone, iy, iz);
            }
        }
    }

    ProfileCellBooleanBuilder builder;
    for ( zone = 0; zone < 2; zone++ ) {
        for ( iy = 0; iy < ny; iy++ ) {
            for ( iz = 0; iz < nz; iz++ ) {
                if ( !OCC(zone, iy, iz) ) {
                    continue;
                }
                if ( zone == 0 || !OCC(zone - 1, iy, iz) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 0);
                }
                if ( zone == 1 || !OCC(zone + 1, iy, iz) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 1);
                }
                if ( iy == 0 || !OCC(zone, iy - 1, iz) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 2);
                }
                if ( iy == ny - 1 || !OCC(zone, iy + 1, iz) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 3);
                }
                if ( iz == 0 || !OCC(zone, iy, iz - 1) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 4);
                }
                if ( iz == nz - 1 || !OCC(zone, iy, iz + 1) ) {
                    addProfileBoundaryQuad(builder, spec, zone, iy, iz, 5);
                }
            }
        }
    }
#undef OCC
    return builder.result();
}
