# Project Closing Steps

This document is a discussion of alternatives and concepts for the last stretch
of the project: migrating `geometry` from `povCpp` into `vitral`. It complements
`designObjectives.md` (the five stated objectives) and
`vitralNormalizationAnalysis.md` / `vitralPovrayParallelLearningExperience.md`
(the unification analysis and the retrospective on doing it).

## 1. Where we actually are

No a-priori design ever produced the current five-layer architecture
(`java`/`base` + `environment` + `render` + `io` + `app`). It was derived after
the fact, by repeatedly decoupling and optimizing, and only in hindsight does it
read as coherent (see `architectureDiagram.png`). Most of the model is settled
and is not expected to change further: `geometry/surface`, `geometry/volume`,
`material`, `render/shaders`, and `app`.

What is left, and what this document is about, is `geometry` proper — moving
its intersection operations
(`doIntersectionForAllRayCrossings`, `doIntersectionForAllRayCrossingsAnnotated`,
`doContainmentTest`, `doExtraInformation`, `getMinMax`, all declared on
`Geometry`, `src/environment/geometry/Geometry.h`) into `vitral`. Two other
areas showed up unplanned, as by-products of decoupling/performance work rather
than conceptual design, and will likely absorb most of the churn that follows:

- **`geometry/element`** (`src/environment/geometry/element/`) — the
  supporting types around a ray-geometry hit (`RayCastingHitElement`,
  `AxisAlignedBoundingBox`, `DetailMask`, `GeometryConfig`,
  `GeometryIntersectionEmissionContext`, `IntersectionAttributes`,
  `IntersectionCandidate`, `PovRayHit`, `RayWithTracingState`,
  `TransformStep`, `PriorityQueuePool`).
  This package grew ad hoc, driven by whatever the last performance or
  decoupling pass needed, without a governing concept.
- **`render/bakedScene`** and **`render/raySharedCache`** — the baked-CSG and
  per-ray-cache machinery built during the performance plans
  (`performanceNotes.md`). Also ad hoc, also likely to be touched by whatever
  comes next.

Before migrating anything, we need more conceptual clarity in these two areas
than we currently have. The rest of this document works through that.

## 2. The core problem: `geometry/element` still mixes two concerns

`RayCastingHitElement` (`src/environment/geometry/element/RayCastingHitElement.h`)
is a one-method interface:

```cpp
class RayCastingHitElement {
  public:
    virtual ~RayCastingHitElement() {};
    virtual void doExtraInformation(
        const RayWithTracingState &ray, double t, PovRayHit *hit) = 0;
};
```

`Geometry` inherits from it. Its single method answers one question: *given
that a hit already happened at parameter `t`, what is the detailed information
at that point* (surface normal, UV, material references, etc.)? That is a
**post-hit** query.

Separately, and not represented by any interface at all today, there is the
**pre-hit** question: *what is the fastest possible bound I can test a ray
against before doing exact geometry math?* Today that question is answered ad
hoc by `AxisAlignedBoundingBox` (`src/environment/geometry/element/
AxisAlignedBoundingBox.h`), a plain value class (no inheritance, `min`/`max`
plus `enclosing`/`intersection`/`expandedBy`/`centroid`/`fromTransformedCorners`)
returned by `Geometry::getMinMax()`. It lives in `element/` for no reason other
than that's where it was convenient to put it when it was written, and every
BVH-adjacent structure since then (`AabbCullingSupport`, `OperandCullBins` in
`render/bakedScene/`) has been built directly against it, with no seam for a
different bounding strategy.

**Proposal.** Split `RayCastingHitElement` into two named concepts:

- **`PreRayHitElement`** — the interface for anything that can *cheaply reject
  or bound* a ray before exact intersection math runs. `BoundingVolumeHierarchy`
  (see §3) implements this.
- **`PostRayHitElement`** — renamed from `RayCastingHitElement`, keeps exactly
  `doExtraInformation`. `Geometry` implements this, unchanged in behavior.

This buys two things: it says out loud that pre-hit acceleration and post-hit
detail queries are different lifecycle stages of the same ray/geometry
interaction, and it gives `BoundingVolumeHierarchy` a legitimate interface to
implement instead of being a bag of static helper functions
(`AabbCullingSupport::rayIntersectsAabbForward`) called from wherever.

## 3. `boundingVolumeHierarchy` as its own package, decoupled from `geometry`

Neither `povCpp` nor the full `vitral` (`/home/jedilink/VITRAL/vitral/cpp/base/`)
has a real BVH today. `vitral` has `VoxelVolume` and the `geometricProcessing/`
voxelizers — a regular grid, not a hierarchy — and no `AABB`/`BSP`/`KDTree`
class at all. So this is genuinely new code, not a port of something that
already exists on the vitral side. That is an opportunity: we can place it
correctly the first time instead of migrating a mistake.

**Dependency direction is the whole point.** Today `AxisAlignedBoundingBox`
sits in `geometry/element`, and every consumer of it — `Geometry::getMinMax()`,
`render/bakedScene`'s `AabbCullingSupport`/`OperandCullBins` — depends
downward on `geometry`. That is backwards for what a bounding volume actually
is: a pre-hit accelerator that should know nothing about the shapes it bounds.

Target layering:

```
element  (defines PreRayHitElement / PostRayHitElement — no dependency on geometry or BVH)
   ^                              ^
   |                              |
boundingVolumeHierarchy    geometry
   (implements PreRayHitElement,     (implements PostRayHitElement,
    depends only on element)          MAY depend on boundingVolumeHierarchy
                                       for its own bounding needs, never the
                                       other way around)
```

Concretely:

- Move `AxisAlignedBoundingBox` out of `geometry/element` into a new
  `boundingVolumeHierarchy` package/directory.
- Introduce a `BoundingVolumeHierarchy` interface (implementing
  `PreRayHitElement`) that `AxisAlignedBoundingBox` becomes one implementation
  of — explicitly the **naive** one. It is a single box, not a hierarchy, but
  it is the placeholder for the interface until real hierarchies exist.
- `boundingVolumeHierarchy` depends only on `element` (for the
  `PreRayHitElement` interface and shared primitives like `Vector3Dd`), never
  on `geometry`.
- `geometry` is free to depend on `boundingVolumeHierarchy` (e.g. `Geometry`
  can hold/query a bounding volume for its own acceleration), but the
  dependency never goes the other direction.
- Future implementations — BSP trees, k-d trees, voxel grids reusing vitral's
  existing `VoxelVolume`/`geometricProcessing` voxelizers — become siblings of
  `AxisAlignedBoundingBox` under the same interface, swappable per scene or
  per object without `geometry` code changing.

This also gives `render/bakedScene`'s `AabbCullingSupport` and
`OperandCullBins` a real home to migrate into instead of being bespoke,
CSG-bake-specific AABB math duplicated next to (but outside) `element/`.

## 4. Cleaning the POV-Ray-specific residue out of `element/`

The migration target is `vitral`, which must stay generic. Auditing
`geometry/element/` for POV-Ray leakage:

- `PovRayHit.h` is, by name and by its own header comment, explicitly
  POV-Ray-specific — modeled field-for-field on vitral's `RayHit`
  (`p/n/t/u/v/hitDistance`) plus POV-only extensions
  (`noShadowFlag`, `materialUsesObjectLocalPoint`, `hitGeometry`/`hitBody`,
  `detailOwners[8]`). This is a deliberate, acknowledged extension point, not
  an accident — see `vitralNormalizationAnalysis.md`, sec. 10.
- `IntersectionAttributes.h` and `RayWithTracingState.h` type their `Material*`
  fields against the **generic** `vsdk/toolkit/environment/material/Material.h`
  (present in vitral, byte-identical), not `PovRayMaterial`. Confirmed by
  grep: `PovRayMaterial` appears nowhere under `geometry/element/`.
- `CsgOperand.h` (`src/environment/geometry/volume/
  constructiveSolidGeometry/CsgOperand.h`) and `SimpleBody`
  (`src/environment/scene/SimpleBody.cpp`) deep-copy a body's material through
  the generic `Material::copy()` virtual, so neither one names
  `PovRayMaterial` either.

So the element layer, and its one-layer-away neighbor `CsgOperand`, are
already clean — the real coupling point to watch during migration is
`PovRayHit`, which should stay in `povCpp` (or a `povCpp`-side extension of
vitral's `RayHit`) and never move into `vitral` itself. `vitral`'s own `RayHit`
(`environment/geometry/element/RayHit.h/.cpp`) already conflates pre-hit
"what to compute" (`requiredDetailMask()`/`needsPoint()`/`needsNormal()`/…)
and post-hit "what was computed" (`material`, `texture`, `hitDistance()`) in
one mutable object — the same Pre/Post conflation described in §2, just
already present on the vitral side under a different name. Reconciling
`PreRayHitElement`/`PostRayHitElement` with vitral's existing `RayHit` is
in-scope work for whoever picks up the migration, not a pre-solved problem.

## 5. `render/bakedScene` and `render/raySharedCache`: name the concept, then decide what migrates

Both packages are performance-plan by-products (`performanceNotes.md`,
`performance_plan*` history) and, like `geometry/element`, were built to solve
an immediate profiling problem rather than to express a concept:

- `render/bakedScene/` (24 files) compiles the parsed scene graph — CSG
  programs, transform chains, operand culling bins — into flat,
  cache-friendly structures (`BakedScene`, `BakedSceneBuilder`, `CsgProgram`,
  `TraceableObject`, `CsgOperandRecord`, …) so that per-ray traversal avoids
  virtual dispatch and repeated matrix work.
- `render/raySharedCache/` is a single header, `RaySharedCache`, a per-ray
  lazily-computed cache of quadric/plane "viewpoint constants" keyed by slot
  index.

Neither package currently has a stated relationship to `geometry` beyond
"consumes it to build a faster structure." Once BVH exists as a first-class
`boundingVolumeHierarchy` concept, `bakedScene`'s culling structures
(`AabbCullingSupport`, `OperandCullBins`) stop being bespoke CSG-bake helpers
and become one more `BoundingVolumeHierarchy` implementation, or a consumer of
one. That reclassification is the main way this area is expected to change
going forward — not a full move into `vitral` (baking is a performance
technique specific to `povCpp`'s CSG representation, not a generic vitral
concept), but a redefinition of what already-written code *is*, in terms of
the new `element`/`boundingVolumeHierarchy`/`geometry` vocabulary.

## 6. Generic ray-caster/ray-tracer construction, agnostic to `Material`

A stated goal beyond the geometry migration itself: the ray-casting and
ray-tracing scaffolding (the `doIntersectionForAllRayCrossings` family, the
depth-queue-based traversal, `PreRayHitElement`/`PostRayHitElement`) should be
buildable generically enough to support future path tracers, without baking in
any assumption about what a `Material` is. This is already partially true —
`Geometry` (pure intersection math) is already separated from `SimpleBody`
(material + transform), per `designObjectives.md`'s "key separation already
achieved" note — but the split has not been tested against a second kind of
tracer (a path tracer) or a second kind of material model.

Concretely, this means:

- `PostRayHitElement::doExtraInformation` and the `IntersectionCandidate`/
  `IntersectionAttributes` records it fills in should stay parametrized over
  the generic vitral `Material` interface, never `PovRayMaterial` — already
  true today (§4), and worth protecting as an invariant, not just an
  observation, as new code is added.
- The depth-queue traversal loop itself (`java::PriorityQueue
  <IntersectionCandidate>`-based, in `Geometry::
  doIntersectionForAllRayCrossings`/`...Annotated`) has no material awareness
  built in already; the risk is in whatever calls it (`SimpleBody`, `CsgOperand`,
  the `bakedScene` kernels), not in `Geometry` itself.
- A path tracer would want recursive, weighted, possibly stochastic ray
  generation from a hit point — a different consumer of the same
  `PostRayHitElement`/`PreRayHitElement` pre/post-hit contract, not a
  different `Geometry`. Keeping `Material` out of that contract is what makes
  this possible later without another `geometry` rewrite.
- `PovRayMaterial` must keep living in `environment/material/povray/` and
  `io/pov/material/`, entirely outside `vitral`, exactly as it does today —
  this is the one boundary that must not blur as `geometry`/`element` move.

## 7. Suggested order of operations

This is a discussion document, not a plan (see `performance_plan_docs_deleted.md`
precedent: no new plan-style docs). But for orientation, the natural dependency
order among the above is:

1. Rename `RayCastingHitElement` → `PostRayHitElement`; introduce
   `PreRayHitElement` (§2). Pure rename + one new empty-ish interface, low
   risk, unlocks the vocabulary for everything after it.
2. Create the `boundingVolumeHierarchy` package; move
   `AxisAlignedBoundingBox` into it as the naive `BoundingVolumeHierarchy`
   implementation; fix the dependency direction so `element` doesn't depend on
   it and `geometry` may (§3).
3. Re-point `render/bakedScene`'s AABB-flavored helpers
   (`AabbCullingSupport`, `OperandCullBins`) at the new package (§5).
4. Only then start moving `geometry`'s actual intersection operations
   (`doIntersectionForAllRayCrossings` and family) into `vitral`, now that
   `element` doesn't carry POV-specific or bounding-volume baggage with it
   (§4, §6).

Each step should keep the gate green throughout, per the project's established
practice on every prior migration/performance effort.
