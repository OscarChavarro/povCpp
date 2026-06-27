# Plan: `BoundedGeometry` → `SimpleBody`, toward a BVH

This document analyses whether `BoundedGeometry` can be removed and its
responsibilities relocated, and lays out a staged plan to do so. The end goal is
an acceleration-structure-ready scene layer (a Bounding Volume Hierarchy, BVH)
needed before a Vulkan port, with a `geometry` package kept deliberately simple:
**canonical, origin-centred shapes that know nothing about `Material` or
placement**.

Companion reading: `doc/vitralNormalizationAnalysis.md` (the povCpp ↔ VITRAL
convergence map; §4 and §6 are the relevant transform/ownership sections).

---

## 1. Findings: what the two "body" types are today

Despite living in `environment/geometry/`, `BoundedGeometry` is **not a
canonical shape**. It is the de-facto **render-time body**: the unit
`Scene::getObjects()` returns (`ArrayList<BoundedGeometry*>`) and the unit
`RenderEngine::trace`, `LocalSurfaceShader` and `DirectLightShader` iterate.

It bundles four distinct concerns (`BoundedGeometry.h`):

| Concern | Field(s) | Maps to the design goal for the render body |
|---|---|---|
| 1. Geometry | `TransformedGeometry *geometry` | (1) Geometry |
| 2. Material / colour / display flags | `geometryMaterial`, `objectTexture`, `objectColor`, `noShadowFlag` | (2) Material/colors/flags |
| 3. Physical properties | — (none yet) | (3) mass/density/units (future) |
| 4. Acceleration / culling | `boundingShapes[]`, `clippingShapes[]` | (4) acceleration structures (BVH) |

`BoundedGeometry::doIntersectionForAllRayCrossings` (body in `Composite.cpp:78`)
already runs the *acceleration protocol* we want a body to run: test bounding
shapes first and early-out (`boundingRegionTests`), intersect the wrapped
geometry, **stamp the shading attribution** (`objectTexture`/`objectColor`/
`noShadowFlag`) onto every candidate, then reject candidates failing the clipping
tests.

`Composite : BoundedGeometry` (`Composite.h`) adds one field — an
`ArrayList<BoundedGeometry*>` of child bodies — and overrides traversal to fan
out across them. It is the POV `composite { }` grouping (a *group of bodies*,
**not** CSG). Architecturally it is already a **BVH-internal-node**: a node
whose "intersection" is the union of its children's intersections.

### 1.1. The two "bodies" are really a *builder* and a *body*

povCpp's current `SimpleBody` (`environment/scene/SimpleBody.h`) is a
**parse-time-only** bundle `{ TransformedGeometry* geometry, Material* material,
ColorRgba* shapeColor }`. Every primitive parser (`SphereParser`, `BoxParser`,
…) returns one; `ObjectParser` then maps it into a `BoundedGeometry`
(`extractObjectState`/`buildObject`), and `CsgParser` siphons its geometry into a
CSG container (`releaseSimpleBodyGeometry`). After parsing, **no `SimpleBody`
survives into the render path** — its render-time methods
(`doIntersectionForAllRayCrossings`, `normal`, `doContainmentTest`, `copy`) are
vestigial, never called.

A dependency grep confirms the split cleanly:

- The current `SimpleBody` is included **only** by `io/pov/...` parsers and by
  `SceneBuilder` (itself a parse helper used only by those parsers). There is
  **zero** render-path consumer.
- `BoundedGeometry`/`Composite` are what the render path and `ScenePostProcessor`
  touch.

So today we have two classes for "geometry + material", split by *lifecycle*:
a transient **parse fragment** (`SimpleBody`) and a durable **render body**
(`BoundedGeometry`). That is the redundancy to resolve — not by merging them,
but by **naming each for what it is** (§2).

### 1.2. The feared coupling is mostly illusory

The original worry — *"obligar a las Geometry que usan cosas de BoundedGeometry
para que no la usen"* — turns out to be cheap:

- **CSG does not depend on the body at all.** `ConstructiveSolidGeometry` holds
  its children as `ArrayList<TransformedGeometry*>` (`shapes`) + `shapeMaterials`,
  and stamps materials through the `Material* materialOverride` parameter of
  `doIntersectionForAllRayCrossings`. It already obeys the "pass shapes by
  parameter, don't reach into a body" rule. **No work needed.**

- **The "complex geometries" coupling is a stale type-pun.** `Blob`,
  `PolynomialShape` and `ParametricBiCubicSolver` are `TransformedGeometry`
  subclasses (not bodies), yet they have *static helpers* typed
  `BoundedGeometry*`:
  - `Blob::calculateFieldValue(BoundedGeometry *obj, …)` → immediately re-cast
    `const Blob *blob = (Blob *)obj;` (`Blob.cpp:200,496`).
  - `PolynomialShape::quarticNormal(…, BoundedGeometry *object, …)`
    (`PolynomialShape.cpp:724,819`).
  - `ParametricBiCubicSolver::allParametricBiCubicPatchIntersections(
    BoundedGeometry *object, …)` (`ParametricBiCubicSolver.cpp:320`,
    `ParametricPatch.cpp:1202`).

  None ever calls a `BoundedGeometry` method — the parameter is an opaque `this`
  laundered through the wrong type. Retyping it to the concrete class is
  **mechanical and behaviour-preserving**, and removes three geometry-package
  `#include BoundedGeometry.h` dependencies outright.

- **`BoundedGeometry`'s `Geometry` base is barely used.** Only so (a) the scene
  loop can call `doIntersectionFirstHit` uniformly and (b) `Composite` can hold
  `BoundedGeometry*` children polymorphically. The hit's
  `IntersectionAttributes::hitGeometry` is **always the leaf**
  `TransformedGeometry` (set inside `Sphere/Box/Quadric/...
  ::doIntersectionForAllRayCrossings`), never the body. `getWrappedGeometry()`
  is dead (declared, never called).

**Conclusion:** removal is both possible and convenient. The hard dependencies
are imaginary.

---

## 2. Chosen approach: rename by role (not field-merge)

Rather than fuse the parse fragment and the render body into one class, **name
each for its actual role and put it in the right package**:

| Today | Becomes | Package | Role |
|---|---|---|---|
| `SimpleBody` (parse fragment) | **`SimpleBodyBuilder`** | `io/pov/...` | transient parse-time carrier of `{geometry, material, color}`, consumed by `ObjectParser`/`CsgParser` |
| `SimpleBody::wrap` helper (`SceneBuilder`) | `SceneBuilder` (moves with the builder) | `io/pov/...` | parse helper |
| `BoundedGeometry` (render body) | **`SimpleBody`** | `environment/scene/` | durable render-time body: geometry + material/colors/flags + acceleration (+ future physical props) |
| `Composite : BoundedGeometry` | `Composite : SimpleBody` | `environment/scene/` | aggregate/group body; future BVH internal node |

This is preferred over a field-merge because:

- **Lower risk.** The render body is an almost-pure **rename** of
  `BoundedGeometry`; its traversal/ownership logic is untouched.
- **It dissolves the one semantic decision.** A merge would force reconciling the
  fragment's `shapeColor` with the body's `objectColor`. Keeping the builder
  separate keeps the existing explicit builder→body mapping in
  `buildObject`/`extractObjectState` — nothing to decide.
- **Honest packaging.** The grep in §1.1 shows the fragment is io-only; moving it
  to `io` and naming it `…Builder` tells the truth. The render body and its
  aggregate move to `scene`, leaving the `geometry` package **100% canonical
  shapes**.
- **VITRAL alignment.** It makes povCpp's `SimpleBody` the render-time unit in
  the `scene` package — exactly VITRAL's role for `SimpleBody`
  (`doc/vitralNormalizationAnalysis.md` §4.5 / §6, the last big divergence).

> Naming nuance: the fragment sometimes feeds a CSG container (only its geometry
> is taken) rather than building a body, so `SimpleBodyBuilder` is slightly
> narrow; `ParsedShape` / `ShapeBuilder` are alternatives. Pick one at Stage 1.

### 2.1. Target class shape

```
Geometry                 (canonical; transform-free; material-free)   — UNCHANGED
  └─ TransformedGeometry  (adds placement matrix + *Geometry ops)      — UNCHANGED
       ├─ Sphere, Box, Quadric, Triangle, InfinitePlane, Blob,
       │  HeightField, PolynomialShape, ParametricBiCubicPatch, …
       └─ ConstructiveSolidGeometry (children: TransformedGeometry*)   — UNCHANGED

SimpleBody               (ex-BoundedGeometry; render body; in scene/)   — RENAMED
  └─ Composite           (aggregate/group body; SimpleBody* children;
                          future BVH internal node; in scene/)          — REPARENTED + MOVED

SimpleBodyBuilder        (ex-SimpleBody; parse fragment; in io/pov/...)  — RENAMED + MOVED
```

### 2.2. Why `Composite` is the BVH seed

A BVH is a tree of bounding nodes over bodies. `Composite` is already that: an
aggregate body holding child bodies, with its own bounding/clipping volumes.
Keeping `Composite : SimpleBody` means the future BVH internal node is **the same
type** the renderer already traverses uniformly — no new top-level concept, and
`Scene::getObjects()` can later collapse to a single root aggregate. The current
`boundingShapes`/`clippingShapes` are the hand-authored precursor of the
automatic BVH bounds that Stage 4 introduces.

### 2.3. Keeping complex geometry decoupled from the body (the standing rule)

The invariant *"complex geometries like CSG must not depend on `SimpleBody`; pass
acceleration structures by parameter"* holds by construction: CSG keeps
`TransformedGeometry*` children + the `materialOverride` parameter and is never
handed a `SimpleBody*`. When the BVH arrives (Stage 4), any geometry that must
consult it receives a `const Bvh&` / acceleration handle **as a parameter** on
the traversal call (mirroring `materialOverride`). The acceleration structure is
*owned* by `SimpleBody`/`Composite` and *borrowed* by geometry, never reached
into.

---

## 3. Staged migration plan

Each stage is independently buildable and **gated by golden images** (§5).
Stages 0–2 are pure refactors with byte-identical output expected; Stages 3–4
add the BVH.

### Progress

| Stage | Description | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | Sever fake `BoundedGeometry*` coupling in geometry package | ✅ Done | `4e63e6d` | Blob, PolynomialShape, ParametricBiCubicSolver retyped to concrete class; 3 includes dropped |
| 1 | Rename parse fragment `SimpleBody` → `SimpleBodyBuilder`; move to `io/pov/geometry/` | ✅ Done | `ae7ed70` | SceneBuilder moved with it; vestigial render methods dropped; ~22 files updated |
| 2 | Rename render body `BoundedGeometry` → `SimpleBody`; reparent+move `Composite` to `scene/` | ✅ Done | `73805a3` | `BoundedGeometry.h` deleted; `geometry/` is now 100% canonical shapes; latent `PriorityQueuePool.txx` include fixed in `DirectLightShader.cpp` |
| 2.5 | (optional) Sever `SimpleBody`'s `Geometry` base | ⬜ Pending | — | Low priority; purely cosmetic for type graph |
| 3 | Generic AABB primitive on `TransformedGeometry` | ⬜ Pending | — | Prerequisite for BVH; aligns with VITRAL `Geometry::getMinMax()` |
| 4 | Real BVH owned by body layer; replace linear scene loops | ⬜ Pending | — | Prerequisite for Vulkan port |

### Stage 0 — Sever the fake coupling (no behaviour change)

Retype the three static helpers off `BoundedGeometry*` to their concrete class
(`Blob*` / `PolynomialShape*` / `ParametricBiCubicPatch*`) and drop the unused
`#include BoundedGeometry.h` from `Blob.h`, `PolynomialShape.h`,
`ParametricBiCubicSolver.h`. Build + gate. **Expected: byte-identical.** Worth
landing on its own.

### Stage 1 — Rename the parse fragment, move it to `io`

`SimpleBody` → `SimpleBodyBuilder` (choose final name here), moved from
`environment/scene/` into `io/pov/...`; move `SceneBuilder` with it. Update the
~25 parser includes/usages (`grep -rl 'scene/SimpleBody.h'`). Its render-time
vestigial methods can be dropped now (they were never called). Build + gate.
**Expected: byte-identical** (rename only).

### Stage 2 — Rename the render body, reparent + move `Composite`

- `BoundedGeometry` → `SimpleBody`; move its declaration to
  `environment/scene/SimpleBody.h` and its method bodies (currently in
  `Composite.cpp`) into `SimpleBody.cpp`.
- `Composite : BoundedGeometry` → `Composite : SimpleBody`; move from
  `environment/geometry/volume/compound/` to `environment/scene/`; child list
  `ArrayList<BoundedGeometry*>` → `ArrayList<SimpleBody*>`.
- `Scene::Objects`/`getObjects`/`setObjects`, `RenderEngine::trace`,
  `LocalSurfaceShader`, `DirectLightShader`, `ScenePostProcessor::linkLights`,
  `ObjectParser` (`buildObject`/`extractObjectState`/`parseObject`/
  `parseComposite`) → `SimpleBody*` / `Composite*`.
- **Delete `BoundedGeometry.h`.**

Build + **full gate** (`testAgainstGoldenImages.sh`, `testQualities.sh`,
`testCsgRoth.sh`, `gateCsgScene.sh`). **Expected: byte-identical** — pure
rename/move; traversal logic unchanged.

> End of Stage 2: `BoundedGeometry` is gone; the render body is `SimpleBody` in
> `scene`; the `geometry` package holds only canonical shapes + CSG. The primary
> question — *can we eliminate `BoundedGeometry`?* — is answered **yes**.

### Stage 2.5 — (optional) sever `SimpleBody`'s `Geometry` base

Give the new `SimpleBody` its **own** virtual intersection interface
(`doIntersectionForAllRayCrossings`/`doIntersectionFirstHit`/
`doContainmentTest`) instead of inheriting `Geometry`, so the body layer no
longer lives nominally inside the canonical-shape hierarchy. Low priority, purely
cosmetic for the type graph; bounding/clipping shapes and CSG children stay
`TransformedGeometry*` regardless. Defer if it complicates the depth-queue facade.

### Stage 3 — Generic bounds primitive on geometry (enabler)

Add a world-space AABB query (recommended on `TransformedGeometry`, which owns
the placement matrix; CSG/compound compose children's boxes). povCpp has none
today; VITRAL mandates `Geometry::getMinMax()`
(`doc/vitralNormalizationAnalysis.md` §6). This is the data a BVH partitions on.
No traversal change; gate stays green.

### Stage 4 — Real BVH owned by the body layer

- Build a `Bvh` from per-`SimpleBody` AABBs, **owned by** a root
  `Composite`/`SimpleBody` (or `Scene`).
- Replace the linear loops over `Scene::getObjects()` in `RenderEngine::trace`
  and the shadow loop in `DirectLightShader` with a BVH descent.
- Where a geometry must consult the BVH, **pass it by parameter** (mirroring
  `materialOverride`); never store a body pointer inside a `Geometry`.
- Keep the node layout flat/array-friendly so the Vulkan port can map it onto a
  GPU acceleration structure (`VkAccelerationStructure`).

Gate: image-identical (BVH changes traversal order/cost, not results — modulo the
documented float-add non-associativity the threshold tolerates).

---

## 4. Risks and decisions

- **R1 — final builder name.** Decide `SimpleBodyBuilder` vs `ParsedShape` /
  `ShapeBuilder` at Stage 1 (it feeds both bodies and CSG containers). *No
  field-level semantic decision remains* — the shapeColor/objectColor mapping
  stays where it already lives, in `buildObject`/`extractObjectState`.
- **R2 — `objectTexture` ownership subtlety.** `~BoundedGeometry` calls
  `objectTexture->releaseFromOwner()` because the texture may be an *alias* to
  the scene's shared default texture (`PovRayMaterialConstancy` /
  `DefaultTextureAliasTracker`, see `Scene.h` and `ObjectParser`). The renamed
  `SimpleBody` must preserve this exact path; do not "simplify" to `delete`.
- **R3 — `detachOwnership` parse-time dance.** `ObjectParser` builds throwaway
  body wrappers purely to invoke virtual `translate/rotate/scale/invert`, then
  `detachOwnership()` before deleting. Preserve verbatim.
- **R4 — include/build churn.** Renames touch ~25 files (the builder) + ~10 (the
  body). Stage 0 and Stage 1 land separately to shrink Stage 2's blast radius.
- **R5 — `Composite.cpp` holds `BoundedGeometry`'s method bodies.** When the body
  moves to `SimpleBody.cpp` in Stage 2, split that file carefully (the base-body
  methods vs the `Composite` overrides currently coexist there).

---

## 5. Validation

Every stage keeps the golden-image gate green:

- `scripts/testAgainstGoldenImages.sh` — all 108 gate scenes vs reference.
- `scripts/testQualities.sh` — quality-band presets (`+qN`).
- `scripts/testCsgRoth.sh` / `scripts/gateCsgScene.sh` — both CSG strategies.

Stages 0–2 must be **byte-identical** (pure refactor/rename). Stages 3–4 stay
within the existing RMSE/antialias threshold (only traversal order changes).
Standing rule (memory: *sphere-nonunit-direction-fix*): recover toward
`referenceTestImages.old`, never toward a buggy re-baseline.

---

## 6. Summary

- **Eliminate `BoundedGeometry`? Yes** — its only "hard" dependents (CSG, Blob,
  Polynomial, Parametric) either already avoid it or couple through a stale
  type-pun that retypes away with zero behaviour change (Stage 0).
- **How?** Rename by role, not merge: today's parse-time `SimpleBody` becomes
  `SimpleBodyBuilder` in `io`; `BoundedGeometry` becomes `SimpleBody` in `scene`;
  `Composite` reparents onto it and moves to `scene`. This is lower-risk than a
  field-merge, removes the only semantic decision, and leaves the `geometry`
  package purely canonical.
- **Where do responsibilities go?** Into the renamed `SimpleBody` (the four
  concerns of §1), the render-time body — matching VITRAL.
- **Complex geometry stays decoupled** by construction: CSG keeps
  `TransformedGeometry*` children + by-parameter material/acceleration passing;
  the future BVH is owned by the body layer and borrowed by parameter, never
  embedded in a `Geometry`.
