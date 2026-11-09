#ifndef __SPHERE_POLYHEDRAL_CACHE__
#define __SPHERE_POLYHEDRAL_CACHE__

#include "java/util/ArrayList.h"

class Sphere;
class PolyhedralBoundedSolid;

/**
Tessellation cache shared by the `*SphereRenderer`s of every rendering
technology (OpenGL1, OpenGL4, software...), independent of all of them.

A `Sphere` is kept by its renderers as a parametric surface; to draw it, it is
converted, in code, into a `PolyhedralBoundedSolid` with the parametric
construction of `Sphere::exportToPolyhedralBoundedSolid(meridians, parallels)`,
and that solid is the cached representation: besides being drawn by the
rasterizers, it can be used by other drawing algorithms that are not
OpenGL based (hidden line removal, software renderers...), which a plain mesh
does not allow.

For the rasterizers, each entry also derives from the (triangular) faces of
the solid the per vertex attributes a smooth, textured and bump mapped sphere
needs, which the faceted solid does not carry: for each vertex, its spherical
coordinates (theta, phi) are recovered from its position, and the normal,
tangent and binormal are evaluated on the parametric surface
(`Sphere::sphereNormal`, `sphereTangent`, `sphereBinormal`), with texture
coordinates (1 - theta / (2 PI), (phi + PI / 2) / PI). Along the seam of the
parametrization (theta = 0) the texture coordinates of a triangle are made
continuous, and at the poles (where theta is undefined) the vertex takes the
mean theta of the other corners of its triangle.

Entries are kept by radius and resolution, so many spheres of a scene share
them. Entries are owned by the cache: a pointer obtained from it is valid
until the next `clear()` or until the cache empties itself when full.
*/
class SpherePolyhedralCache {
public:
    /**
    A sphere converted into a polyhedral bounded solid, with the attributes
    of its triangles for the rasterizers: three consecutive vertices make a
    triangle, counterclockwise seen from outside.
    */
    class Entry {
    public:
        Entry(double radius, int meridians, int parallels,
              PolyhedralBoundedSolid* solid);
        ~Entry();

        double getRadius() const;
        int getMeridians() const;
        int getParallels() const;
        /// The sphere as a polyhedral bounded solid, shared by every user of
        /// the entry: it must not be modified
        PolyhedralBoundedSolid* getSolid() const;
        int getVertexCount() const;
        /// x, y, z of each vertex of the triangles
        const java::ArrayList<float>& getPositions() const;
        /// x, y, z of the normal of the sphere at each vertex
        const java::ArrayList<float>& getNormals() const;
        /// u, v texture coordinates of each vertex
        const java::ArrayList<float>& getUvs() const;
        /// x, y, z of the tangent of the sphere at each vertex
        const java::ArrayList<float>& getTangents() const;
        /// x, y, z of the binormal of the sphere at each vertex
        const java::ArrayList<float>& getBiNormals() const;

    private:
        friend class SpherePolyhedralCache;

        double radius;
        int meridians;
        int parallels;
        PolyhedralBoundedSolid* solid;
        java::ArrayList<float> positions;
        java::ArrayList<float> normals;
        java::ArrayList<float> uvs;
        java::ArrayList<float> tangents;
        java::ArrayList<float> biNormals;

        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
    };

    /**
    @param sphere sphere to tessellate
    @param meridians number of meridians
    @param parallels number of bands between the poles
    @return the cached tessellation of a sphere of that radius, created the
    first time it is asked for
    */
    static const Entry* obtain(const Sphere* sphere, int meridians, int parallels);

    /**
    Forgets (and releases) every entry.
    */
    static void clear();

private:
    SpherePolyhedralCache() = delete;

    static Entry* build(const Sphere* sphere, int meridians, int parallels);
};

#endif
