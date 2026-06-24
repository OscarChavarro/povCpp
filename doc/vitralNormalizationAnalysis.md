# Comparative analysis of the intersection model: povCpp ↔ VITRAL

**Descriptive** document (not a plan). It compares how the `povCpp` raytracer
(POV-Ray 1.0, 1992, rewritten in C++) and the **VITRAL** library
(`/home/jedilink/VITRAL/vitral/cpp/base`) model ray-geometry intersection, so
that work on either side can be reasoned about against the other. VITRAL stays
frozen; povCpp is the side that moves toward it, gated by:

```
./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh   # => "Test passed."
```

and without losing performance.

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

This carries **exactly the same name** as VITRAL's canonical primitive
`bool Geometry::doIntersectionFirstHit(const Ray&, RayHit*)`: "give me the
nearest hit and tell me whether there was one". The name is fully reconciled
on both sides; what differs is the *role* — in VITRAL `doIntersectionFirstHit`
is the pure-virtual primitive each geometry implements directly, whereas in
povCpp it is a non-virtual facade built over the `allIntersections`
primitive (which VITRAL has no equivalent of).

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

**VITRAL returns only the nearest hit.** It has no "all intersections"
mechanism, so it **does not support CSG** (booleans) nor the media stack for
nested refraction. Its `SimpleRaytracer` is, by design, simpler than povCpp's
engine.

### 2.2. povCpp's model: "all intersections in a depth queue"

Inherited from classic POV-Ray, because it needs it for CSG and nested
refraction indices:

```cpp
virtual int allIntersections(RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue);
```

Each surface crossed by the ray is pushed into a priority queue ordered by
`t`. The unit travelling through that queue is:

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
They are split in povCpp so the CSG queue can order by `t` alone. `PovRayHit`
is the named mirror that joins both halves with VITRAL's names.

---

## 3. The central architectural tension

| Aspect               | VITRAL                              | povCpp |
|----------------------|--------------------------------------|--------|
| Geometric primitive  | `doIntersectionFirstHit(Ray&, RayHit*)` | `allIntersections(Ray*, PriorityQueue*)` |
| Result               | nearest hit (1)                     | all hits (N), ordered |
| CSG / booleans       | not supported                       | supported (needs the N) |
| Nested refraction    | no                                  | yes (containing-media stack) |
| Surface detail       | lazy by mask                        | normal computed once for the winner; UV in the texturizer |
| Hit record           | `RayHit` (merged)                   | `Intersection` + `IntersectionAttributes` (projected into `PovRayHit`) |

**`allIntersections` cannot be replaced by nearest-hit
`doIntersectionFirstHit`**: CSG and refraction need the full set of surface
crossings. povCpp's `doIntersectionFirstHit` (§1.3) sits on top of
`allIntersections` rather than replacing it.

---

## 4. CSG in povCpp: two interchangeable algorithms

`ConstructiveSolidGeometry` (`environment/geometry/volume/constructiveSolidGeometry/`)
is an abstract base (owns the child list and the `UNION`/`INTERSECTION`/
`DIFFERENCE` `geometryType`) with two concrete strategies. Which one is
instantiated is decided by `CsgParser::parse`, based on
`ParserContext::usesCsgRoth()` (the `-csgRoth` command-line flag):

### 4.1. `ConstructiveSolidGeometryByMorganRules` (default)

Point-membership classification, no `-csgRoth` flag needed:

- Gathers **all** of each child's intersections into a queue
  (`allCsgUnionIntersections`/`allCsgIntersectIntersections`).
- For **each** candidate, walks **all the other children** and calls
  `doContainmentTest(point, tolerance)` to decide whether the crossing
  survives (`insideCsgUnion`/`insideCsgIntersection`).
- `difference` has no dedicated `geometryType`: `CsgParser` builds an
  `INTERSECTION` container and calls `invert()` on each non-first child.
  `ConstructiveSolidGeometryByMorganRules::invertGeometry()` flips
  `UNION↔INTERSECTION` on itself and recursively calls `invert()` on every
  child.
- Cost: for K children and M crossings, ~O(K · M · K) `doContainmentTest`
  calls, and its correctness depends on `doContainmentTest` being reliable at
  points on the surface (where tolerance handling is fragile).

### 4.2. `ConstructiveSolidGeometryByRaySegment` (`-csgRoth`)

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
- The leaf solids (e.g. `Sphere::allIntersectionsForMaterial`) already emit
  both crossings (entry `depth1`, exit `depth2`); this path consumes them
  directly as segment endpoints instead of re-deriving "inside" via
  point-membership.
- Because segments *are* the "inside the solid" intervals, this path
  connects directly to `containingTextures[]`/`containingIORs[]` of
  `RayWithSegments` for nested refraction, and avoids `doContainmentTest`
  fragility at surface points.

Both strategies implement the same `ConstructiveSolidGeometry` interface
(`allIntersections`, `doContainmentTest`, `copy`, `invertGeometry`), so the
rest of the engine (CSG composition, transforms, the depth-queue consumers)
is agnostic to which one is active.

---

## 5. Correspondence table (current state)

| povCpp concept | VITRAL equivalent | Status |
|---|---|---|
| `RayWithSegments : Ray` | `Ray` (+ segments) | shared base |
| `Intersection` | `Intersection` (identical) | shared class |
| `IntersectionCandidate` (Intersection + Attributes) | `RayHit` | mirrored via `PovRayHit` (read-only projection) |
| `TransformableElement::doIntersectionFirstHit` | `Geometry::doIntersectionFirstHit(Ray&,RayHit*)` | **same name on both sides**; in VITRAL it is the pure-virtual primitive, in povCpp a non-virtual facade over `allIntersections` |
| `doContainmentTest(const Vector3Dd&, double)` | `Geometry::doContainmentTest(const Vector3Dd&, double)` | **signature identical**; both return `int` with the same constant names *and* values `INSIDE=1`/`LIMIT=0`/`OUTSIDE=-1`. The only difference: VITRAL backs them with `enum class Containment`, povCpp with bare `static constexpr int` on `TransformableElement`. Implemented by 17 classes (leaves, CSG, `Composite`, `BoundedGeometry`, `SimpleBody`, `LightGeometryAdapter`) |
| `doExtraInformation(RayWithSegments&, t, PovRayHit*)` | `doExtraInformation(Ray,t,RayHit)` | signature aligned; forwards to `normal()`; no lazy per-mask computation (no `requiredDetailMask`) |
| `PriorityQueuePool` | per-level `RayHit` workspace | same "no-alloc in hot path" pattern |
| `Statistics` | `RaytraceStatistics` | **not unified**: povCpp keeps per-instance counters keyed by primitive type (`raySphereTests`, `rayBoxTests`, …, each with a `*Succeeded` pair); VITRAL exposes static, per-ray-category recorders (`recordPrimaryRay`, `recordShadowRay`, `recordObjectIntersectionTest`, …). Different granularity and different ownership model — no 1:1 mapping to align toward |
| `RenderingConfiguration` (file `RendererConfiguration.h`) | `RendererConfiguration` | **not the same concept**: VITRAL's class configures *display* of a geometry in a GUI viewer (shading type, wireframe/points/normals visibility, bounding-volume color); povCpp's class configures the *render run itself* (I/O file names, antialiasing threshold, line range, thread count, CSG algorithm selection). Same-sounding name, unrelated responsibilities |
| — (no general bounding box) | `Geometry::getMinMax()` | **divergence**: VITRAL mandates `virtual double* getMinMax() = 0` on every geometry; povCpp has no such method on `TransformableElement`/`Geometry`, only `HeightField::getBoundingBox()`/`findHfMinMax`, specific to that class |

---

## 6. Open divergences

- **Lazy surface-detail mask**: VITRAL's `requiredDetailMask` lets the
  raytracer skip normal/UV/tangent computation when not needed (e.g. shadow
  rays only need `hitDistance`). povCpp always computes the normal for the
  winning hit in `doExtraInformation`/`normal()` and has no equivalent
  short-circuit.
- **`doContainmentTest` backing type**: largely converged — same signature,
  same `INSIDE=1`/`LIMIT=0`/`OUTSIDE=-1` constant names and values. The
  residual difference is only that VITRAL derives them from `enum class
  Containment` while povCpp declares bare `int` constants; povCpp does not
  return the enum type itself.
- **Statistics and `RendererConfiguration`**: as noted in §5, these are
  structurally and conceptually different between the two codebases; there
  is no pending rename or refactor that would unify them — they solve
  different problems.
- **No general bounding-box primitive**: VITRAL requires every geometry to
  implement `getMinMax()`; povCpp has no such concept at the
  `TransformableElement`/`Geometry` level (only `HeightField` has a local
  one), so CSG and other compound geometries cannot be bounded generically.

---

## 7. Lazy/selective work: VITRAL's detail mask vs povCpp's `+qN` quality

Both engines have a mechanism to *skip work that the final image does not
need*, but they express it very differently. This is the most promising place
to bring the two designs closer, and it has its own work document:
`doc/lazyQualitySelectionPlan.md`.

### 7.1. VITRAL: an orthogonal set of feature requirements

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

### 7.2. povCpp: one totally-ordered integer (`+qN`)

povCpp inherits classic POV-Ray's **single quality integer** `quality`
(`RenderingConfiguration::quality`, default `9`, set by `+qN` /
`CommandLineOptions`). There is no detail mask; the normal of the winning hit
is always computed (§6). Instead, scattered `getQuality()` comparisons across
the shading pipeline switch features on as the number rises. The actual
thresholds in the current code are:

| Feature gated | Condition | Where |
|---|---|---|
| Surface lit at all (normal + ambient + diffuse/specular) | `quality > 1` | `LocalSurfaceShader.cpp:36` |
| Shadow-ray object tests (cast shadows) | `quality > 3` | `DirectLightShader.cpp:63` |
| Full pigment/texture eval (else `quickColor`/`objectColor`/grey) | `quality > 5` | `RayShaderPipeline.cpp:72` |
| Transparent/coloured shadows (shadow ray not short-circuited) | `quality > 5` | `RayShaderPipeline.cpp:59` |
| Refraction / transmission | `quality > 5` | `RayShaderPipeline.cpp:124` |
| Bump-mapped normal (surface and refraction paths) | `quality >= 8` / `> 7` | `LocalSurfaceShader.cpp:46`, `RayShaderPipeline.cpp:131` |
| Mirror reflection | `quality >= 8` | `LocalSurfaceShader.cpp:67` |

So the ten levels collapse into a handful of **monotone bands** — each band is
the previous one plus one or two features:

- **q0–q1**: flat colour only (`quickColor`/`objectColor`/grey 0.5), no
  normal, no lighting, no shadows, no textures, no refraction, no reflection.
- **q2–q3**: + surface normal + ambient/diffuse/specular direct light, still
  no shadows, still `quickColor`.
- **q4–q5**: + cast shadows (opaque), still `quickColor`, no refraction.
- **q6–q7**: + full procedural/image textures + coloured/transparent shadows
  + refraction/transmission, no bump, no reflection.
- **q8–q9**: + bump mapping + mirror reflection (the full model).

This is exactly the "fast preview cheaper than the full model" lever: a low
`+qN` deliberately drops the expensive parts (texture evaluation, shadow rays,
refraction, reflection) to render a coarse-but-quick approximation. Measured
on `etc/level2/iortest.pov` at 320×200 (the new quality fixture, §7.3), the
output image is byte-identical *within* each band and only changes at the band
boundaries `q1→q2`, `q5→q6`, `q7→q8` — i.e. four distinct images across the
ten levels for that scene. (`q3→q4` adds shadows but they do not happen to
change *this* scene, which already illustrates the core weakness below.)

### 7.3. Why this is the convergence opportunity — and the mismatch

povCpp's `quality` integer and VITRAL's `requiredDetailMask` +
`RendererConfiguration` flags are **two encodings of the same idea**: *don't
compute what the output does not need*. The differences:

- **Total order vs orthogonal set.** `+qN` forces a fixed feature *staircase*:
  you cannot ask for "textures but no shadows" or "reflection without bump".
  VITRAL's booleans/mask can express any subset. The POV staircase is really
  a hand-picked *path* through VITRAL's flag space.
- **Conflation hides cost.** Because `quality` bundles unrelated features, a
  scene that has no shadows (like `iortest` above at `q4`) still pays the
  staircase's bookkeeping, and the user cannot isolate the one feature they
  want. A flag set makes each cost individually switchable and measurable.
- **No per-ray laziness.** povCpp's quality is a *global* setting for the
  whole render; VITRAL additionally varies the detail mask *per ray*
  (shadow vs primary). povCpp approximates this only with the ad-hoc
  `shadowRay && quality <= 5` short-circuit.
- **`RenderingConfiguration` would have to grow.** Today povCpp's
  configuration object is about I/O and the run (§5); it carries `quality` as
  a bare int. Reinterpreting `+qN` VITRAL-style means giving it real feature
  predicates (`withShadows`, `withTexture`, `withRefraction`, `withReflection`,
  `withBump`, a `shadingType`) that the shaders query instead of magic-number
  comparisons — moving povCpp's configuration model toward VITRAL's
  `RendererConfiguration` feature-flag shape.

The plan in `doc/lazyQualitySelectionPlan.md` proposes decomposing the
`quality` staircase into exactly such a flag set (kept gate-identical via the
new `scripts/testQualities.sh` golden test), so that `+qN` becomes a *named
preset* over orthogonal switches — the same switches VITRAL already exposes —
and so that povCpp can additionally drive a VITRAL-style per-ray lazy mask
from those switches.
