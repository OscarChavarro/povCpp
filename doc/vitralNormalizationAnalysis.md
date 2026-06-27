# Comparative analysis of the intersection model: povCpp ↔ VITRAL

Status snapshot comparing how the `povCpp` raytracer (POV-Ray 1.0, 1992,
rewritten in C++) and the
[vitral library](`https://github.com/OscarChavarro/vitral`) model ray-geometry
intersection and the placement transform.

VITRAL is the reference both projects converge toward, but the convergence is
**bidirectional**: the all-crossings primitive that povCpp needs for CSG and
nested refraction is meant to migrate *upward* into VITRAL (see the alignment
note in §3). The immediate goal is to make the two codebases share as much
shape and naming as possible before that primitive is promoted.

---

## 1. What both projects already share

### 1.1. `RayWithSegments` inherits from VITRAL's `Ray`

`src/environment/geometry/element/RayWithSegments.h`:

```cpp
#include "vsdk/toolkit/environment/geometry/element/Ray.h"   // <- VITRAL's Ray
class RayWithSegments : public Ray { ... };
```

povCpp **has no `Ray` of its own**: it uses VITRAL's `Ray` (`origin`,
`direction`, `t`, with its immutable API `withOrigin/withDirection/withT` and
the mutable `setOrigin/setDirection/setT`). `RayWithSegments` only **adds**
POV-specific state:

- Precomputed quadratic terms for quadric intersection: `position2`,
  `direction2`, `positionDirection`, `mixedPositionPosition`,
  `mixedDirectionDirection`, `mixedPositionDirection`, plus the
  `quadricConstantsCached` flag guarding their (re)computation.
- The containing-media stack for nested transparency/refraction:
  `containingTextures[]`, `containingIORs[]`, `containingIndex`.
- Ray-type flags (`isShadowRay`, `isPrimaryRay`) and context pointers
  (`statistics`, `config`, `intersectionQueuePool`).

The `Ray` class is shared as-is. The divergence between the two libraries is
in the *result* of the intersection and in the *shape* of the operation, not
in the ray.

### 1.2. `Intersection` is a shared class

`Intersection` lives only in
`base/.../vsdk/toolkit/environment/geometry/element/Intersection.h` — povCpp
has no copy of its own. It is byte-identical to upstream VITRAL: public
fields `t`/`point`/`normal` plus two constructors, used as the **purely
geometric** record on both sides.

### 1.3. The nearest-hit adapter: `doIntersectionFirstHit`

`src/environment/geometry/Geometry.cpp`:

```cpp
bool Geometry::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (doIntersectionForAllRayCrossings(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();   // the nearest (the queue is a min-heap by t)
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}
```

This carries **exactly the same name** as VITRAL's canonical primitive
`bool Geometry::doIntersectionFirstHit(const Ray&, RayHit*)`: "give me the
nearest hit and tell me whether there was one". The name is fully reconciled
on both sides; what differs is the *role* — in VITRAL `doIntersectionFirstHit`
is the pure-virtual primitive each geometry implements directly, whereas in
povCpp it is a non-virtual facade built over the `doIntersectionForAllRayCrossings`
primitive (which VITRAL has no equivalent of, see §3).

`SimpleBody` also exposes `doIntersectionFirstHit(RayWithSegments*, IntersectionCandidate&)`
as a direct delegation to `getGeometry()->doIntersectionFirstHit(...)`, giving
the same entry point at the body level.

### 1.4. `PovRayHit` mirrors VITRAL's `RayHit`

`src/environment/geometry/element/PovRayHit.h` is a type **exclusive to
povCpp** whose core fields are named after VITRAL's `RayHit`
(`p`/`n`/`t`/`u`/`v`/`hitDistance`) and which groups the POV extensions
(`hitGeometry`, `material`, `objectTexture`, `objectColor`, `noShadowFlag`).
It is built via `PovRayHit::fromCandidate(const IntersectionCandidate&)` and
used as a **read-only projection** at the shading boundary
(`RayShaderPipeline`, `LocalSurfaceShader`); it does not replace the
`Intersection`+`IntersectionAttributes` storage that travels through the
depth queues.

---

## 2. `Ray`/`RayHit` (VITRAL) vs `Intersection`+`IntersectionAttributes` (povCpp)

### 2.1. VITRAL's model: "nearest-hit with lazy detail"

- `Ray` = `(origin, direction, t)`. The `t` travels in the ray: after
  intersecting, `inRay.withT(distance)` produces the "consumed" ray.
- `RayHit` = the **full result record**: `p` (point), `n` (normal), `t`
  (tangent), `u,v` (texture), `material`, `texture`, `normalMap`, plus
  `hitDistance`, the consumed `Ray`, and a **`requiredDetailMask`**
  (`DETAIL_POINT | DETAIL_NORMAL | DETAIL_UV | DETAIL_TANGENT`).
- The key trick: **lazy per-mask computation**. The raytracer decides *up
  front* what data it needs (`buildSurfaceDetailMask` in
  `SimpleRaytracer.cpp`), and the geometry computes only that
  (`doExtraInformation()`); for shadow rays `hitDistance` is enough.
- The canonical primitive is `bool Geometry::doIntersectionFirstHit(const
  Ray&, RayHit*)` (pure virtual in `Geometry`). Many concrete geometries
  (`Box`, `Sphere`, `Cone`, …) also provide a `Ray* doIntersectionFirstHit(
  const Ray&)` convenience overload that returns the consumed ray.

**VITRAL returns only the nearest hit.** It has no "all crossings"
mechanism, so it **does not support CSG** (booleans) nor the media stack for
nested refraction. Its `SimpleRaytracer` is, by design, simpler than povCpp's
engine.

### 2.2. povCpp's model: "all ray crossings in a depth queue"

Inherited from classic POV-Ray, because it needs it for CSG and nested
refraction indices:

```cpp
virtual int doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride = nullptr);
```

Each surface crossed by the ray is pushed into a priority queue ordered by
`t`. The optional `materialOverride` lets a CSG container hand each leaf the
material it should stamp on its crossings (the leaf records it in the
candidate's attributes); when omitted the leaf records `nullptr`. This single
method replaces the older split between an "all intersections" call and a
material-carrying variant — there is now one primitive.

The unit travelling through that queue is:

```
IntersectionCandidate  =  Intersection  +  IntersectionAttributes
```

- **`Intersection`** (shared): the **purely geometric record** → `t`,
  `point`, `normal`. Equivalent to the *geometric core* of `RayHit`.
- **`IntersectionAttributes`**: the **shading attribution** → `hitGeometry`,
  `material`, `objectTexture`, `objectColor`, `noShadowFlag`. POV-specific
  information that VITRAL embeds inside `RayHit` (only `material`, `texture`,
  `normalMap`) or simply does not have.

They are kept as two separate sub-objects *so `Intersection` stays a plain
t/point/normal record*: the priority queue **compares only by the geometric
part** (`a.getIntersection().t < b.getIntersection().t`) and the attributes
ride along.

### 2.3. Correspondence table

| VITRAL `RayHit`            | povCpp                                          | Notes |
|----------------------------|-------------------------------------------------|-------|
| `p` (point)                | `Intersection::point` (`PovRayHit::p`)          | same concept |
| `n` (normal)               | `Intersection::normal` (`PovRayHit::n`)         | povCpp computes it once for the winner in `RenderEngine::trace`, and only when the ray's `requiredDetailMask` asks for it (`needsNormal()`) |
| `t` (tangent)              | — (does not exist; `PovRayHit::t` reserved)     | povCpp's texturizers derive it if needed |
| `u,v`                      | — (not in `Intersection`)                       | povCpp computes UV inside the texture pipeline |
| `hitDistance` / `Ray.t`    | `Intersection::t` (`PovRayHit::hitDistance`)    | same concept |
| `material`                 | `IntersectionAttributes::material`              | POV `Material` ≠ VITRAL `SimpleMaterial` |
| `texture`                  | `IntersectionAttributes::objectTexture`         | |
| `normalMap`                | — (lives in `Material`)                          | |
| `requiredDetailMask` (on `RayHit`) | `RayWithSegments::requiredDetailMask` | shared concept, same `DETAIL_NONE/POINT/NORMAL/UV/TANGENT/ALL` constants; povCpp keeps it on the *ray* (not the hit), because the decision is made before the hit exists, and gates only `NORMAL` (see §8) |
| — (does not exist)         | `IntersectionAttributes::{hitGeometry,objectColor,noShadowFlag}` | POV/CSG-specific |

**Summary:** `Intersection` (shared) = **geometric half** of `RayHit`;
`IntersectionAttributes` = the shading attribution VITRAL embeds in `RayHit`.
They are split in povCpp so the CSG queue can order by `t` alone. `PovRayHit`
is the named mirror that joins both halves with VITRAL's names.

---

## 3. The central architectural tension

| Aspect               | VITRAL                              | povCpp |
|----------------------|--------------------------------------|--------|
| Geometric primitive  | `doIntersectionFirstHit(Ray&, RayHit*)` | `doIntersectionForAllRayCrossings(Ray*, PriorityQueue*, Material*)` |
| Result               | nearest hit (1)                     | all crossings (N), ordered |
| CSG / booleans       | not supported                       | supported (needs the N) |
| Nested refraction    | no                                  | yes (containing-media stack) |
| Surface detail       | lazy by mask (point/normal/UV/tangent) | lazy by mask (normal only, all-or-nothing); UV in the texturizer |
| Hit record           | `RayHit` (merged)                   | `Intersection` + `IntersectionAttributes` (projected into `PovRayHit`) |

**`doIntersectionForAllRayCrossings` cannot be replaced by nearest-hit
`doIntersectionFirstHit`**: CSG and refraction need the full set of surface
crossings. povCpp's `doIntersectionFirstHit` (§1.3) sits on top of
`doIntersectionForAllRayCrossings` rather than replacing it.

> **Alignment intent.** It is an explicit intention to promote
> `doIntersectionForAllRayCrossings` *from povCpp up into VITRAL*, so that
> VITRAL gains an all-crossings primitive alongside its nearest-hit
> `doIntersectionFirstHit` and can host CSG and nested refraction itself.
> Before that promotion happens, the two projects are being aligned as much as
> possible (shared `Ray`/`Intersection`, matching method names, the
> transform-free `Geometry` base of §4) so the operation lands on top of an
> already-converged surface rather than dragging divergence into VITRAL.

---

## 4. The transformation model: where placement lives

Both projects keep their concrete shapes **canonical** and carry placement in a
4×4 matrix applied to the *ray* at intersection time (transform the ray into
object space, intersect, transform the result back; `t` is invariant under the
affine ray-transform because the parameterization is linear). What differs is
**which object owns that matrix**.

### 4.1. VITRAL: the transform lives in `SimpleBody`

`vsdk/.../environment/scene/SimpleBody.h` holds the placement, decomposed:

- `Vector3Dd position`, `Vector3Dd scale` / `inverseScale`,
  `Matrix4x4d rotation` / `rotationInverse`, plus
  `Quaterniond rotationQuaternion` / `rotationInverseQuaternion`.
- A set of **fast-path flags** computed by `updateTransformFlags()`:
  `hasIdentityRotation`, `hasUnitScale`, `hasZeroTranslation`,
  `hasTranslationOnlyTransform`, `hasIdentityTransform`, `hasInvertibleScale`.
- `SimpleBody::doIntersectionFirstHit` applies the inverse transform to the
  ray and dispatches to specialized paths
  (`doIntersectionWithTranslationOnly`,
  `doIntersectionWithTranslationOnlySphereFastPath`) when the flags allow.

The `Geometry` it wraps stays **shared and canonical**: VITRAL's `Sphere`, for
instance, carries only `radius_`/`radiusSquared_` and is centered at the
origin — position, scale and rotation are entirely `SimpleBody`'s business. The
same `Geometry` instance can in principle be placed by several bodies.

### 4.2. povCpp: the transform lives in `TransformedGeometry` (the geometry)

`src/environment/geometry/TransformedGeometry.h` is **a base class**, not a
wrapper. Every concrete shape inherits it — `Sphere`, `Box`, `Quadric`,
`Triangle`, `InfinitePlane`, `Blob`, `HeightField`, `ParametricBiCubicPatch`,
`PolynomialShape`, `ConstructiveSolidGeometry`, `LightGeometryAdapter` — so the
placement matrix is a member of the shape itself:

```cpp
class TransformedGeometry : public Geometry {
  protected:
    Matrix4x4d *transformation = nullptr;          // nullptr == identity
    Matrix4x4d *transformationInverse = nullptr;
  public:
    virtual void translateGeometry(Vector3Dd *vector);
    virtual void rotateGeometry(Vector3Dd *vector);
    virtual void scaleGeometry(Vector3Dd *vector);
};
```

- A **single combined matrix** (plus its inverse) is accumulated at parse time
  by `translateGeometry`/`rotateGeometry`/`scaleGeometry`. There is no
  position/scale/rotation decomposition and no fast-path flag set — the leaf
  always goes through the general inverse-matrix path (`nullptr` short-circuits
  to the identity case).
- The matrices are lazily allocated (`ensureMatrices()`), so an untransformed
  shape pays nothing and intersects in world space directly.
- Each leaf's `doIntersectionForAllRayCrossings` does the object-space
  transform itself: e.g. `Sphere::intersectSphere` maps `ray->getOrigin()`/
  `getDirection()` through `transformationInverse`, intersects the canonical
  unit sphere, and divides `t` back by the (non-unit) object-space direction
  length. `normal()` maps the point through the inverse and back through
  `withoutTranslation()` of the inverse.

So povCpp's `Sphere` is a **strict canonical unit sphere at the origin**: even
the radius is carried by the matrix (`scale(r)`), unlike VITRAL's radius-bearing
`Sphere`.

### 4.3. `Geometry` is transform-free on both sides

This is the part that is now **aligned**. povCpp's `Geometry` base
(`src/environment/geometry/Geometry.h`) declares no `translate/rotate/scale`
and no matrix — only the intersection/containment/normal/copy primitives,
exactly like VITRAL's `Geometry` (which also has no transform). The transform
ops live one level down: in `TransformedGeometry` (povCpp) or in `SimpleBody`
(VITRAL). The divergence is purely *which* level: povCpp pushes the matrix into
the geometry subtype; VITRAL keeps it in the scene-level body.

### 4.4. Compound and CSG transforms push down to children

povCpp's container geometries do **not** accumulate a matrix of their own;
they forward the transform into their children at parse time:

- `ConstructiveSolidGeometry` (itself a `TransformedGeometry`) overrides
  `translateGeometry`/`rotateGeometry`/`scaleGeometry` to walk its
  `shapes` (`ArrayList<TransformedGeometry*>`) and call each child's own
  `*Geometry` op, transforming the parallel `shapeMaterials` to match.
- `BoundedGeometry`/`Composite` expose virtual `translate`/`rotate`/`scale`
  that propagate into the bounded geometry and into nested composites'
  children (the virtual dispatch is what lets an outer composite's transform
  reach a nested composite's children).

The result is that after parsing, every leaf carries the full
already-composed world transform in its own `TransformedGeometry` matrix; there
is no transform left to apply at the container level during traversal.

### 4.5. `SimpleBody` is now a thin parse-time bundle

povCpp's `SimpleBody` (`src/environment/scene/SimpleBody.h`) is **not** a
`Geometry` subclass and is **not** the scene's render-time unit. It is a small
holder of `{TransformedGeometry* + Material* + ColorRgba*}` used while parsing;
its `translate`/`rotate`/`scale` delegate to the geometry's `*Geometry` ops
*and* transform the held material, and `releaseGeometry()`/`releaseMaterial()`/
`releaseShapeColor()` hand ownership off into the long-lived structures
(`ConstructiveSolidGeometry::addShape`, `BoundedGeometry`). The render path
iterates `Scene::getObjects()` as `ArrayList<BoundedGeometry*>`, where each
`BoundedGeometry` holds its `TransformedGeometry* geometry` + `Material*
geometryMaterial` directly — not a `SimpleBody`.

This is the inverse of VITRAL, where `SimpleBody` *is* the render-time unit and
the holder of the transform. The naming matches; the responsibility does not.

---

## 5. CSG in povCpp: two interchangeable algorithms

`ConstructiveSolidGeometry` (`environment/geometry/volume/constructiveSolidGeometry/`)
is an abstract base (a `TransformedGeometry` that owns the child list as
`shapes`/`shapeMaterials` and the `UNION`/`INTERSECTION`/`DIFFERENCE`
`geometryType`) with two concrete strategies. Which one is instantiated is
decided by `CsgParser::parse`, based on `ParserContext::usesCsgRoth()` (the
`-csgRoth` command-line flag):

### 5.1. `ConstructiveSolidGeometryByMorganRules` (default)

Point-membership classification, no `-csgRoth` flag needed:

- Gathers **all** of each child's crossings into a queue
  (`allCsgUnionIntersections`/`allCsgIntersectIntersections`).
- For **each** candidate, walks **all the other children** and calls
  `doContainmentTest(point, tolerance)` to decide whether the crossing
  survives (`insideCsgUnion`/`insideCsgIntersection`, via `insideCsgChild`).
- `difference` has no dedicated `geometryType`: `CsgParser` builds an
  `INTERSECTION` container and calls `invert()` on each non-first child.
  `ConstructiveSolidGeometryByMorganRules::invertGeometry()` flips
  `UNION↔INTERSECTION` on itself and recursively calls `invert()` on every
  child.
- Cost: for K children and M crossings, ~O(K · M · K) `doContainmentTest`
  calls, and its correctness depends on `doContainmentTest` being reliable at
  points on the surface (where tolerance handling is fragile).

### 5.2. `ConstructiveSolidGeometryByRaySegment` (`-csgRoth`)

Ray-segment classification per
`doc/references/[ROTH1982]_RayCastingForModelingSolids.pdf` (Scott D. Roth,
*Ray Casting for Modeling Solids*, CGIP 18, 109-144, 1982), §3.3:

- Each child contributes a `RaySegments` (in/out intervals along the ray),
  built by `buildRaySegments`.
- Segments are merged by `mergeUnion`/`mergeIntersection`/`mergeDifference`,
  all backed by the generic `mergeByMembership` (a single O(M log M)
  ordered-crossing merge, not nested containment tests).
- `DIFFERENCE` is a first-class `geometryType` here (unlike the
  Morgan-rules path, it is **not** folded into `INTERSECTION` + per-child
  `invert()` during parsing).
- `topLevel` (`setTopLevel`/`isTopLevel`, set by `CsgParser` from `!isNested`)
  marks the outermost CSG node so it can apply boundary handling that only
  makes sense once, at the top of a nested CSG tree.
- The leaf solids (e.g. `Sphere::doIntersectionForAllRayCrossings`) already
  emit both crossings (entry `depth1`, exit `depth2`); this path consumes them
  directly as segment endpoints instead of re-deriving "inside" via
  point-membership.
- Because segments *are* the "inside the solid" intervals, this path
  connects directly to `containingTextures[]`/`containingIORs[]` of
  `RayWithSegments` for nested refraction, and avoids `doContainmentTest`
  fragility at surface points.

Both strategies implement the same `ConstructiveSolidGeometry` interface
(`doIntersectionForAllRayCrossings`, `doContainmentTest`, `copy`,
`invertGeometry`), so the rest of the engine (CSG composition, transforms, the
depth-queue consumers) is agnostic to which one is active.

---

## 6. Correspondence table (current state)

| povCpp concept | VITRAL equivalent | Status |
|---|---|---|
| `RayWithSegments : Ray` | `Ray` (+ segments) | shared base |
| `Intersection` | `Intersection` (identical) | shared class |
| `IntersectionCandidate` (Intersection + Attributes) | `RayHit` | mirrored via `PovRayHit` (read-only projection) |
| `Geometry` (pure intersection primitive) | `Geometry` (pure intersection primitive) | **aligned**: neither base carries any transform — only `doIntersectionForAllRayCrossings`/`doContainmentTest`/`doIntersectionFirstHit`/`normal`/`copy`/`invertGeometry` (povCpp) and `doIntersectionFirstHit`/`doContainmentTest`/`doExtraInformation`/`getMinMax` (VITRAL). `translate/rotate/scale` exist on neither `Geometry` |
| `TransformedGeometry` (base class: matrix + `*Geometry` ops, inherited by every leaf) | `SimpleBody` (holds the transform) | **divergent ownership**: povCpp puts the placement matrix *in the geometry subtype*; VITRAL puts it in the scene-level `SimpleBody`. Same canonical-shape + inverse-matrix-on-ray idea, different owner. See §4 |
| `SimpleBody` (`{TransformedGeometry* + Material* + ColorRgba*}`) | `SimpleBody` (render-time unit + transform holder) | **name shared, role divergent**: povCpp `SimpleBody` is a thin *parse-time* bundle, not a `Geometry` and not the render unit; the render path holds `BoundedGeometry*` directly. VITRAL `SimpleBody` is the render unit and owns the transform. See §4.5 |
| `Geometry::doIntersectionForAllRayCrossings(Ray*, PriorityQueue*, Material* = nullptr)` | — (no equivalent) | **POV-only**, the all-crossings primitive CSG/refraction need. Intended to migrate into VITRAL (§3) |
| `Geometry::doIntersectionFirstHit` | `Geometry::doIntersectionFirstHit(Ray&,RayHit*)` | **same name on both sides**; in VITRAL it is the pure-virtual primitive, in povCpp a non-virtual facade over `doIntersectionForAllRayCrossings`. `SimpleBody` also exposes it by delegating to its wrapped `TransformedGeometry` |
| `doContainmentTest(const Vector3Dd&, double)` | `Geometry::doContainmentTest(const Vector3Dd&, double)` | **signature identical**; both return `int` with the same constant names *and* values `INSIDE=1`/`LIMIT=0`/`OUTSIDE=-1`. The only difference: VITRAL backs them with `enum class Containment`, povCpp with bare `static constexpr int` on `Geometry` |
| `doExtraInformation(RayWithSegments&, t, PovRayHit*)` | `doExtraInformation(Ray,t,RayHit)` | signature aligned; forwards to `normal()`. Lazy: `RenderEngine::trace` calls it only when the ray's `requiredDetailMask` has `DETAIL_NORMAL` (`needsNormal()`), matching VITRAL's mask-gated `doExtraInformation` |
| `PriorityQueuePool` | per-level `RayHit` workspace | same "no-alloc in hot path" pattern |
| `Statistics` | `RaytraceStatistics` | **not unified**: povCpp keeps per-instance counters keyed by primitive type (`raySphereTests`, `rayBoxTests`, …, each with a `*Succeeded` pair); VITRAL exposes static, per-ray-category recorders (`recordPrimaryRay`, `recordShadowRay`, `recordObjectIntersectionTest`, …). Different granularity and different ownership model — no 1:1 mapping to align toward |
| `PovRayRendererConfiguration : RendererConfiguration` (`PovRayRendererConfiguration.h`) | `RendererConfiguration` | **inherits from VITRAL's base**: `SHADING_TYPE_*` constants and `getShadingType()` come from the base; `setSurfaceLightingEnabled()` keeps the base's `shadingType` field in sync. povCpp adds an `options` bitmask with render/quality flags (`WITH_SURFACE_LIGHTING`, `WITH_SHADOWS`, …, plus IO/threading/antialias config) that have no equivalent in VITRAL's display-oriented base. Owned by `PovRayApplication`, propagated as `const PovRayRendererConfiguration&`. See §8 |
| — (no general bounding box) | `Geometry::getMinMax()` | **divergence**: VITRAL mandates `virtual double* getMinMax() = 0` on every geometry; povCpp has no such method on `Geometry`, only `HeightField`'s class-specific bounds |
| `CameraSnapshot` in `Scene` + parser-local `PovCameraSpec` | `Camera` + `CameraSnapshot` (`vsdk/.../environment/camera/`) | **render-time storage shared**: povCpp renders from VITRAL's `CameraSnapshot`, while the POV parser keeps a local five-vector `PovCameraSpec` so `look_at` and post-declaration transforms stay byte-identical. `RenderEngine::createRay` reads `eyePosition`/`dir`/`upWithScale`/`rightWithScale` directly from the snapshot but applies POV's own sampling/normalization. See §9 |

---

## 7. Open divergences

- **Transform ownership (§4)**: povCpp carries the placement matrix inside the
  geometry (`TransformedGeometry` base), VITRAL inside the scene-level
  `SimpleBody`. The canonical-shape + inverse-matrix-on-ray mechanism is the
  same; the owning object is not. VITRAL additionally decomposes the transform
  (position/scale/rotation + fast-path flags) where povCpp keeps one combined
  matrix and its inverse.
- **All-crossings primitive (§3)**: `doIntersectionForAllRayCrossings` exists
  only in povCpp; VITRAL has the nearest-hit primitive only. This is the
  operation slated to migrate into VITRAL.
- **Lazy surface-detail mask granularity**: `RayWithSegments::requiredDetailMask`
  carries the same `DETAIL_*` constants as VITRAL, and `RenderEngine::trace`
  skips the winner's normal when it is not needed (shadow rays and the `q0-1`
  preview both request `DETAIL_NONE`). The divergence is *granularity*: VITRAL
  gates `POINT`/`NORMAL`/`UV`/`TANGENT` independently, whereas povCpp's gate is
  all-or-nothing on `NORMAL` (UV is produced inside the texture pipeline, not
  requested through the mask).
- **`doContainmentTest` backing type**: same signature, same `INSIDE=1`/
  `LIMIT=0`/`OUTSIDE=-1` constant names and values. The divergence is only that
  VITRAL derives them from `enum class Containment` while povCpp declares bare
  `int` constants; povCpp does not return the enum type itself.
- **Statistics**: structurally and conceptually different between the two
  codebases (per-instance per-primitive counters vs static per-ray-category
  recorders), solving different problems.
- **`PovRayRendererConfiguration` scope**: povCpp's class inherits from
  VITRAL's `RendererConfiguration`, sharing the `SHADING_TYPE_*` constants and
  `getShadingType()`. The POV-specific `options` bitmask (render quality flags,
  IO/threading/antialias config) lives only in the derived class; VITRAL's
  display-only fields (`wires`, `points`, `normals`, `boundingVolume`, `wireColor`,
  …) are inherited but unused in the rendering path.
- **No general bounding-box primitive**: VITRAL requires every geometry to
  implement `getMinMax()`; povCpp has no such concept at the
  `Geometry` level (only `HeightField` has a local one), so CSG and other
  compound geometries cannot be bounded generically.
- **Camera generator differs**: povCpp stores the render-time camera as VITRAL
  `CameraSnapshot`, fed from a parser-local `PovCameraSpec` that preserves POV's
  five raw vectors. The divergence is in sampling and normalization only: VITRAL
  samples pixel **centres** with `+0.5` and leaves the direction unnormalised,
  while povCpp samples integer coordinates and then applies `normalizedFast()`.
  See §9.

---

## 8. Lazy/selective work: VITRAL's detail mask vs povCpp's `+qN` quality

Both engines skip work that the final image does not need, using the same two
mechanisms: an orthogonal feature-flag set on the configuration object and a
per-ray surface-detail mask. In povCpp `+qN` is a *named preset* over those
flags. This section describes how the two map onto each other and where they
still differ.

### 8.1. VITRAL: an orthogonal set of feature requirements

VITRAL's selectivity is **two cooperating, orthogonal mechanisms**:

- **`RayHit::requiredDetailMask`** — a bitmask
  (`DETAIL_POINT | DETAIL_NORMAL | DETAIL_UV | DETAIL_TANGENT`, plus
  `DETAIL_NONE`/`DETAIL_ALL`). The raytracer decides per ray which surface
  details are actually needed (`SimpleRaytracer::buildSurfaceDetailMask`),
  and `doExtraInformation()` computes only those. A shadow ray asks for
  `DETAIL_NONE` and pays only for `hitDistance`; a textured primary ray asks
  for normal+UV. The mask is queried through `needsPoint()`/`needsNormal()`/
  `needsTextureCoordinates()`/`needsTangent()`.
- **`RendererConfiguration`** — independent feature booleans for the *display
  intent* (`isSurfacesSet`, `isTextureSet`, `isBumpMapSet`, `isNormalsSet`,
  the `shadingType` among `NOLIGHT`/`FLAT`/`GOURAUD`/`PHONG`/`COOK_TERRANCE`,
  …). These can be toggled in any combination.

The key property is **orthogonality**: "with texture", "with bump map", "flat
vs Phong shading", and "which surface details to fill in" are independent
switches. Turning one off does not imply any particular state of the others.

### 8.2. povCpp: an orthogonal flag set with `+qN` as a preset

The shading pipeline queries **feature predicates** on
`PovRayRendererConfiguration`, each backed by a bit in the `options` bitmask:

| Feature | Predicate (queried by the shaders) | Where read |
|---|---|---|
| Surface lit at all (normal + ambient + diffuse/specular) | `withSurfaceLighting()` | `LocalSurfaceShader.cpp:36` |
| Shadow-ray object tests (cast shadows) | `withShadows()` | `DirectLightShader.cpp:67` |
| Full pigment/texture eval (else `quickColor`/`objectColor`/grey) | `withTextures()` | `RayShaderPipeline.cpp:73` |
| Transparent/coloured shadows (shadow ray not short-circuited) | `withFilteredShadows()` | `RayShaderPipeline.cpp:60` |
| Refraction / transmission | `withRefraction()` | `RayShaderPipeline.cpp:125` |
| Bump-mapped normal (surface and refraction paths) | `withBumpMapping()` | `LocalSurfaceShader.cpp:46`, `RayShaderPipeline.cpp:132` |
| Mirror reflection | `withReflection()` | `LocalSurfaceShader.cpp:67` |
| Shading model (synced to base field) | `getShadingType()` → `SHADING_TYPE_NOLIGHT`/`PHONG` (inherited from `RendererConfiguration`; `setSurfaceLightingEnabled()` calls `setShadingType()` to keep it current) | — |

`+qN` is **a preset over these flags**, exclusive to the command-line layer:

- `setQuality(N)` maps the integer band onto the seven bits to reproduce
  classic POV-Ray's quality bands bit-for-bit (q0-1: nothing; q2-3: +lighting;
  q4-5: +shadows; q6-7: +textures/filtered-shadows/refraction; q8-9:
  +bump/reflection). The flags are the only stored state; no `quality` integer
  is retained, since nothing reads one back.
- `setQuality()` is **exclusive to the command-line layer**: its only caller is
  `CommandLineOptions::parseOption` (the `+qN` switch). The default-construction
  path does **not** route through it — `PovRayRendererConfiguration::reset()` sets
  the seven feature bits on directly (full quality, identical to the `setQuality(9)`
  bit pattern) rather than invoking the `+qN` preset, so the band-preset code
  never executes outside the command line. `+qflags<letters>` likewise toggles
  individual bits directly (`L`/`S`/`T`/`F`/`R`/`B`/`M`), so any subset is
  reachable from the command line (e.g. `+q9 -qflagsS` = full minus shadows),
  not only the band presets.

The configuration object is **owned by `PovRayApplication`** (a value member,
`PovRayApplication::configuration`) and propagated by reference from
construction: it is handed to `RenderContext` (`const PovRayRendererConfiguration&`),
reaches `RenderEngine` through the context, and each `RayWithSegments` carries
a `const PovRayRendererConfiguration*` (`setConfig`/`getConfig`) so the shaders read
the same instance. No global, no per-shader copy, no re-derivation.

The **per-ray detail mask** is the second mechanism (§7): `RenderEngine::trace`
sets the primary ray's `requiredDetailMask` to `DETAIL_ALL` when
`withSurfaceLighting()` and `DETAIL_NONE` otherwise, and skips the winner's
normal computation (`doExtraInformation`) when the mask does not request it;
`DirectLightShader` marks shadow rays `DETAIL_NONE`. This is the "fast preview
cheaper than the full model" lever, expressed exactly as VITRAL expresses it.

### 8.3. Shared structure and residual differences

povCpp's selectivity and VITRAL's `requiredDetailMask` +
`RendererConfiguration` (base) flags are **the same two encodings of the same idea** —
an orthogonal feature-flag set plus a per-ray detail mask. What they share:

- **Feature flags.** The shaders query orthogonal predicates rather than a
  numeric threshold. Arbitrary subsets ("textures but no shadows") are
  expressible via the direct setters and `+qflags`; `+qN` is one hand-picked
  path through that flag space, kept bit-identical and guarded by
  `scripts/testQualities.sh` (which covers all 108 gate scenes at one quality
  per band, plus `iortest` at all ten levels).
- **Per-ray laziness.** Shadow and primary rays carry an explicit
  `requiredDetailMask`, the same mechanism as VITRAL's `buildSurfaceDetailMask`.
- **Shading vocabulary.** `getShadingType()` reports VITRAL's `SHADING_TYPE_*`
  values, so povCpp's "no lighting" preview and full model map onto VITRAL's
  `NOLIGHT`/`PHONG`.

Residual differences:

- **Engine traversal stays separate.** The flags select *what* to compute; they
  do not touch the `doIntersectionForAllRayCrossings` + CSG + containing-media
  machinery that povCpp needs and VITRAL's nearest-hit `SimpleRaytracer` lacks
  (§3). The shared part is the selection vocabulary, not the traversal.
- **Mask granularity.** povCpp gates `NORMAL` all-or-nothing; VITRAL gates
  `POINT`/`NORMAL`/`UV`/`TANGENT` independently. povCpp computes UV inside the
  texture pipeline, not on request through the mask.
- **`shadingType` synced, not independently settable.** `PovRayRendererConfiguration`
  implements no `FLAT`, `GOURAUD` or `COOK_TERRANCE` shader, so `getShadingType()`
  (inherited from `RendererConfiguration`) only ever returns `NOLIGHT` or `PHONG`.
  `setSurfaceLightingEnabled()` calls `RendererConfiguration::setShadingType()` to
  keep the inherited field current; there is no independent setter for shading mode.
- **`PovRayRendererConfiguration` scope.** The class inherits the `SHADING_TYPE_*`
  constants and `getShadingType()` from VITRAL's `RendererConfiguration`. The
  POV-specific `options` bitmask (render-quality flags, IO/threading config) has no
  equivalent in the base, and the base's display-only fields (`wires`, `points`,
  `normals`, `wireColor`, …) are inherited but unused in the rendering path (§6).

---

## 9. The camera: `PovCameraSpec`/`CameraSnapshot` (povCpp) vs `Camera`/`CameraSnapshot` (VITRAL)

povCpp renders from VITRAL `CameraSnapshot`, while the parser keeps a local
`PovCameraSpec` to preserve POV's mutable five-vector grammar.

### 9.1. povCpp's parse-time camera: POV-Ray's five raw vectors

`src/io/pov/camera/PovCameraSpec.h` stores exactly the classic POV-Ray 1.0 view
record — five `Vector3Dd` with **magnitude semantics**:

- `location` — eye position.
- `direction` — view axis; its **length sets the focal length / field of view**.
- `up`, `right` — half-extents of the projection window; their **lengths encode
  the vertical FOV and the aspect ratio** (POV's default `right <1.33,0,0>` is
  the 4:3 aspect), and they need be neither unit nor mutually orthogonal.
- `sky` — only consumed transiently by `look_at` to re-derive `right`/`up`; it
  is not read at render time.

`CameraParser` mutates these vectors in place (`location`/`direction`/`up`/
`right`/`sky`, plus `look_at`/`translate`/`rotate`/`scale`), then bakes the
result into the `Scene`'s `CameraSnapshot` (`Scene::viewPoint`), and
`RenderEngine::createRay` turns pixel `(x,y)` into a ray:

```cpp
xScalar = (x - width/2) / width;
yScalar = ((screenHeight-1 - y) - height/2) / height;
direction = up*yScalar + right*xScalar + cameraDirection;   // raw vectors
direction = direction.normalizedFast();                     // POV fast inverse-sqrt
origin    = location;
```

### 9.2. VITRAL's camera: orthonormal frame + FOV, baked into a snapshot

`vsdk/.../environment/camera/Camera.h` parametrises the same view differently:
an orthonormal basis (`front`/`left`/`up`), an eye position, and scalar `fov`/
`projectionMode`/`near`/`far`/`viewport`. From these `updateVectors()` **bakes**
the per-pixel scale vectors and `exportToCameraSnapshot()` freezes them into an
immutable `CameraSnapshot`:

- `eyePosition`,
- `dir          = front · 0.5`,
- `upWithScale  = up · tan(fov/2)`,
- `rightWithScale = left · (−aspect · tan(fov/2))`.

VITRAL's raytracer does **not** generate rays from `Camera`; it consumes the
`CameraSnapshot` (`SimpleRaytracer::generateRay(const CameraSnapshot*, x, y)`):

```cpp
u = ((x+0.5) - vpX/2) / vpX;
v = ((vpY - (y+0.5)) - vpY/2) / vpY;
direction = rightWithScale*u + upWithScale*v + dir;   // NOT normalized
origin    = eyePosition;
```

### 9.3. The 1:1 mapping, and the two ways the generators differ

The two ray formulas are the **same expression** — `rightScale·u + upScale·v +
dir` from an eye origin — and povCpp's raw vectors line up exactly with the
snapshot's baked vectors:

| povCpp `Camera` | VITRAL `CameraSnapshot` | role |
|---|---|---|
| `location`  | `eyePosition`     | ray origin |
| `direction` | `dir`             | view-axis term |
| `up`        | `upWithScale`     | vertical pixel scale |
| `right`     | `rightWithScale`  | horizontal pixel scale |
| `sky`       | — (parse-only)    | not needed at render time |

Because `+` is commutative in IEEE-754, povCpp's `up*yScalar + right*xScalar`
and VITRAL's `rightWithScale*u + upWithScale*v` produce **bit-identical** sums
when fed the same vectors and the same `u/v`. The data is shared through
`CameraSnapshot`; the two differences both live in the *generator*, not the data:

1. **Pixel sampling.** VITRAL samples pixel **centres** (`x+0.5`, `vpY-(y+0.5)`);
   povCpp samples integer coordinates with a `screenHeight-1` offset. The two
   `u/v` differ by half a pixel on each axis.
2. **Normalization.** povCpp normalises the direction with POV's `normalizedFast()`
   (fast inverse-sqrt approximation, *not* scale-invariant at the bit level);
   VITRAL returns the direction unnormalised.

So the storage is shared with VITRAL's `CameraSnapshot` with no arithmetic
change; the *sampling/normalization* steps are the only pixel-changing
differences that remain on povCpp's side.
