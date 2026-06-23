# Comparative analysis of the intersection model: povCpp ↔ VITRAL

**Descriptive** document (not a plan). It compares how the `povCpp` raytracer
(POV-Ray 1.0, 1992, rewritten in C++) and the **VITRAL** library model
ray-geometry intersection, in order to understand what they already share, what
diverges, and why. The convergence effort keeps VITRAL frozen and mutates povCpp
gradually, keeping the *gate* green:

```
./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh   # => "Test passed."
```

and without losing performance.

> **Recorded performance lesson (commit `9124240`).** Moving a *hot path* class
> into the base layer **must not define its constructors/accessors out of line in
> a `.cpp`** while the project compiles with `-O3` **without `-flto`**. The first
> migrated version defined `Intersection`'s constructors in `Intersection.cpp`
> (non-inline) and the render went from **99 s to 138 s (+41 %)**, because
> `IntersectionCandidate` is stack-constructed in 14+ *hot path* sites (one
> ray-object test per primitive, per ray). The fix was **inline in the header**,
> returning to 99.6 s. General rule: **POD hot-path classes → header-only/inline**;
> measure `renderAll.sh`, not just "Test passed.".

---

## 1. What both projects already share

### 1.1. `RayWithSegments` ALREADY inherits from VITRAL's `Ray`

`src/environment/geometry/element/RayWithSegments.h`:

```cpp
#include "vsdk/toolkit/environment/geometry/element/Ray.h"   // <- VITRAL's Ray
class RayWithSegments : public Ray { ... };
```

povCpp **has no `Ray` of its own**: it uses VITRAL's `Ray` (`origin`,
`direction`, `t`, with its immutable API `withOrigin/withDirection/withT` and the
mutable `setOrigin/setDirection/setT`). `RayWithSegments` only **adds**
POV-specific state:

- Precomputed quadratic terms (`position2`, `direction2`, `positionDirection`,
  `mixed*`) to speed up quadric intersection.
- The containing-media stack for nested transparency/refraction
  (`containingTextures[]`, `containingIORs[]`, `containingIndex`).
- Ray-type flags (`isShadowRay`, `isPrimaryRay`) and context pointers
  (`statistics`, `config`, `intersectionQueuePool`).

**Conclusion:** the `Ray` class is already reconciled. The divergence is in the
*result* of the intersection and in the *shape* of the operation, not in the ray.

### 1.2. `Intersection` is a shared class (commit `9124240`)

povCpp's own `Intersection.h` was removed and replaced with the base layer's
`base/.../vsdk/toolkit/environment/geometry/element/Intersection.h`, which is
**byte-identical to upstream VITRAL**. `Intersection` (public fields
`t/point/normal` + two constructors) is now a **shared class** between povCpp and
VITRAL, just as `Ray`/`RayWithSegments` already were.

### 1.3. The nearest-hit adapter exists: `doIntersectionFirstHit`

`src/environment/geometry/element/TransformableElement.cpp`:

```cpp
bool TransformableElement::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    auto *depthQueue = ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (allIntersections(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();   // the nearest (the queue is a min-heap by t)
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}
```

This is **conceptually the same** as VITRAL's `bool doIntersection(const Ray&,
RayHit*)`: "give me the nearest hit and tell me whether there was one". It
already carries VITRAL's destination name (`doIntersectionFirstHit`).

### 1.4. `PovRayHit` already exists as a mirror of VITRAL's `RayHit`

`src/environment/geometry/element/PovRayHit.h` is a type **exclusive to povCpp**
whose core is born with VITRAL's names (`p/n/t/u/v/hitDistance`) and groups the
POV extensions (`hitGeometry`, `material`, `objectTexture`, `objectColor`,
`noShadowFlag`). It is used as a **read-only projection** over
`IntersectionCandidate` at the shading boundary (`RayShaderPipeline`,
`LocalSurfaceShader`), without yet replacing the
`Intersection`+`IntersectionAttributes` storage.

---

## 2. `Ray`/`RayHit` (VITRAL) vs `Intersection`+`IntersectionAttributes` (povCpp)

### 2.1. VITRAL's model: "nearest-hit with lazy detail"

- `Ray` = `(origin, direction, t)`. The `t` travels in the ray: after
  intersecting, `inRay.withT(distance)` produces the "consumed" ray.
- `RayHit` = the **full result record**: `p` (point), `n` (normal), `t`
  (tangent), `u,v` (texture), `material`, `texture`, `normalMap`, plus
  `hitDistance`, the consumed `Ray`, and a **`requiredDetailMask`**
  (`DETAIL_POINT | DETAIL_NORMAL | DETAIL_UV | DETAIL_TANGENT`).
- The key trick: **lazy per-mask computation**. The raytracer decides *up front*
  what data it needs (`buildSurfaceDetailMask` in `SimpleRaytracer.cpp`), and the
  geometry computes only that (`doExtraInformation()`); for shadow rays
  `hitDistance` is enough.
- `doIntersection` is **overloaded**: `Ray* doIntersection(const Ray&)` and the
  canonical `bool doIntersection(const Ray&, RayHit*)`.

**VITRAL returns only the nearest hit.** It has no "all intersections" mechanism,
so it **does not support CSG** (booleans) nor the media stack for nested
refraction. Its `SimpleRaytracer` is, by design, simpler than povCpp's engine.

### 2.2. povCpp's model: "all intersections in a depth queue"

Inherited from classic POV-Ray, because it needs it for CSG and nested
refraction indices:

```cpp
virtual int allIntersections(RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue);
```

Each surface crossed by the ray is pushed into a priority queue ordered by `t`.
The unit travelling through that queue is:

```
IntersectionCandidate  =  Intersection  +  IntersectionAttributes
```

- **`Intersection`** (shared): the **purely geometric record** → `t`, `point`,
  `normal`. Equivalent to the *geometric core* of `RayHit`.
- **`IntersectionAttributes`**: the **shading attribution** → `hitGeometry`,
  `material`, `objectTexture`, `objectColor`, `noShadowFlag`. POV-specific
  information that VITRAL embeds inside `RayHit` (only `material`, `texture`,
  `normalMap`) or simply does not have.

They are kept as two separate sub-objects *"so Intersection stays a plain
t/point/normal record"*: the priority queue **compares only by the geometric
part** (`a.getIntersection().t < b.getIntersection().t`) and the attributes ride
along.

### 2.3. Correspondence table

| VITRAL `RayHit`            | povCpp                                          | Notes |
|----------------------------|-------------------------------------------------|-------|
| `p` (point)                | `Intersection::point` (`PovRayHit::p`)          | same concept |
| `n` (normal)               | `Intersection::normal` (`PovRayHit::n`)         | povCpp computes it once for the winner (`RenderEngine::trace`) |
| `t` (tangent)              | — (does not exist; `PovRayHit::t` reserved)     | povCpp's texturizers derive it if needed |
| `u,v`                      | — (not in `Intersection`)                       | povCpp computes UV inside the texture pipeline |
| `hitDistance` / `Ray.t`    | `Intersection::t` (`PovRayHit::hitDistance`)    | same concept |
| `material`                 | `IntersectionAttributes::material`              | POV `Material` ≠ VITRAL `SimpleMaterial` |
| `texture`                  | `IntersectionAttributes::objectTexture`         | |
| `normalMap`                | — (lives in `Material`)                          | |
| `requiredDetailMask`       | — (does not exist)                              | povCpp has no lazy per-mask computation |
| — (does not exist)         | `IntersectionAttributes::{hitGeometry,objectColor,noShadowFlag}` | POV/CSG-specific |

**Summary:** `Intersection` (shared) = **geometric half** of `RayHit`;
`IntersectionAttributes` = the shading attribution VITRAL embeds in `RayHit`.
They are split in povCpp so the CSG queue can order by `t` alone. `PovRayHit` is
the named mirror that joins both halves with VITRAL's names.

---

## 3. The central architectural tension

| Aspect               | VITRAL                              | povCpp |
|----------------------|-------------------------------------|--------|
| Geometric primitive  | `doIntersection(Ray&, RayHit*)`     | `allIntersections(Ray*, PriorityQueue*)` |
| Result               | nearest hit (1)                     | all hits (N), ordered |
| CSG / booleans       | not supported                       | supported (needs the N) |
| Nested refraction    | no                                  | yes (containing-media stack) |
| Surface detail       | lazy by mask                        | normal computed once for the winner; UV in the texturizer |
| Hit record           | `RayHit` (merged)                   | `Intersection` + `IntersectionAttributes` (projected into `PovRayHit`) |

**`allIntersections` cannot be replaced by nearest-hit `doIntersection`**: CSG
and refraction need the full set of surface crossings. Convergence is therefore
**layered and additive**: the powerful `allIntersections` primitive stays as
Level 1 and the `doIntersectionFirstHit` facade (Level 2) is built on top.

---

## 4. How povCpp does CSG today: point-membership classification

Conceptual reference for the superior algorithm:
`doc/references/[ROTH1982]_RayCastingForModelingSolids.pdf` (Scott D. Roth, *Ray
Casting for Modeling Solids*, 1982). The **plan** to adopt that algorithm lives
in a separate document (`/tmp/CSGByRaySegments.md`); here we only describe the
current state for contrast.

povCpp 1.0 implements CSG by **point-membership classification**, not by segments
(`CSG::allCsgIntersectIntersections`, `CSG.cpp`):

- It gathers **all** of each child's intersections into a queue.
- For **each** candidate, it walks **all the other children** and calls
  `doContainmentTest(point)` to decide whether the crossing survives.
- The **difference** is modeled by inverting the subtracted solid: `CsgParser`
  calls `invert()` on each non-first child of a `difference` block, and
  `invert()` flips `union↔intersection` and marks the children as `inverted`. It
  is not an interval subtraction.

Cost: for K children and M crossings, ~**O(K · M · K)** `doContainmentTest`
calls. Roth's scheme (In/Out segments + interval algebra) is a single
**O(M log M)** merge and, additionally:

- It is **numerically more robust**: the In/Out state is derived from the parity
  of ordered crossings, without depending on `doContainmentTest()` at points on
  the surface (where `SMALL_TOLERANCE` is fragile).
- It gives the containing-media stack for nested refraction **for free** (the
  segments *are* the "inside the solid" intervals): it connects directly to
  `containingTextures[]`/`containingIORs[]` of `RayWithSegments`.

**The hint in the name `RayWithSegments`:** the name of POV's `Ray` subclass
betrays that the design already pointed toward a segment model. Today that class
carries cached quadratic terms and the media stack, but **not** an explicit
In/Out segment list.

Note: the leaf solids (e.g. `Sphere::allIntersectionsForMaterial`) already emit
**both** crossings (entry `depth1`, exit `depth2`) to the queue; the pairs of
crossings already **define** implicit segments — they just need to be
materialized and combined by interval algebra instead of by point membership.

---

## 5. Additional meeting points identified (status)

| povCpp concept | VITRAL equivalent | Status |
|---|---|---|
| `RayWithSegments : Ray` | `Ray` (+ future segments) | ✅ shared base |
| `Intersection` | `Intersection` (identical) | ✅ shared (commit `9124240`) |
| `IntersectionCandidate` (Intersection + Attributes) | `RayHit` | mirrored via `PovRayHit` (read-only projection) |
| `TransformableElement::doIntersectionFirstHit` | `Geometry::doIntersection(Ray&,RayHit*)` | ✅ renamed to the destination name |
| `doContainmentTest(const Vector3Dd&, double)` | `doContainmentTest` | ✅ renamed (returns `int` `INSIDE/LIMIT/OUTSIDE`; does not yet adopt VITRAL's `Containment` enum nor the explicit tolerance) |
| `normal(result, point, config)` | `doExtraInformation(Ray,t,RayHit)` | partial: `doExtraInformation(RayWithSegments&, t, PovRayHit*)` exists and forwards to `normal()`; lazy per-mask computation missing |
| `PriorityQueuePool` | per-level `RayHit` *workspace* | ✅ same "no-alloc in hot path" pattern; correspondence note |
| `Statistics` (incrementRaySphereTests…) | `RaytraceStatistics` | counter names not yet unified |
| `RendererConfiguration` | `RendererConfiguration` | check whether they already match |

> **Correction of an old entry:** `getMinMax()` **does not exist** today as a
> general method of `TransformableElement`/`Geometry` in povCpp (only
> `HeightField::getBoundingBox()`/`findHfMinMax`, specific to that class). It is
> not "low-hanging fruit" because povCpp has no bounding-box concept at the
> `TransformableElement` level yet.

---

## 6. Convergence already achieved (factual summary)

1. **`Ray` reconciled**: `RayWithSegments : public Ray` (from VITRAL).
2. **`Intersection` shared** (commit `9124240`): same class from the base/VITRAL
   layer.
3. **`doIntersectionFirstHit`** = nearest-hit facade with VITRAL's destination
   name (`TransformableElement`), over the `allIntersections` primitive.
4. **`PovRayHit`** = named mirror of VITRAL's `RayHit` (core with VITRAL names +
   POV extensions), used as a projection at the shading boundary.
5. **`doContainmentTest`** = name aligned with VITRAL across the 27 classes that
   implement/call it (signature `Vector3Dd*`, returns `int` 0/1; the `Containment`
   tri-state and the explicit tolerance are still pending).
6. **`doExtraInformation`** = signature aligned with VITRAL (forwards to
   `normal()`; lazy detail-mask computation pending).
7. **Queue vocabulary** aligned: `PriorityQueuePool` = per-level
   `RayHit` *workspace*; `localDepthQueue` locals unified.
