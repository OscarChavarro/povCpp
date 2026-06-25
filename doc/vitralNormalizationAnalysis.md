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
| `n` (normal)               | `Intersection::normal` (`PovRayHit::n`)         | povCpp computes it once for the winner in `RenderEngine::trace`, and only when the ray's `requiredDetailMask` asks for it (`needsNormal()`) |
| `t` (tangent)              | — (does not exist; `PovRayHit::t` reserved)     | povCpp's texturizers derive it if needed |
| `u,v`                      | — (not in `Intersection`)                       | povCpp computes UV inside the texture pipeline |
| `hitDistance` / `Ray.t`    | `Intersection::t` (`PovRayHit::hitDistance`)    | same concept |
| `material`                 | `IntersectionAttributes::material`              | POV `Material` ≠ VITRAL `SimpleMaterial` |
| `texture`                  | `IntersectionAttributes::objectTexture`         | |
| `normalMap`                | — (lives in `Material`)                          | |
| `requiredDetailMask` (on `RayHit`) | `RayWithSegments::requiredDetailMask` | shared concept, same `DETAIL_NONE/POINT/NORMAL/UV/TANGENT/ALL` constants; povCpp keeps it on the *ray* (not the hit), because the decision is made before the hit exists, and gates only `NORMAL` (see §7) |
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
| Surface detail       | lazy by mask (point/normal/UV/tangent) | lazy by mask (normal only, all-or-nothing); UV in the texturizer |
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
| `doExtraInformation(RayWithSegments&, t, PovRayHit*)` | `doExtraInformation(Ray,t,RayHit)` | signature aligned; forwards to `normal()`. Lazy: `RenderEngine::trace` calls it only when the ray's `requiredDetailMask` has `DETAIL_NORMAL` (`needsNormal()`), matching VITRAL's mask-gated `doExtraInformation` |
| `PriorityQueuePool` | per-level `RayHit` workspace | same "no-alloc in hot path" pattern |
| `Statistics` | `RaytraceStatistics` | **not unified**: povCpp keeps per-instance counters keyed by primitive type (`raySphereTests`, `rayBoxTests`, …, each with a `*Succeeded` pair); VITRAL exposes static, per-ray-category recorders (`recordPrimaryRay`, `recordShadowRay`, `recordObjectIntersectionTest`, …). Different granularity and different ownership model — no 1:1 mapping to align toward |
| `RenderingConfiguration` (file `RendererConfiguration.h`) | `RendererConfiguration` | **shared feature-flag vocabulary, different scope**: both expose `withSurfaceLighting/withShadows/withTextures/withFilteredShadows/withRefraction/withBumpMapping/withReflection` predicates and a `shadingType` using the same `SHADING_TYPE_*` values; but VITRAL's class is GUI display config (wireframe/points/normals visibility, bounding-volume color) while povCpp's also carries render-run config (I/O file names, antialiasing, line range, threads, CSG algorithm). See §7 |
| — (no general bounding box) | `Geometry::getMinMax()` | **divergence**: VITRAL mandates `virtual double* getMinMax() = 0` on every geometry; povCpp has no such method on `TransformableElement`/`Geometry`, only `HeightField::getBoundingBox()`/`findHfMinMax`, specific to that class |
| `CameraSnapshot` in `Scene` + parser-local `PovCameraSpec` | `Camera` + `CameraSnapshot` (`vsdk/.../environment/camera/`) | **render-time storage unified**: povCpp now renders from VITRAL's `CameraSnapshot`, while the POV parser keeps a local five-vector `PovCameraSpec` so `look_at` and post-declaration transforms stay byte-identical. `RenderEngine::createRay` still uses povCpp's legacy sampling/normalization, but it now reads `eyePosition`/`dir`/`upWithScale`/`rightWithScale` directly from the snapshot. See §8 and `doc/cameraPlan.md` |

---

## 6. Open divergences

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
- **`RendererConfiguration` scope**: the two classes share the feature-flag
  vocabulary (§5, §7) but bundle different surrounding responsibilities (GUI
  display config vs render-run/IO config), so they remain distinct objects
  rather than one shared type.
- **No general bounding-box primitive**: VITRAL requires every geometry to
  implement `getMinMax()`; povCpp has no such concept at the
  `TransformableElement`/`Geometry` level (only `HeightField` has a local
  one), so CSG and other compound geometries cannot be bounded generically.
- **Camera generator still differs**: povCpp now stores the render-time camera
  as VITRAL `CameraSnapshot`, fed from a parser-local `PovCameraSpec` that
  preserves POV's five raw vectors. The remaining divergence is in sampling and
  normalization only: VITRAL samples pixel **centres** with `+0.5` and leaves
  the direction unnormalised, while povCpp samples integer coordinates and then
  applies `normalizedFast()`. §8 and `doc/cameraPlan.md` describe that
  remaining pixel-changing step.

---

## 7. Lazy/selective work: VITRAL's detail mask vs povCpp's `+qN` quality

Both engines skip work that the final image does not need, using the same two
mechanisms: an orthogonal feature-flag set on the configuration object and a
per-ray surface-detail mask. In povCpp `+qN` is a *named preset* over those
flags. This section describes how the two map onto each other and where they
still differ.

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

### 7.2. povCpp: an orthogonal flag set with `+qN` as a preset

The shading pipeline queries **feature predicates** on
`RenderingConfiguration`, each backed by a bit in the `options` bitmask:

| Feature | Predicate (queried by the shaders) | Where read |
|---|---|---|
| Surface lit at all (normal + ambient + diffuse/specular) | `withSurfaceLighting()` | `LocalSurfaceShader.cpp:36` |
| Shadow-ray object tests (cast shadows) | `withShadows()` | `DirectLightShader.cpp:67` |
| Full pigment/texture eval (else `quickColor`/`objectColor`/grey) | `withTextures()` | `RayShaderPipeline.cpp:72` |
| Transparent/coloured shadows (shadow ray not short-circuited) | `withFilteredShadows()` | `RayShaderPipeline.cpp:59` |
| Refraction / transmission | `withRefraction()` | `RayShaderPipeline.cpp:124` |
| Bump-mapped normal (surface and refraction paths) | `withBumpMapping()` | `LocalSurfaceShader.cpp:46`, `RayShaderPipeline.cpp:131` |
| Mirror reflection | `withReflection()` | `LocalSurfaceShader.cpp:67` |
| Shading model (derived, read-only) | `getShadingType()` → `SHADING_TYPE_NOLIGHT`/`PHONG` | — |

`+qN` is **a preset over these flags**, exclusive to the command-line layer:

- `setQuality(N)` maps the integer band onto the seven bits to reproduce
  classic POV-Ray's quality bands bit-for-bit (q0-1: nothing; q2-3: +lighting;
  q4-5: +shadows; q6-7: +textures/filtered-shadows/refraction; q8-9:
  +bump/reflection). The flags are the only stored state; no `quality` integer
  is retained, since nothing reads one back.
- `setQuality()` is **exclusive to the command-line layer**: its only caller is
  `CommandLineOptions::parseOption` (the `+qN` switch). The default-construction
  path does **not** route through it — `RenderingConfiguration::reset()` sets
  the seven feature bits on directly (full quality, identical to the `setQuality(9)`
  bit pattern) rather than invoking the `+qN` preset, so the band-preset code
  never executes outside the command line. `+qflags<letters>` likewise toggles
  individual bits directly (`L`/`S`/`T`/`F`/`R`/`B`/`M`), so any subset is
  reachable from the command line (e.g. `+q9 -qflagsS` = full minus shadows),
  not only the band presets.

The configuration object is **owned by `PovRayApplication`** (a value member,
`PovRayApplication::configuration`) and propagated by reference from
construction: it is handed to `RenderContext` (`const RenderingConfiguration&`),
reaches `RenderEngine` through the context, and each `RayWithSegments` carries
a `const RenderingConfiguration*` (`setConfig`/`getConfig`) so the shaders read
the same instance. No global, no per-shader copy, no re-derivation.

The **per-ray detail mask** is the second mechanism (§6): `RenderEngine::trace`
sets the primary ray's `requiredDetailMask` to `DETAIL_ALL` when
`withSurfaceLighting()` and `DETAIL_NONE` otherwise, and skips the winner's
normal computation (`doExtraInformation`) when the mask does not request it;
`DirectLightShader` marks shadow rays `DETAIL_NONE`. This is the "fast preview
cheaper than the full model" lever, expressed exactly as VITRAL expresses it.

### 7.3. Shared structure and residual differences

povCpp's selectivity and VITRAL's `requiredDetailMask` +
`RendererConfiguration` flags are **the same two encodings of the same idea** —
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
  do not touch the `allIntersections` + CSG + containing-media machinery that
  povCpp needs and VITRAL's nearest-hit `SimpleRaytracer` lacks (§3). The
  shared part is the selection vocabulary, not the traversal.
- **Mask granularity.** povCpp gates `NORMAL` all-or-nothing; VITRAL gates
  `POINT`/`NORMAL`/`UV`/`TANGENT` independently. povCpp computes UV inside the
  texture pipeline, not on request through the mask.
- **`shadingType` is derived, read-only.** povCpp implements no `FLAT`,
  `GOURAUD` or `COOK_TERRANCE` shader, so `getShadingType()` only ever returns
  `NOLIGHT` or `PHONG`; it is a view over `withSurfaceLighting()`, not settable
  state, to avoid claiming support that does not exist.
- **`RenderingConfiguration` scope.** The flag vocabulary is shared, but the
  class also holds render-run/IO config that VITRAL's display-oriented
  `RendererConfiguration` does not (§5) — they remain distinct objects.

---

## 8. The camera: `PovCameraSpec`/`CameraSnapshot` (povCpp) vs `Camera`/`CameraSnapshot` (VITRAL)

This is a **descriptive** snapshot of where the two camera models stand today.
The render-time migration described in `doc/cameraPlan.md` is now implemented:
povCpp renders from VITRAL `CameraSnapshot`, while the parser keeps a local
`PovCameraSpec` to preserve POV's mutable five-vector grammar.

### 8.1. povCpp's parse-time camera: POV-Ray's five raw vectors

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

### 8.2. VITRAL's camera: orthonormal frame + FOV, baked into a snapshot

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

### 8.3. The 1:1 mapping, and the two reasons the generators are not yet byte-identical

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
when fed the same vectors and the same `u/v`. That data convergence is now in
place through `CameraSnapshot`. Only two deltas remain, and both live in the
*generator*, not the data:

1. **Pixel sampling.** VITRAL samples pixel **centres** (`x+0.5`, `vpY-(y+0.5)`);
   povCpp samples integer coordinates with a `screenHeight-1` offset. The two
   `u/v` differ by half a pixel on each axis.
2. **Normalization.** povCpp normalises the direction with POV's `normalizedFast()`
   (fast inverse-sqrt approximation, *not* scale-invariant at the bit level);
   VITRAL returns the direction unnormalised.

So the storage is already reconcilable to VITRAL's `CameraSnapshot` with no
arithmetic change, while the *sampling/normalization* differences are what a
byte-identical migration must keep on povCpp's side (or defer, with golden
regeneration, to a later pixel-changing step). `doc/cameraPlan.md` turns this
into a staged plan.
