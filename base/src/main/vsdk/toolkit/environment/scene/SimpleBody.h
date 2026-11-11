#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "vsdk/toolkit/common/Entity.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Geometry;
class Ray;
class RayHit;
class Image;
class RGBImageUncompressed;
class NormalMap;
class SimpleMaterial;
class Material;
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSupport.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
class RaySolidClassification;
class GeometryBounds;

class SimpleBody : public Entity {
protected:
    Geometry* geometry;
private:
    /// False when the geometry is shared (see `setGeometryReference`)
    bool ownsGeometry;
    bool geometryIsSphere;
    AffinePlacement affinePlacement;
    bool explicitAffinePlacement;
    Vector3Dd position;
    Vector3Dd scale;
    Matrix4x4d rotation;
    Matrix4x4d rotationInverse;
    Quaterniond rotationQuaternion;
    Quaterniond rotationInverseQuaternion;
    Vector3Dd inverseScale;
    bool hasInvertibleScale;
    bool hasIdentityRotation;
    bool hasUnitScale;
    bool hasZeroTranslation;
    bool hasTranslationOnlyTransform;
    bool hasIdentityTransform;

    SimpleMaterial* material;
    /// False when the material is shared (see `setMaterialReference`)
    bool ownsMaterial;
    Image* texture;
    NormalMap* normalMap;
    RGBImageUncompressed* normalMapRgb;

    java::String name;
    long long modificationVersion;

protected:
    void markModified();
private:
    enum class QueryPlacementStatus { Valid, Invalid, Indeterminate };
    QueryPlacementStatus queryPlacementStatus() const;
    void updateTransformFlags();
    void updateDecomposedPlacement();
    GeometryContainment classifyPointCertifiedWithLimit(const Vector3Dd& point,
        double distanceTolerance, int iterationLimit) const;
    bool doIntersectionWithAffinePlacement(const Ray& ray, RayHit* hit) const;
    void doExtraInformationWithAffinePlacement(const Ray& ray, double t, RayHit* hit) const;
    static bool isIdentityRotation(const Matrix4x4d& matrix);
    bool doIntersectionWithTranslationOnly(const Ray& inOutRay, RayHit* outHit, int requiredDetailMask) const;
    bool doIntersectionWithTranslationOnlySphereFastPath(const Ray& inOutRay, RayHit* outHit) const;

public:
    virtual RayCrossingSupport emitRayCrossings(
        const Ray&, const RayCrossingQuery&, RayCrossingSink&) const;
    // Classifies with an affine local ray and appends this instance's provenance.
    // Positive world-space tolerance uses conservative affine-sphere certificates;
    // other nonuniform placements remain explicitly indeterminate.
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

    virtual bool supportsDecomposedPlacement() const { return !explicitAffinePlacement; }
    SimpleBody();
    virtual ~SimpleBody();

    const java::String& getName() const;
    long long getModificationVersion() const;
    void setName(const java::String& n);

    virtual Geometry* getGeometry() const;

    /**
    Sets the geometry, taking its ownership (it is deleted with this body or
    when replaced).
    @param g new geometry, or null
    */
    virtual void setGeometry(Geometry* g);

    /**
    Sets a geometry shared with other owners (i.e. a primitive instanced by
    several bodies of a gizmo, as done by Java code with garbage collected
    references): this body does not delete it.
    @param g shared geometry, or null
    */
    virtual void setGeometryReference(Geometry* g);

    virtual Matrix4x4d getRotation() const;
    virtual void setRotation(const Matrix4x4d& rotation);
    virtual Matrix4x4d getRotationInverse() const;
    virtual void setRotationInverse(const Matrix4x4d& rotationInverse);

    virtual SimpleMaterial* getMaterial() const;
    virtual Material* getSurfaceMaterial() const;

    /**
    Sets the material, taking its ownership (it is deleted with this body or
    when replaced).
    @param m new material, or null
    */
    void setMaterial(SimpleMaterial* m);

    /**
    Sets a material shared with other owners: this body does not delete it.
    @param m shared material, or null
    */
    void setMaterialReference(SimpleMaterial* m);

    Image* getTexture() const;
    void setTexture(Image* in);

    NormalMap* getNormalMap() const;
    RGBImageUncompressed* getNormalMapRgb() const;
    void setNormalMap(NormalMap* in);

    virtual Vector3Dd getPosition() const;
    virtual void setPosition(const Vector3Dd& p);

    virtual Vector3Dd getScale() const;
    virtual Matrix4x4d getTransformationMatrix() const;
    // Column-vector affine geometry-to-parent placement, including all layers.
    virtual Matrix4x4d getAffinePlacement() const;
    // On failure the previous placement remains unchanged.
    virtual AffinePlacementStatus setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse);
    // Reset to identity and re-enable decomposed editing.
    virtual void resetAffinePlacement();
    virtual AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const;
    // Result is in the parent coordinate system of this body.
    virtual void getWorldBounds(GeometryBounds& result) const;
    virtual void setScale(const Vector3Dd& s);

    Ray* doIntersectionFirstHit(const Ray& inRay) const;
    virtual bool doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit) const;
    int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p) const;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) const;
};

#endif
