#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include <cmath>
#include "java/util/concurrent/locks/ReentrantLock.h"

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/render/SpherePolyhedralCache.h"

namespace {
    /// Maximum number of entries kept; when reached, the cache is emptied
    const std::size_t MAX_CACHED_ENTRIES = 256;
    /// Relative distance to a pole under which a vertex is taken as a pole
    const double POLE_TOLERANCE = 1.0e-9;

    java::ReentrantLock& cacheLock()
    {
        static java::ReentrantLock lock;
        return lock;
    }

    java::TreeMap<java::String, SpherePolyhedralCache::Entry*>& entries()
    {
        static java::TreeMap<java::String, SpherePolyhedralCache::Entry*> map;
        return map;
    }

    void releaseAll(java::TreeMap<java::String, SpherePolyhedralCache::Entry*>& map)
    {
        for (long i = 0; i < map.size(); i++) {
            delete map.getValue(i);
        }
        map.clear();
    }

    /**
    Makes the angles of the corners of a triangle continuous across the seam
    (a triangle whose angles span more than half a turn crosses it, so the
    small ones are taken one turn further), and gives the poles the mean
    angle of the other corners.
    */
    void makeContinuous(double theta[3], const bool pole[3])
    {
        double min = 1.0e300;
        double max = -1.0e300;

        for (int k = 0; k < 3; k++) {
            if (!pole[k]) {
                min = std::fmin(min, theta[k]);
                max = std::fmax(max, theta[k]);
            }
        }
        if (max - min > M_PI) {
            for (int k = 0; k < 3; k++) {
                if (!pole[k] && theta[k] < M_PI) {
                    theta[k] += 2 * M_PI;
                }
            }
        }

        double sum = 0;
        int count = 0;

        for (int k = 0; k < 3; k++) {
            if (!pole[k]) {
                sum += theta[k];
                count++;
            }
        }
        for (int k = 0; k < 3; k++) {
            if (pole[k]) {
                theta[k] = count > 0 ? sum / count : 0;
            }
        }
    }

    /**
    @return the vertices of the faces of the solid as triangles (a fan per
    face, from its outer loop), three consecutive per triangle, each one
    counterclockwise seen from outside the sphere
    */
    java::ArrayList<Vector3Dd> collectTriangles(PolyhedralBoundedSolid* solid)
    {
        java::ArrayList<Vector3Dd> triangles;
        java::ArrayList<_PolyhedralBoundedSolidFace*>& faces = solid->getPolygonsList();

        for (long int f = 0; f < faces.size(); f++) {
            _PolyhedralBoundedSolidFace* face = faces.get(f);

            if (face == nullptr || face->boundariesList.size() < 1) {
                continue;
            }
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(0);
            _PolyhedralBoundedSolidHalfEdge* start =
                loop != nullptr ? loop->boundaryStartHalfEdge : nullptr;
            java::ArrayList<Vector3Dd> polygon;

            if (start == nullptr) {
                continue;
            }
            _PolyhedralBoundedSolidHalfEdge* he = start;
            do {
                polygon.add(he->startingVertex->position);
                he = he->next();
            } while (he != start && he != nullptr);

            for (std::size_t i = 1; i + 1 < polygon.size(); i++) {
                const Vector3Dd& a = polygon[0];
                const Vector3Dd& b = polygon[i];
                const Vector3Dd& c = polygon[i + 1];
                Vector3Dd normal = b.subtract(a).crossProduct(c.subtract(a));
                Vector3Dd center = a.add(b).add(c);

                triangles.add(a);
                if (normal.dotProduct(center) >= 0) {
                    triangles.add(b);
                    triangles.add(c);
                }
                else {
                    triangles.add(c);
                    triangles.add(b);
                }
            }
        }
        return triangles;
    }

    void put(java::ArrayList<float>& array, std::size_t index, const Vector3Dd& v)
    {
        array[3 * index] = (float)v.x();
        array[3 * index + 1] = (float)v.y();
        array[3 * index + 2] = (float)v.z();
    }
}

SpherePolyhedralCache::Entry::Entry(double radius, int meridians, int parallels,
                                    PolyhedralBoundedSolid* solid)
    : radius(radius), meridians(meridians), parallels(parallels), solid(solid)
{
}

SpherePolyhedralCache::Entry::~Entry()
{
    delete solid;
}

double SpherePolyhedralCache::Entry::getRadius() const { return radius; }
int SpherePolyhedralCache::Entry::getMeridians() const { return meridians; }
int SpherePolyhedralCache::Entry::getParallels() const { return parallels; }
PolyhedralBoundedSolid* SpherePolyhedralCache::Entry::getSolid() const { return solid; }
int SpherePolyhedralCache::Entry::getVertexCount() const { return (int)(positions.size() / 3); }
const java::ArrayList<float>& SpherePolyhedralCache::Entry::getPositions() const { return positions; }
const java::ArrayList<float>& SpherePolyhedralCache::Entry::getNormals() const { return normals; }
const java::ArrayList<float>& SpherePolyhedralCache::Entry::getUvs() const { return uvs; }
const java::ArrayList<float>& SpherePolyhedralCache::Entry::getTangents() const { return tangents; }
const java::ArrayList<float>& SpherePolyhedralCache::Entry::getBiNormals() const { return biNormals; }

const SpherePolyhedralCache::Entry* SpherePolyhedralCache::obtain(
    const Sphere* sphere, int meridians, int parallels)
{
    java::ReentrantLockGuard guard(cacheLock());
    java::TreeMap<java::String, Entry*>& map = entries();
    java::String key = java::String::format("%f", sphere->getRadius()) + "/" +
        java::String::valueOf(meridians) + "/" + java::String::valueOf(parallels);
    Entry** it = map.get(key);

    if (it != nullptr) {
        return (*it);
    }
    Entry* entry = build(sphere, meridians, parallels);
    if (map.size() >= MAX_CACHED_ENTRIES) {
        releaseAll(map);
    }
    map.put(key, entry);
    return entry;
}

void SpherePolyhedralCache::clear()
{
    java::ReentrantLockGuard guard(cacheLock());
    releaseAll(entries());
}

SpherePolyhedralCache::Entry* SpherePolyhedralCache::build(
    const Sphere* sphere, int meridians, int parallels)
{
    // The parametric evaluators of Sphere are not const qualified
    Sphere parametric(sphere->getRadius());
    PolyhedralBoundedSolid* solid =
        parametric.exportToPolyhedralBoundedSolid(meridians, parallels);
    java::ArrayList<Vector3Dd> triangles = collectTriangles(solid);
    std::size_t vertexCount = triangles.size();
    double radius = sphere->getRadius();
    Entry* entry = new Entry(radius, meridians, parallels, solid);

    entry->positions = java::Collections::nCopies<float>(vertexCount * 3, 0.0f);
    entry->normals = java::Collections::nCopies<float>(vertexCount * 3, 0.0f);
    entry->uvs = java::Collections::nCopies<float>(vertexCount * 2, 0.0f);
    entry->tangents = java::Collections::nCopies<float>(vertexCount * 3, 0.0f);
    entry->biNormals = java::Collections::nCopies<float>(vertexCount * 3, 0.0f);

    for (std::size_t t = 0; t + 2 < vertexCount; t += 3) {
        double theta[3];
        double phi[3];
        bool pole[3];

        for (int k = 0; k < 3; k++) {
            const Vector3Dd& p = triangles[t + k];
            double z = radius > 0 ? p.z() / radius : 0;

            phi[k] = std::asin(std::fmax(-1.0, std::fmin(1.0, z)));
            pole[k] = std::fabs(std::fabs(z) - 1.0) < POLE_TOLERANCE;
            theta[k] = std::atan2(-p.y(), p.x());
            if (theta[k] < 0) {
                theta[k] += 2 * M_PI;
            }
        }
        makeContinuous(theta, pole);

        for (int k = 0; k < 3; k++) {
            std::size_t i = t + k;

            put(entry->positions, i, triangles[i]);
            put(entry->normals, i, parametric.sphereNormal(theta[k], phi[k]));
            put(entry->tangents, i, parametric.sphereTangent(theta[k], phi[k]));
            put(entry->biNormals, i, parametric.sphereBinormal(theta[k], phi[k]));
            entry->uvs[2 * i] = (float)(1.0 - theta[k] / (2 * M_PI));
            entry->uvs[2 * i + 1] = (float)((phi[k] + M_PI / 2) / M_PI);
        }
    }
    return entry;
}
