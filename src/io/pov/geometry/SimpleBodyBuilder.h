#ifndef __SIMPLE_BODY_BUILDER__
#define __SIMPLE_BODY_BUILDER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"
#include "environment/intersection/hit/SurfaceDetailFamily.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/RayCastingHit.h"
#include "environment/intersection/hit/RayCastingContainment.h"
#include "environment/intersection/ray/TransformStep.h"
#include "environment/scene/PovrayBody.h"

class ConstructiveSolidGeometry;
class Light;
class ProjectedTriangleTraversal;

/**
A POV shape under construction: its geometry, CSG or light with the material, color,
placement and steps parsed for it. The builder is consumed whole by
`buildBody`, `buildRegion` or `addToCsg`; its parts are never moved out
one by one.
*/
class SimpleBodyBuilder {
  private:
    Geometry *geometry = nullptr;
    ConstructiveSolidGeometry *csg = nullptr;
    Light *light = nullptr;
    // Projected traversal of a triangle mesh geometry, given to the body.
    ProjectedTriangleTraversal *triangleTraversal = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    java::ArrayList<TransformStep> steps{4};
    // Inversion of a family the body keeps as a flag (see PovrayBody).
    bool surfaceInverted = false;

    void ensureMatrices();

  public:
    SimpleBodyBuilder() {}
    SimpleBodyBuilder(Geometry *geometry, Material *material, ColorRgba *shapeColor);
    explicit SimpleBodyBuilder(ConstructiveSolidGeometry *csg);
    explicit SimpleBodyBuilder(Light *light);
    SimpleBodyBuilder(const SimpleBodyBuilder &other);
    ~SimpleBodyBuilder();

    Geometry* getGeometry() const { return geometry; }
    ConstructiveSolidGeometry* getCsg() const { return csg; }
    // Makes the geometry, which must be a triangle mesh, a POV projected mesh.
    ProjectedTriangleTraversal* enableProjectedTriangleTraversal();
    ProjectedTriangleTraversal* getTriangleTraversal() const { return triangleTraversal; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    Matrix4x4d* getTransformation() const { return transformation; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }
    const java::ArrayList<TransformStep>& getSteps() const { return steps; }
    bool isSurfaceInverted() const { return surfaceInverted; }
    void setSurfaceInverted(bool value) { surfaceInverted = value; }
    Material* releaseMaterial();
    ColorRgba* ensureShapeColor();
    void prependMaterialLayers(Material *newHead);

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void translateOwnerOnly(Vector3Dd *vector);
    void rotateOwnerOnly(Vector3Dd *vector);
    void scaleOwnerOnly(Vector3Dd *vector);
    void invert();

    /**
    Moves the content into a scene object body: a CSG keeps its placement
    as the body layer, any other shape as the geometry layer. The shape
    color is dropped, as POV does for object shapes. Leaves the builder empty.
    */
    PovrayBody *buildBody(Material *objectTexture, PovrayBody::MaterialReleaser releaser);

    /**
    Moves the geometry and placement into a bounded_by/clipped_by region
    body; material and color are dropped. Leaves the builder empty.
    */
    PovrayBody *buildRegion();

    /**
    Moves the content into a new operand of `container`. Leaves the builder
    empty.
    */
    void addToCsg(ConstructiveSolidGeometry *container);
};

#endif
