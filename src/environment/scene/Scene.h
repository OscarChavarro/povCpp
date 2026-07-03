#ifndef __SCENE__
#define __SCENE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "environment/geometry/element/AxisAlignedBox.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "environment/light/Light.h"
#include "environment/material/Material.h"
#include "environment/scene/SimpleBody.h"

class Composite;
class ConstructiveSolidGeometry;
class CsgOperand;

class Scene {
  public:
    struct CompiledTracingObject {
        SimpleBody *object = nullptr;
        AxisAlignedBox bounds = AxisAlignedBox::unbounded();
        bool bounded = false;
        bool castsShadow = true;
        int bakedSimpleBodyIndex = -1;
        int bakedCompositeIndex = -1;
    };

    enum class BakedSimpleBodyExecutionKind {
        Empty,
        DirectPrimitive,
        TransformedPrimitive,
        BoundedOrClippedPrimitive,
        DirectCsg,
        TransformedCsg,
        BoundedOrClippedCsg,
        GenericFallback,
    };

    struct BakedSimpleBody {
        SimpleBody *object = nullptr;
        Geometry *geometry = nullptr;
        Quadric *quadricGeometry = nullptr;
        Material *geometryMaterial = nullptr;
        Material *objectTexture = nullptr;
        ColorRgba *objectColor = nullptr;
        int bakedCsgIndex = -1;
        AxisAlignedBox worldBounds = AxisAlignedBox::unbounded();
        Matrix4x4d objectToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToObject = Matrix4x4d::identityMatrix();
        Matrix4x4d geometryToObject = Matrix4x4d::identityMatrix();
        Matrix4x4d objectToGeometry = Matrix4x4d::identityMatrix();
        Matrix4x4d geometryToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToGeometry = Matrix4x4d::identityMatrix();
        bool bounded = false;
        bool noShadowFlag = false;
        bool hasObjectTransform = false;
        bool hasGeometryTransform = false;
        bool hasBoundingShapes = false;
        bool hasClippingShapes = false;
        BakedSimpleBodyExecutionKind executionKind =
            BakedSimpleBodyExecutionKind::Empty;
        java::ArrayList<CompiledTracingObject> boundingObjects;
        java::ArrayList<CompiledTracingObject> clippingObjects;
        // World-space coefficient-rewritten copies (Plan 5 Phase 4) - same
        // by-value-not-self-pointer design and the same one-time fix-up-pass
        // requirement as BakedCsgOperand::bakedQuadric/bakedPlane; see the
        // comment there.
        Quadric bakedQuadric;
        bool hasBakedQuadric = false;
        InfinitePlane bakedPlane;
        bool hasBakedPlane = false;
    };

    enum class BakedCsgAlgorithm {
        MorganRules,
        RaySegments,
    };

    enum class BakedCsgSpecialization {
        None,
        TopLevelPlaneUnion,
        DisjointBoundedUnion,
        SingleCorePlaneIntersection,
    };

    enum class BakedCsgExecutionPlanKind {
        GenericMorgan,
        GenericRaySegments,
        TopLevelPlaneUnion,
        DisjointBoundedUnion,
        SingleCorePlaneIntersection,
        Fallback,
    };

    enum class BakedCsgOperandExecutionKind {
        Empty,
        DirectAnnotatedPrimitive,
        DirectPrimitive,
        DirectPlane,
        TransformedPlane,
        NestedCsg,
        TransformedNestedCsg,
        TransformedQuadric,
        TransformedSphere,
        TransformedPrimitive,
        GenericFallback,
    };

    struct BakedCsgOperand {
        CsgOperand *operand = nullptr;
        Geometry *geometry = nullptr;
        Quadric *quadricGeometry = nullptr;
        Material *material = nullptr;
        int bakedCsgIndex = -1;
        AxisAlignedBox bakedBounds = AxisAlignedBox::unbounded();
        Matrix4x4d objectToLocal = Matrix4x4d::identityMatrix();
        Matrix4x4d localToObject = Matrix4x4d::identityMatrix();
        bool hasTransform = false;
        bool bounded = false;
        bool cullSafe = false;
        bool isInfinitePlane = false;
        Vector3Dd planeNormal = Vector3Dd(0.0, 1.0, 0.0);
        double planeDistance = 0.0;
        mutable double planeVpNormDotOrigin = 0.0;
        mutable bool planeVpCached = false;
        // World-space coefficient-rewritten copies (Plan 5 Phase 3): when a
        // Quadric/InfinitePlane operand had a non-empty recorded TransformStep
        // list, BakedGeometryBaker replays it here at bake time and
        // `geometry`/`quadricGeometry` are repointed at these owned copies
        // (fixed up once, after the whole compiled scene has reached its
        // final, never-again-relocated storage - see
        // Scene::buildCompiledTracingScene's post-pass). hasTransform is
        // cleared for these operands so the per-ray transform path is never
        // reached. Plain by-value members (not accessed through a
        // self-pointer during baking) to avoid dangling pointers across the
        // several array reallocations the bake pipeline performs before
        // settling into final storage.
        Quadric bakedQuadric;
        bool hasBakedQuadric = false;
        InfinitePlane bakedPlane;
        bool hasBakedPlane = false;
        BakedCsgOperandExecutionKind executionKind =
            BakedCsgOperandExecutionKind::Empty;
        bool compiledTransformedNestedCorePlane = false;
        int compiledNestedCoreOperandIndex = -1;
        bool compiledNestedCoreDirectQuadric = false;
        bool compiledNestedCoreTransformedQuadric = false;
        java::ArrayList<int> compiledNestedPlaneOperandIndices;
        java::ArrayList<int> compiledNestedContainmentOperandIndices;
    };

    struct BakedConstructiveSolidGeometry {
        ConstructiveSolidGeometry *geometry = nullptr;
        BakedCsgAlgorithm algorithm = BakedCsgAlgorithm::MorganRules;
        BakedCsgSpecialization specialization = BakedCsgSpecialization::None;
        BakedCsgExecutionPlanKind executionPlanKind =
            BakedCsgExecutionPlanKind::Fallback;
        BooleanSetOperations geometryType = BooleanSetOperations::UNION;
        bool topLevel = false;
        bool specializationValid = false;
        int specializationCoreOperandIndex = -1;
        java::ArrayList<int> executionPlanPlaneOperandIndices;
        java::ArrayList<int> executionPlanNestedOperandIndices;
        java::ArrayList<int> executionPlanTransformedPrimitiveOperandIndices;
        java::ArrayList<int> executionPlanDirectPrimitiveOperandIndices;
        java::ArrayList<BakedCsgOperand> operands;
    };

    struct BakedComposite {
        Composite *object = nullptr;
        AxisAlignedBox worldBounds = AxisAlignedBox::unbounded();
        Matrix4x4d objectToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToObject = Matrix4x4d::identityMatrix();
        bool bounded = false;
        bool noShadowFlag = false;
        bool hasObjectTransform = false;
        bool hasBoundingShapes = false;
        bool hasClippingShapes = false;
        java::ArrayList<CompiledTracingObject> boundingObjects;
        java::ArrayList<CompiledTracingObject> clippingObjects;
        java::ArrayList<CompiledTracingObject> childObjects;
        java::ArrayList<CompiledTracingObject> boundedChildObjects;
        java::ArrayList<CompiledTracingObject> unboundedChildObjects;
    };

    struct CompiledTracingScene {
        java::ArrayList<CompiledTracingObject> objects;
        java::ArrayList<BakedSimpleBody> bakedSimpleBodies;
        java::ArrayList<BakedConstructiveSolidGeometry> bakedCsgs;
        java::ArrayList<BakedComposite> bakedComposites;
        java::ArrayList<CompiledTracingObject> boundedObjects;
        java::ArrayList<CompiledTracingObject> unboundedObjects;
        java::ArrayList<CompiledTracingObject> shadowCastingObjects;
        java::ArrayList<CompiledTracingObject> boundedShadowCastingObjects;
        java::ArrayList<CompiledTracingObject> unboundedShadowCastingObjects;
    };

    struct TracingObjectEntry {
        SimpleBody *object = nullptr;
        AxisAlignedBox bounds = AxisAlignedBox::unbounded();
    };

  public:
    static constexpr double DEFAULT_ANTIALIAS_THRESHOLD = 0.3;

    Scene();
    ~Scene();

    const CameraSnapshot& getViewPoint() const { return viewPoint; }
    void setViewPoint(const CameraSnapshot &camera) { viewPoint = camera; }
    int& getScreenHeight() { return screenHeight; }
    int getScreenHeight() const { return screenHeight; }
    void setScreenHeight(int h) { screenHeight = h; }
    int& getScreenWidth() { return screenWidth; }
    int getScreenWidth() const { return screenWidth; }
    void setScreenWidth(int w) { screenWidth = w; }
    const java::ArrayList<Light*>& getLightSources() const { return lightSources; }
    java::ArrayList<Light*>& getLightSources() { return lightSources; }
    double getAtmosphereIor() const { return atmosphereIor; }
    void setAtmosphereIor(double ior) { atmosphereIor = ior; }
    double getAntialiasThreshold() const { return antialiasThreshold; }
    void setAntialiasThreshold(double threshold) { antialiasThreshold = threshold; }
    double getFogDistance() const { return fogDistance; }
    void setFogDistance(double distance) { fogDistance = distance; }
    void setFog(const ColorRgba& color, double distance)
    {
        fogColor = color;
        fogDistance = distance;
    }
    const ColorRgba& getFogColor() const { return fogColor; }
    const java::ArrayList<SimpleBody*>& getObjects() const { return Objects; }
    void setObjects(const java::ArrayList<SimpleBody*> &objects)
    {
        Objects = objects;
        compiledTracingSceneFinalized = false;
        rebuildTracingStructures();
    }
    const java::ArrayList<TracingObjectEntry>& getBoundedTracingObjects() const
    {
        return boundedTracingObjects;
    }
    const java::ArrayList<SimpleBody*>& getUnboundedTracingObjects() const
    {
        return unboundedTracingObjects;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledTracingObjects() const
    {
        return compiledTracingScene.objects;
    }
    const java::ArrayList<BakedSimpleBody>& getBakedSimpleBodies() const
    {
        return compiledTracingScene.bakedSimpleBodies;
    }
    const java::ArrayList<BakedConstructiveSolidGeometry>& getBakedCsgs() const
    {
        return compiledTracingScene.bakedCsgs;
    }
    const java::ArrayList<BakedComposite>& getBakedComposites() const
    {
        return compiledTracingScene.bakedComposites;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledBoundedTracingObjects() const
    {
        return compiledTracingScene.boundedObjects;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledUnboundedTracingObjects() const
    {
        return compiledTracingScene.unboundedObjects;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledShadowCastingTracingObjects() const
    {
        return compiledTracingScene.shadowCastingObjects;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledBoundedShadowCastingTracingObjects() const
    {
        return compiledTracingScene.boundedShadowCastingObjects;
    }
    const java::ArrayList<CompiledTracingObject>& getCompiledUnboundedShadowCastingTracingObjects() const
    {
        return compiledTracingScene.unboundedShadowCastingObjects;
    }
    void rebuildTracingStructures();
    void buildTracingCache();
    void buildCompiledTracingScene();
    void finalizeCompiledTracingScene();
    bool isCompiledTracingSceneFinalized() const { return compiledTracingSceneFinalized; }
    void resetForSceneParse(double antialiasThreshold = DEFAULT_ANTIALIAS_THRESHOLD);

    // The scene's default texture is aliased (not cloned) into every untextured
    // object's SimpleBody::objectTexture for the entire render (see
    // ~SimpleBody()'s PovRayMaterialConstancy guard) - it can only be freed
    // once nothing will read it again, i.e. once Scene itself is destroyed.
    // SceneParser::parse() calls this once, after parsing finishes, with
    // whatever the final default texture turned out to be (it may have been
    // replaced mid-parse by a `default { texture {...} }` block).
    void captureDefaultTexture(Material *texture) { defaultTexture = texture; }

  private:
    static ColorRgba blackFogColor();

    CameraSnapshot viewPoint;
    int screenHeight;
    int screenWidth;
    java::ArrayList<Light*> lightSources;
    java::ArrayList<SimpleBody*> Objects{4};
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;
    Material *defaultTexture = nullptr;
    java::ArrayList<TracingObjectEntry> boundedTracingObjects;
    java::ArrayList<SimpleBody*> unboundedTracingObjects;
    CompiledTracingScene compiledTracingScene;
    bool compiledTracingSceneFinalized = false;
};

#endif
