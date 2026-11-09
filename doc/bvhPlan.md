# Bounding Volume Hierarchy: Prospects Beyond the Single AABB

This document analyzes the performance-improvement prospects of giving
`BoundingVolumeHierarchy`
(`src/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h`)
concrete implementations other than the current single axis-aligned box, and
compares the resulting design space with the conceptual model of the Vulkan
ray-tracing API (`VK_KHR_acceleration_structure`), whose two-level split into
top-level and bottom-level acceleration structures maps surprisingly well onto
what this codebase already does — and onto what it is missing.

It complements `projectClosingSteps.md` §3 (the package layering rationale) and
`performanceNotes.md` (the measured cost model every claim below is checked
against).

## 1. What exists today

`BoundingVolumeHierarchy` is a conservative pre-hit contract (via
`PreRayHitElement`): `mayIntersect` / `mayIntersectBefore` may return false
positives, never false negatives, plus `containsPoint`, `isUnbounded`,
`axisAlignedExtent`, and `copy`. It has exactly one implementation:
`AabbBoundingVolume`, a polymorphic wrapper around the vptr-free value type
`AxisAlignedBoundingBox`. Despite the package name, no implementation is a
*hierarchy* yet — the interface is a placeholder for hierarchies that do not
exist.

Culling actually happens at four independent tiers, and only some of them go
through the interface:

1. **Top-level object scan** — `RenderEngine::trace`
   (`src/render/RenderEngine.cpp:415`) walks `bakedScene.boundedObjectIndices`
   linearly, testing each object's world-space AABB with
   `intersectsRayBefore(ray, currentBestT)`, so the interval shrinks as closer
   hits are found; unbounded objects are always tested. This is a flat list,
   not a tree.
2. **Composite children** — `BakedTrace` tests each child's `worldBounds`
   before recursing (`src/render/bakedScene/BakedTrace.cpp:403`). Note the
   known defect: those bounds are composite-local, not world
   (`performanceNotes.md`, "Known defects").
3. **CSG union operands** — `OperandCullBins` + `AabbCullingSupport`
   (`src/render/bakedScene/AabbCullingSupport.h`): a bake-time, one-level
   binned index over wide union operand lists (activation threshold 17,
   tuned against a real `iortest` regression at 8). Survivors are re-sorted
   descending to reproduce the plain scan's iteration order exactly.
4. **Per-primitive factories** — each `Geometry` implements
   `createBoundingVolume()`; every one of them, including
   `TriangleMesh::createBoundingVolume()`
   (`src/environment/geometry/surface/TriangleMesh.cpp:405`), returns a
   *single* `AabbBoundingVolume`. Consumers immediately flatten it to an
   `AxisAlignedBoundingBox` via `axisAlignedExtent()` at bake time.

The slab test itself is already cheap: reciprocals are cached per ray
(`getAabbSlabReciprocals`), and the value-plus-wrapper split exists because
putting a vptr on the hot-path box type measured as a real regression
(~139 s vs 123 s on the full render gate — `performanceNotes.md`).

## 2. The Vulkan two-level model, and how this codebase maps onto it

Vulkan's ray-tracing acceleration structures are strictly two-level:

- A **BLAS** (bottom-level acceleration structure) is built once, in *object
  space*, over either triangle geometry or **procedural AABBs**. A procedural
  AABB carries no geometry — when a ray enters it, the driver invokes an
  application-supplied *intersection shader* that does the exact math.
- A **TLAS** (top-level acceleration structure) is built over *instances*.
  Each instance is a reference to one BLAS plus a 3×4 transform, a visibility
  mask, and flags. Traversal transforms the ray into the instance's object
  space and continues inside the BLAS.
- Nesting deeper than two levels is forbidden by the API, precisely to bound
  traversal complexity and keep the transform stack depth fixed.
- Ray flags such as `TerminateOnFirstHit`, and the **any-hit** shader stage,
  let occlusion (shadow) queries stop at the first accepted hit instead of
  finding the closest one.
- Build-time knobs (`PREFER_FAST_TRACE` vs `PREFER_FAST_BUILD`, refit vs
  rebuild, compaction) make the build/traversal budget an explicit trade.

The mapping to povCpp is close enough to be a useful design vocabulary:

| Vulkan concept | povCpp analog | Status |
| --- | --- | --- |
| TLAS over instances | `boundedObjectIndices` scan over `BakedTraceableObject` world AABBs | Exists, but **flat** (a degenerate one-level TLAS) |
| Instance transform (ray into object space) | `LocalIntersectionClone` + `transformationInverse` in `CsgOperand` / `SimpleBody` | Exists — same technique, per-ray clone instead of hardware traversal |
| Triangle BLAS | `TriangleMesh` internal structure | **Missing** — single AABB, then a linear scan over all triangles |
| Procedural-AABB BLAS + intersection shader | `createBoundingVolume()` guarding `doIntersectionForAllRayCrossings` on quadric/blob/sphere/box/height-field | Exists conceptually — each primitive *is* a procedural hit group |
| BLAS build command (`vkCmdBuildAccelerationStructures`) | `BakedSceneBuilder` at the pre-render boundary | Exists — the bake step is exactly the "build once, trace many" phase |
| `TerminateOnFirstHit` / any-hit for shadows | `mayIntersectBefore(ray, maxT)` + shadow paths in `DirectLightShader` | Partial — see §3.5 |
| Two-level nesting limit | `Composite` nesting (unbounded depth) | **Violated** — composites are effectively a BLAS containing instances, which Vulkan forbids; the composite-local `worldBounds` defect is a direct symptom of the ambiguity this creates |
| Visibility mask / instance flags | `noShadowFlag` (not consulted on nested paths) | Partial, same nesting caveat |

Two lessons transfer directly. First, Vulkan separates *object-space* bounds
(BLAS, built once) from *world-space* instance bounds (TLAS, rebuilt cheaply),
which is exactly the discipline the composite-local-bounds defect breaks.
Second, Vulkan's model has **no CSG**: it answers "first hit" and "any hit",
never "all crossings with membership annotations". povCpp's depth-queue CSG
traversal is fundamentally outside the two-level model, which correctly
predicts that acceleration structures help CSG *operand culling* but cannot
replace the Morgan/core-plane evaluation itself.

## 3. Alternative implementations, ranked by realistic prospect

The measured cost model (`performanceNotes.md`) constrains expectations: on
`drums` the current tree already executes *half* the baseline's primitive
tests — culling works — and the residual 1.32× suite gap is per-test
orchestration cost (composite ray clones, candidate heap traffic), not
missed culls. Any bounding-volume change attacks test *count*, so its ceiling
is bounded by how many tests remain cullable.

### 3.1 Triangle-mesh BVH (the BLAS analog) — highest prospect

`TriangleMesh::doIntersectionForAllRayCrossings`
(`src/environment/geometry/surface/TriangleMesh.cpp:315`) tests every
triangle on every ray that touches the whole-mesh box. This is the one tier
where the classical O(N) → O(log N) BVH argument applies without caveats:

- Triangles are immutable after parse; a bake-time SAH or median-split build
  fits the existing `BakedSceneBuilder` phase naturally.
- `CsgParser` already fuses same-material triangle runs into single meshes,
  so mesh sizes are the *largest* homogeneous primitive counts in the system.
- A per-mesh BVH is a pure filter (§4): it only skips triangles that provably
  miss, so it is byte-exact-safe by construction, provided crossings are
  still offered to the depth queue and ties resolve identically.
- Prospect is scene-dependent: scenes with large fused meshes gain
  substantially; scenes without meshes gain nothing. Since neither pole scene
  (`drums`, `iortest`) is mesh-dominated, expect *per-scene* wins rather than
  a suite wall-time win.

Implementation shape: a `TriangleMeshBvh final : BoundingVolumeHierarchy`
whose nodes live in a flat array (indices, not pointers, no per-node vptr —
the §4 lesson), returned by `TriangleMesh::createBoundingVolume()` and *kept*
by the mesh for traversal instead of being flattened to one box.

### 3.2 Top-level BVH over objects (the TLAS analog) — low prospect today

Plan 13's Phase 2 was skipped for exactly this reason: top-level object
counts in the 1992-era scene suite are too small for a tree to beat the
linear scan with its shrinking-`t` early-out. A flat scan over a few dozen
world AABBs with cached reciprocals is near-optimal at this scale. The TLAS
analogy matters here as *architecture*, not speed: if the suite ever grows
scenes with hundreds of instances, the right structure is a binned/SAH BVH
over `BakedTraceableObject` records — and the current flat list is already
the correct build input for it. Nothing needs to change pre-emptively.

### 3.3 Deeper union-operand hierarchies — mostly harvested

`OperandCullBins` is a one-level BVH in all but name, and it already
delivered the available wins (`spline` −62%, `ntreal` −19%, `piece3` −12%,
with `drums`/`iortest` flat). Deepening it to a real tree only pays when
operand counts far exceed the threshold (17); the measured regression at
threshold 8 shows the overhead floor is real. Prospect: marginal on the
current suite; revisit only with evidence of unions with hundreds of
operands.

### 3.4 Tighter volume shapes (sphere, OBB, k-DOP) — low prospect

These reduce *false positives*, so they pay only where loose boxes cause
measurable wasted exact tests:

- **Bounding sphere**: cheaper test than a slab, but strictly looser fit for
  almost everything in this scene corpus (boxes, planes-heavy CSG). Not
  promising as a replacement; conceivable as a pre-filter in front of the
  slab test, but the reciprocal-cached slab is already ~cheap enough that
  the added branch likely costs more than it saves.
- **OBB (oriented box)**: tighter for rotated elongated geometry — exactly
  the baked transformed quadrics. But an OBB test is a ray transform plus a
  slab test, i.e. it re-buys per-ray matrix work that transform pushdown and
  identity-fold were built to remove. Only sensible where the false-positive
  rate of the world AABB is demonstrably high (a diagonal thin cylinder), and
  no such hotspot is currently measured.
- **k-DOP**: tighter than AABB at k/2× the plane tests and k/2× the storage
  per node. The suite's residual cost is not false-positive-driven, so this
  is machinery without a measured customer.

The honest summary: `performanceNotes.md` attributes the residual gap to
per-test orchestration, not to rays reaching primitives they should have been
culled from. Tighter volumes optimize the already-cheap part.

### 3.5 Any-hit / first-hit shadow semantics — the real money, but blocked

Vulkan's `TerminateOnFirstHit` is the model's most instructive feature for
this codebase: ~87% of `drums`' composite-space traces are shadow rays, and a
shadow ray needs only *any* occluder in `[0, lightDistance]`, not the closest
crossing with full annotations. The interface already anticipates this
(`mayIntersectBefore`), and `DirectLightShader` already interval-culls — but
a true early-exit shadow traversal through composites was attempted and
**reverted**, because `BakedCsgTrace`'s compiled first-hit path for
`SingleCorePlaneIntersection` is non-minimal (it can return a qualifying hit
that is not the nearest crossing — `performanceNotes.md`, "Known defects").
Any-hit occlusion is where the largest measurable win of everything in this
document lives, and it is gated on fixing that defect first, not on any
bounding-volume shape.

### 3.6 Grids / kd-trees / BSP — not competitive here

vitral's `VoxelVolume` and the `geometricProcessing/` voxelizers make a
uniform grid the cheapest structure to *reuse*, and historical POV-Ray 3
itself used grid-flavored vista/light buffers. But grids degrade under the
"teapot in a stadium" distribution and add a second traversal idiom to
maintain; kd-trees/BSP build better trees for static scenes but with far more
build-code complexity and worse worst cases under the byte-exactness
constraint (§4), since spatial *partitioning* (unlike object partitioning)
can visit the same primitive twice and needs mail-boxing. Modern practice
(embodied by Vulkan/DXR, Embree, OptiX) settled on object-partitioning BVHs
for good reasons; if any hierarchy gets built here, BVH is the one.

## 4. Constraints any new implementation must respect

These are inherited from measured history, not style preferences:

1. **Byte-identical golden gate.** Distinguish two roles:
   - *Filter* (safe): skip candidates that provably cannot intersect / cannot
     beat `currentBestT`. Cannot change the image; `OperandCullBins` and the
     top-level interval test are this.
   - *Enumeration* (dangerous): change the order in which surviving
     candidates are tested. Winner selection uses strict `<` on `t`, so exact
     ties resolve by iteration order; `AabbCullingSupport` re-sorts survivors
     descending for exactly this reason, and `CompositeRecord` child order is
     documented as load-bearing. A hierarchy that changes enumeration order
     must either restore the linear order among survivors (as cull bins do)
     or accept a re-baseline — the former is the established practice.
2. **No vptr on hot-path records.** The 139 s-vs-123 s measurement is the
   package's founding lesson: `AxisAlignedBoundingBox` stays a value type;
   hierarchy nodes should be flat index-linked arrays (the `OperandCullBins`
   idiom), with `BoundingVolumeHierarchy` polymorphism reserved for
   build-time construction and queries.
3. **Build once, at the bake boundary.** All geometry is static per render;
   every structure belongs in `BakedSceneBuilder`'s phase (Vulkan's
   `PREFER_FAST_TRACE` end of the trade). No refit machinery is needed.
4. **Conservative contract.** False positives allowed, false negatives
   forbidden — including under the 1e-6 padding conventions
   `CsgOperand::ensureBakedBounds` already applies, and the `1e30`/`1e29`
   unbounded sentinels of `AxisAlignedBoundingBox`.
5. **Fix the world/local bounds split before flattening anything.** The
   Vulkan model keeps BLAS bounds in object space and instance bounds in
   world space as separate artifacts. The composite-local `worldBounds`
   defect shows povCpp currently blurs this; a TLAS-like structure over
   composites is unsound until child bounds are genuinely world-space.

## 5. Summary of prospects

| Alternative | Analogy | Expected effect on current suite | Verdict |
| --- | --- | --- | --- |
| Triangle-mesh BVH | Triangle BLAS | Per-scene wins on mesh-heavy scenes; poles unaffected | **Build first** when mesh scenes matter |
| Any-hit shadow traversal | `TerminateOnFirstHit` + any-hit | Largest measured target (~87% of drums composite traces) | Blocked on CSG first-hit non-minimality fix |
| Top-level object BVH | TLAS | ~none (object counts too small; shrinking-`t` scan near-optimal) | Architecture placeholder only |
| Deeper operand trees | — | Marginal; one-level bins already harvested the wins | Only with new evidence |
| OBB / k-DOP / spheres | — | Attacks false positives, which are not the measured bottleneck | Not worth the machinery |
| Grid / kd-tree / BSP | pre-BVH history | Worse fit under byte-exactness (mail-boxing, order) | Prefer BVH if anything |

The overall conclusion is the inverse of the naive one. The suite's residual
gap does not come from missing hierarchies — culling already halved the
primitive-test count — so swapping bounding-volume *shapes* buys little. What
the Vulkan comparison actually surfaces is structural: the codebase already
has procedural-AABB "hit groups", an instance-transform mechanism, and a bake
phase; what it lacks is a triangle BLAS (a concrete, safe win where meshes
are large), a sound world/object bounds separation (a prerequisite, currently
a known defect), and first-hit-correct occlusion queries (the one change the
profile says would move the poles).
