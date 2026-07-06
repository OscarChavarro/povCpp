#ifndef __BAKED_SCENE_KINDS__
#define __BAKED_SCENE_KINDS__

enum class BakedSceneTraceKind {
    Empty,
    DirectPrimitive,
    Csg,
    Composite,
    BoundedGeneric,
    GenericFallback,
};

enum class BakedSceneCsgAlgorithm {
    MorganRules,
    RaySegments,
};

enum class BakedSceneCsgPlanKind {
    GenericMorgan,
    GenericRaySegments,
    TopLevelPlaneUnion,
    DisjointBoundedUnion,
    SingleCorePlaneIntersection,
    Fallback,
};

enum class BakedSceneCsgOperandKind {
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

#endif
