#ifndef __SCENE_SOLID__
#define __SCENE_SOLID__

#include "vsdk/toolkit/common/Entity.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSupport.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
class Geometry;
class GeometryBounds;
class Material;
class Ray;
class RayHit;
class RaySolidClassification;
class Sphere;

/**
The scene element contract shared by every application's bodies: a geometry
reference, an authoritative affine placement (column vectors, geometry to
parent) with its inverse, and the queries of the placed solid. The typed
queries are computed here, once, from the geometry and
`queryAffinePlacement`; implementations add their own placement storage,
appearance and first hit paths, and override a typed query only when a scene
policy changes the solid they denote (CSG, clipping regions, inversion).
Bounds, box classification and solid equivalence apply only to bodies that
opt in through `denotesPlacedGeometry`.

`SimpleBody` is the VITRAL implementation; applications may provide their own
(PovRay's `PovrayBody`) referencing the same `Geometry` types.
*/
class SceneSolid : public Entity {
protected:
    Geometry* geometry;
private:
    /// False when the geometry is shared (see `setGeometryReference`)
    bool ownsGeometry;
    bool geometryIsSphere;
    java::String name;
    long long modificationVersion;

    static GeometryContainment classifyPlacedSphereCertified(double radius,
        const Matrix4x4d& forward, const Matrix4x4d& inverse, const Vector3Dd& point,
        double distanceTolerance, int iterationLimit);
    static GeometryContainment classifyPlacedSolidCertified(const Geometry& geometry,
        const Matrix4x4d& forward, const Vector3Dd& point, double distanceTolerance);
    static GeometryContainment orientSide(GeometryContainment side, bool inverted);
    GeometryDistanceStatus queryDenotedSphere(const Sphere*& sphere, bool& inverted,
        Matrix4x4d& forward, Matrix4x4d& inverse) const;

protected:
    void markModified();
    bool hasSphereGeometry() const;
    /**
    Whether the solid of this body is exactly its geometry under its
    placement, with no scene policy that changes membership. Only such bodies
    report solid bounds, certified box classification and an equivalence
    geometry. False by default, so unknown implementations do not opt in.
    */
    virtual bool denotesPlacedGeometry() const;
    /**
    The intrinsic solid, and whether the body denotes its complement, when
    this body's solid is that solid under its placement with no scene policy
    changing membership. By default the geometry itself, only for bodies that
    `denotesPlacedGeometry`. Bodies that wrap or invert their geometry return
    the wrapped solid and the inversion. Box classification, solid
    equivalence and the boundary distances are computed from it.
    @return false when no such solid exists
    */
    virtual bool queryDenotedSolid(const Geometry*& intrinsic, bool& inverted) const;

public:
    // The single certified world-space point classification for an intrinsic
    // solid under a validated affine pair. Spheres use the ellipsoid distance
    // certificates; other solids map the tolerance ball to local inner and outer
    // balls. An inverted binding swaps Inside and Outside. Scene specializations
    // and CSG operands reuse this routine rather than reimplementing a certificate.
    static GeometryContainment classifyPlacedBoxCertified(const Geometry& geometry,
        bool inverted, const Matrix4x4d& inverse, const GeometryBounds& box);
    static GeometryContainment classifyPlacedPointCertified(const Geometry& geometry,
        bool inverted, const Matrix4x4d& forward, const Matrix4x4d& inverse,
        const Vector3Dd& point, double distanceTolerance);

    SceneSolid();
    virtual ~SceneSolid();

    const java::String& getName() const;
    long long getModificationVersion() const;
    void setName(const java::String& n);

    virtual Geometry* getGeometry() const;
    // Whether a CSG traversal may descend into this body's geometry as its
    // solid: exactly when the body denotes its placed geometry.
    bool isStructuralTraversalSafe() const;
    // Borrowed core solid only when scene-specific policies preserve it exactly.
    // Unknown subclasses must opt in explicitly; nullptr means not comparable.
    virtual const Geometry* getSolidEquivalenceGeometry() const;
    /**
    Sets the geometry, taking its ownership (it is deleted with this body or
    when replaced).
    @param g new geometry, or null
    */
    virtual void setGeometry(Geometry* g);
    /**
    Sets a geometry shared with other owners (i.e. a primitive instanced by
    several bodies): this body does not delete it.
    @param g shared geometry, or null
    */
    virtual void setGeometryReference(Geometry* g);
    /// The material reported with this body's ray crossings, or null
    virtual Material* getSurfaceMaterial() const;

    // Column-vector affine geometry-to-parent placement, including all layers.
    virtual Matrix4x4d getAffinePlacement() const = 0;
    // On failure the previous placement remains unchanged.
    virtual AffinePlacementStatus setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse) = 0;
    // Reset to identity.
    virtual void resetAffinePlacement() = 0;
    virtual AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const = 0;

    // Result is in the parent coordinate system of this body.
    virtual void getWorldBounds(GeometryBounds& result) const;
    // Conservative world bounds of the solid this body denotes. Only bodies
    // that denote their placed geometry, over Solid geometry, qualify; others
    // report Unsupported, so bounds are never mistaken for volumetric membership.
    virtual void getSolidWorldBounds(GeometryBounds& result) const;
    // Certified classification of a closed world-space axis-aligned box:
    // Inside/Outside only when the whole box lies strictly in the interior or
    // exterior of this body's solid; never Boundary.
    virtual GeometryContainment classifyWorldBoxCertified(const GeometryBounds& box) const;

    virtual bool doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit) const = 0;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) const = 0;
    virtual RayCrossingSupport emitRayCrossings(
        const Ray&, const RayCrossingQuery&, RayCrossingSink&) const;
    // Zero tolerance classifies the mapped local point. Positive world-space
    // tolerance uses the certified classification below.
    virtual GeometryContainment classifyPoint(const Vector3Dd& point, double distanceTolerance) const;
    // Numerical estimate for placed spheres; estimatedError is not certified.
    // On failure both output arguments remain unchanged.
    virtual GeometryDistanceStatus estimateBoundaryDistance(const Vector3Dd& point,
        double& distance, double& estimatedError) const;
    // Certified enclosure for a placed sphere and a nonzero local direction.
    // The lower bound may be zero; all outputs are unchanged on failure.
    virtual GeometryDistanceStatus encloseBoundaryDistance(const Vector3Dd& point,
        const Vector3Dd& localDirection, GeometryContainment& side, double& lower, double& upper) const;
    // Certified minimum-distance enclosure no wider than maxWidth. Failure
    // leaves all outputs untouched; currently supported for placed spheres.
    virtual GeometryDistanceStatus encloseMinimumBoundaryDistance(const Vector3Dd& point,
        double maxWidth, GeometryContainment& side, double& lower, double& upper) const;
    // Only decides when the enclosure proves the result; otherwise Indeterminate.
    virtual GeometryContainment classifyPointCertified(const Vector3Dd& point, double distanceTolerance) const;
    virtual RaySolidQueryStatus classifyRay(const Ray&, const RayCrossingQuery&,
        RaySolidClassification& result) const;
};

#endif
