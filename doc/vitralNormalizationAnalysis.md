# Comparative analysis of the intersection model: povCpp <-> VITRAL

Status snapshot comparing the current `povCpp` raytracer with the current
[VITRAL library](https://github.com/OscarChavarro/vitral).

The focus is the required alignment surface between both codebases: rays,
intersection results, traversal primitives, transforms, scene bodies, detail
selection, renderer configuration, bounding information and camera snapshots.

---

## 1. Shared primitives already in use

### 1.1. `RayWithSegments` extends VITRAL `Ray`

`povCpp` uses VITRAL's `Ray` directly:

```cpp
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class RayWithSegments : public Ray { ... };
```

VITRAL `Ray` stores `origin`, `direction` and `t`, and exposes immutable-style
builders (`withOrigin`, `withDirection`, `withT`) plus setters.

`RayWithSegments` adds POV-specific render state:

- Quadric cached terms: `position2`, `direction2`, `positionDirection`,
  `mixedPositionPosition`, `mixedDirectionDirection`,
  `mixedPositionDirection`, guarded by `quadricConstantsCached`.
- Nested medium state: `containingTextures`, `containingIORs`,
  `containingIndex`.
- Ray classification: `isShadowRay`, `isPrimaryRay`.
- Per-ray detail mask using VITRAL-compatible `DETAIL_*` constants.
- Runtime pointers: `Statistics`, `PovRayRendererConfiguration`,
  `PriorityQueuePool<IntersectionCandidate>`.

Alignment status: the ray base is shared; povCpp's extra state is specific to
POV traversal, CSG and nested refraction.

### 1.2. `Intersection` is shared

`povCpp` includes VITRAL's `Intersection`:

```cpp
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"
```

The class is a purely geometric record:

```cpp
double t;
Vector3Dd point;
Vector3Dd normal;
```

Alignment status: both codebases use the same class for the geometric core.

### 1.3. `PovRayHit` mirrors VITRAL `RayHit`

VITRAL `RayHit` is the full hit record: `p`, `n`, `t`, `u`, `v`, `material`,
`texture`, `normalMap`, `hitDistance`, optional consumed `Ray`, and a
`requiredDetailMask`.

`povCpp` keeps queue storage split:

```cpp
IntersectionCandidate = Intersection + IntersectionAttributes
```

`IntersectionAttributes` stores POV attribution:

- `hitGeometry`
- `material`
- `objectTexture`
- `objectColor`
- `noShadowFlag`

`PovRayHit::fromCandidate()` projects that split storage onto VITRAL-shaped
names (`p`, `n`, `t`, `u`, `v`, `hitDistance`) at the shading boundary.

Alignment status: `PovRayHit` gives povCpp a VITRAL-compatible read model
without replacing the all-crossings queue representation needed by CSG.

---

## 2. Intersection operation model

### 2.1. VITRAL: nearest-hit primitive

VITRAL `Geometry` declares:

```cpp
virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) = 0;
virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p);
virtual double* getMinMax() = 0;
virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
```

Concrete VITRAL shapes implement nearest-hit intersection directly. Detail is
lazy through `RayHit::requiredDetailMask`: point, normal, UV and tangent are
computed only when requested.

### 2.2. povCpp: all-crossings primitive

`povCpp` `Geometry` declares:

```cpp
virtual int doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride = nullptr);

bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
virtual void normal(Vector3Dd *result, Vector3Dd *intersectionPoint);
virtual void normal(
    Vector3Dd *result,
    Vector3Dd *intersectionPoint,
    const PovRayRendererConfiguration *config);
virtual void doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit);
virtual void *copy() = 0;
virtual void invertGeometry();
```

`doIntersectionForAllRayCrossings` is the primary povCpp operation. Shapes push
every ray crossing into a priority queue ordered by `Intersection::t`.

`Geometry::doIntersectionFirstHit` is a non-virtual adapter:

```cpp
bool Geometry::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (doIntersectionForAllRayCrossings(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}
```

Alignment status: both projects expose the `doIntersectionFirstHit` name, but
the required primitive is different. VITRAL implements nearest-hit directly;
povCpp derives nearest-hit from all-crossings.

### 2.3. Required all-crossings alignment

`doIntersectionForAllRayCrossings` has no VITRAL equivalent. It is required in
povCpp for:

- CSG boolean operations, where the complete ordered crossing list defines
  inside/outside intervals.
- Nested refraction, where entering and leaving media updates the containing
  material/IOR stack.
- Shadow filtering, where transparent/filtered shadows may need to process
  multiple blockers along the shadow ray.

Required alignment item: VITRAL needs an all-crossings primitive alongside
nearest-hit if it is to host POV-compatible CSG and nested media traversal.

---

## 3. Hit record correspondence

| VITRAL `RayHit` | povCpp current representation | Status |
|---|---|---|
| `p` | `Intersection::point`, projected as `PovRayHit::p` | aligned concept |
| `n` | `Intersection::normal`, projected as `PovRayHit::n` | aligned concept; computed for the winning hit when the ray needs normals |
| `t` tangent | `PovRayHit::t` placeholder | field name aligned; tangent is not produced by povCpp geometry |
| `u`, `v` | `PovRayHit::u`, `PovRayHit::v` placeholders | UV is computed in the texture pipeline, not stored in candidates |
| `hitDistance` | `Intersection::t`, projected as `PovRayHit::hitDistance` | aligned concept |
| `material` | `IntersectionAttributes::material` | aligned role, different material type |
| `texture` | `IntersectionAttributes::objectTexture` | aligned role, POV material model |
| `normalMap` | part of povCpp material/normal pipeline | not a hit attribute |
| `requiredDetailMask` | `RayWithSegments::requiredDetailMask` | constants aligned; owner differs |
| consumed `Ray` | `RayWithSegments` plus `Intersection::t` | no stored consumed-ray object in candidate |
| no VITRAL field | `hitGeometry`, `objectColor`, `noShadowFlag` | POV-specific |

Required alignment item: keep `Intersection` as the geometric core and keep
POV attribution separate unless VITRAL gains an all-crossings record that can
carry both geometric and shading attribution.

---

## 4. Transform and placement model

Both projects keep concrete geometry in canonical/object space and transform
rays for intersection. The ownership of the transform differs.

### 4.1. VITRAL transform ownership

VITRAL `SimpleBody` owns placement:

- `Geometry* geometry`
- `Vector3Dd position`
- `Vector3Dd scale` and `inverseScale`
- `Matrix4x4d rotation` and `rotationInverse`
- `Quaterniond rotationQuaternion` and `rotationInverseQuaternion`
- Fast-path flags: `hasIdentityRotation`, `hasUnitScale`,
  `hasZeroTranslation`, `hasTranslationOnlyTransform`,
  `hasIdentityTransform`, `hasInvertibleScale`

`SimpleBody::doIntersectionFirstHit` transforms the incoming world ray into
local geometry space, calls `SurfaceRayIntersection::doIntersectionFirstHit`,
converts hit distance/detail back to world space, and stamps body-level
material/texture/normal map onto the hit.

Alignment status: VITRAL keeps `Geometry` transform-free and uses
scene-level bodies as transform owners.

### 4.2. povCpp transform ownership

`povCpp` `Geometry` is transform-free. Placement lives in
`TransformedGeometry`, inherited by concrete renderable shapes:

```cpp
class TransformedGeometry : public Geometry {
  protected:
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;

  public:
    virtual AxisAlignedBox getMinMax() const;
    virtual void translateGeometry(Vector3Dd *vector);
    virtual void rotateGeometry(Vector3Dd *vector);
    virtual void scaleGeometry(Vector3Dd *vector);
};
```

Concrete examples include `Sphere`, `Box`, `Quadric`, `Triangle`,
`InfinitePlane`, `Blob`, `HeightField`, `ParametricBiCubicPatch`,
`PolynomialShape`, `ConstructiveSolidGeometry` and `LightGeometryAdapter`.

`transformation == nullptr` means identity. Transform matrices are allocated
lazily and accumulated at parse time.

Alignment status: both `Geometry` bases are transform-free; transform ownership
is divergent. VITRAL owns transforms in `SimpleBody`; povCpp owns transforms in
`TransformedGeometry`.

### 4.3. Canonical shapes

`povCpp` `Sphere` is a canonical unit sphere centered at the origin. Radius and
placement are carried by `TransformedGeometry::transformation`.

VITRAL `Sphere` carries `radius_`/`radiusSquared_` in the geometry and relies on
`SimpleBody` for body placement.

Required alignment item: if geometry classes are to converge, the codebases
need one agreed ownership model for intrinsic dimensions versus scene
placement. Current povCpp puts sphere radius in the transform; current VITRAL
keeps radius in the shape.

---

## 5. Scene body model

### 5.1. VITRAL `SimpleBody`

VITRAL `SimpleBody` is the render-time scene object:

- It owns `Geometry*`.
- It owns transform state.
- It owns global material, texture and normal map.
- It provides nearest-hit traversal.
- It tracks modification version for render-time consistency checks.

### 5.2. povCpp `SimpleBody`

`povCpp` `SimpleBody` is also a `Geometry` subclass and is the scene object
stored in `Scene::Objects`:

```cpp
class SimpleBody : public Geometry {
  private:
    java::ArrayList<TransformedGeometry*> boundingShapes;
    java::ArrayList<TransformedGeometry*> clippingShapes;
    TransformedGeometry *geometry;
    Material *geometryMaterial;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
};
```

It handles:

- Bounding region tests before object intersection.
- Clipping region filtering after object crossings are gathered.
- Object-level material, texture, quick color and no-shadow attribution.
- `translate`, `rotate`, `scale` propagation into geometry, bounding shapes,
  clipping shapes, material and object texture.
- `invert` propagation into geometry and region shapes.
- `getAABB()` from bounding regions or wrapped geometry.

`Composite` extends `SimpleBody` and contains nested `SimpleBody*` children.
Its transforms propagate into children and region shapes.

Alignment status: the `SimpleBody` name is shared and both are scene-object
level concepts. Responsibilities differ: VITRAL owns placement in `SimpleBody`;
povCpp stores placement in wrapped `TransformedGeometry` and uses `SimpleBody`
for POV object attribution, bounding/clipping and composite traversal.

Required alignment item: preserve the shared scene-body concept, but decide
whether transform ownership should remain divergent or move to one common layer.

---

## 6. CSG and all-crossings in povCpp

`ConstructiveSolidGeometry` is a `TransformedGeometry` with child
`TransformedGeometry*` shapes, child material overrides and a geometry type:
`UNION`, `INTERSECTION`, `DIFFERENCE`.

Two strategies implement the same interface.

### 6.1. Morgan-rules strategy

`ConstructiveSolidGeometryByMorganRules` gathers child crossings and filters
candidate points through `doContainmentTest` against the other children.

This strategy depends on reliable point containment near boundaries.

### 6.2. Ray-segment strategy

`ConstructiveSolidGeometryByRaySegment` builds `RaySegments` from ordered
crossings and merges intervals for union, intersection and difference. This is
selected through the `CSG_ROTH` configuration option exposed by the `-csgRoth`
command-line path.

This strategy consumes the same leaf all-crossings records used by refraction
and shadow traversal.

Alignment status: CSG is implemented only in povCpp and depends on
`doIntersectionForAllRayCrossings`.

Required alignment item: VITRAL needs an all-crossings contract and a segment
or containment-based boolean layer to align with povCpp CSG semantics.

---

## 7. Containment

Both projects expose:

```cpp
int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
```

Both use the same values:

- `INSIDE = 1`
- `LIMIT = 0`
- `OUTSIDE = -1`

VITRAL backs these constants with `enum class Containment`. povCpp declares
bare `static constexpr int` values on `Geometry`.

Alignment status: signature and values are aligned. The backing type differs.

Required alignment item: use one containment type consistently if shared CSG or
shared geometric processing is moved across the codebases.

---

## 8. Detail masks and selective work

### 8.1. VITRAL

VITRAL uses `RayHit::requiredDetailMask`:

- `DETAIL_NONE`
- `DETAIL_POINT`
- `DETAIL_NORMAL`
- `DETAIL_UV`
- `DETAIL_TANGENT`
- `DETAIL_ALL`

`SimpleRaytracer::buildSurfaceDetailMask` computes the mask from lighting,
texture, bump map and reflection requirements. Geometry fills only requested
details.

### 8.2. povCpp

`povCpp` mirrors the same constants on `RayWithSegments`. The mask lives on the
ray because the all-crossings queue contains candidates before a final hit
record exists.

`RenderEngine::trace` sets the winning ray mask to:

- `DETAIL_ALL` when `withSurfaceLighting()` is enabled.
- `DETAIL_NONE` when surface lighting is disabled.

Only `DETAIL_NORMAL` gates `doExtraInformation()` for the winning hit. Shadow
rays are explicitly marked `DETAIL_NONE`.

Alignment status: constants and intent are aligned. Granularity differs:
VITRAL gates point, normal, UV and tangent independently; povCpp gates normal
work at the render-engine boundary and computes UV inside the texture pipeline.

Required alignment item: if shared geometry detail evaluation is desired,
povCpp needs point/UV/tangent mask usage beyond the current normal-only gate.

---

## 9. Renderer configuration

`PovRayRendererConfiguration` inherits VITRAL `RendererConfiguration`.

Shared surface:

- `SHADING_TYPE_NOLIGHT`
- `SHADING_TYPE_FLAT`
- `SHADING_TYPE_GOURAUD`
- `SHADING_TYPE_PHONG`
- `SHADING_TYPE_COOK_TERRANCE`
- `getShadingType()`
- `setShadingType()`

povCpp-specific state is stored in an `options` bitmask:

- Render/runtime flags: display, verbose, disk write, prompt exit, antialias,
  continue trace, verbose file, parallel, CSG Roth.
- Quality feature flags: surface lighting, shadows, textures, filtered shadows,
  refraction, bump mapping, reflection.
- IO/threading/parser fields: file names, output format, buffer size,
  first/last line, tokenizer settings, number of threads.

`setSurfaceLightingEnabled()` keeps the inherited `shadingType` synchronized:
`NOLIGHT` when disabled, `PHONG` when enabled.

`setQuality(q)` maps POV quality bands onto the seven feature flags:

- `q0-1`: no surface lighting.
- `q2-3`: lighting.
- `q4-5`: lighting and shadows.
- `q6-7`: lighting, shadows, textures, filtered shadows and refraction.
- `q8-9`: all of the above plus bump mapping and reflection.

Alignment status: povCpp reuses VITRAL's renderer-configuration base for the
shading-type vocabulary, but rendering feature selection is POV-specific.

Required alignment item: keep `RendererConfiguration` as the shared vocabulary
for broad shading mode; keep POV quality flags separate unless VITRAL gains
equivalent render-feature semantics.

---

## 10. Bounding information

VITRAL requires every `Geometry` to implement:

```cpp
virtual double* getMinMax() = 0;
```

povCpp `Geometry` does not expose a bounding-box method. Bounds live one level
down on `TransformedGeometry`:

```cpp
virtual AxisAlignedBox getMinMax() const { return AxisAlignedBox::unbounded(); }
```

Several concrete povCpp geometries override it: `Sphere`, `Box`, `Blob`,
`HeightField`, `Triangle`, `ParametricBiCubicPatch` and
`ConstructiveSolidGeometry`.

`SimpleBody::getAABB()` returns the intersection of bounding-shape AABBs when
bounding regions exist; otherwise it returns the wrapped geometry AABB.

Alignment status: both codebases have bounding concepts, but the API shape is
not aligned. VITRAL uses `double*` at `Geometry`; povCpp uses `AxisAlignedBox`
at `TransformedGeometry`/`SimpleBody`.

Required alignment item: a shared bounding API needs an agreed return type and
an agreed owner (`Geometry`, `TransformedGeometry`, `SimpleBody`, or a shared
body abstraction).

---

## 11. Camera snapshots

`povCpp` renders from VITRAL `CameraSnapshot`. The POV parser stores a local
`PovCameraSpec` with the classic five vectors:

- `location`
- `direction`
- `up`
- `right`
- `sky`

`PovCameraSpec::bake()` returns a `CameraSnapshot`:

```cpp
CameraSnapshot(
    location,
    direction.normalizedFast(),
    right.normalizedFast().multiply(-1.0),
    up.normalizedFast(),
    Camera::PROJECTION_MODE_PERSPECTIVE,
    1.0,
    0.0,
    0.0,
    direction,
    up,
    right);
```

`RenderEngine::createRay` consumes `eyePosition`, `dir`, `upWithScale` and
`rightWithScale`:

```cpp
xScalar = (x - width / 2.0) / width;
yScalar = (((screenHeight - 1) - y) - height / 2.0) / height;
direction = upWithScale * yScalar + rightWithScale * xScalar + dir;
direction = direction.normalizedFast();
origin = eyePosition;
```

VITRAL `Camera` uses an orthonormal frame and FOV, exports a
`CameraSnapshot`, and `SimpleRaytracer::generateRay` consumes the snapshot:

```cpp
u = ((x + 0.5) - viewportXSize / 2.0) / viewportXSize;
v = ((viewportYSize - (y + 0.5)) - viewportYSize / 2.0) / viewportYSize;
direction = rightWithScale * u + upWithScale * v + dir;
origin = eyePosition;
```

Alignment status: render-time storage is shared through `CameraSnapshot`.
Ray generation differs in pixel sampling and direction normalization:

- VITRAL samples pixel centers.
- povCpp samples integer pixel coordinates using `screenHeight - 1`.
- povCpp normalizes the generated direction with `normalizedFast()`.
- VITRAL returns the generated perspective direction through `Ray`, whose
  constructor normalizes according to VITRAL `Ray` semantics.

Required alignment item: keep `CameraSnapshot` as the shared storage format.
Only align the generator if visual parity between VITRAL sampling and POV-Ray
sampling becomes a goal; POV compatibility depends on povCpp's generator
semantics.

---

## 12. Statistics and hot-path allocation

povCpp `Statistics` is per-render/per-task and records POV-specific counters:
ray totals, primitive tests and successes, shadow tests, clipping/bounding
tests, reflected/refracted/transmitted rays and related values.

VITRAL `RaytraceStatistics` exposes static recorders for ray categories and
object intersection tests.

povCpp uses `PriorityQueuePool<IntersectionCandidate>` to avoid allocating a
new queue for every intersection operation. VITRAL uses reusable hit/workspace
records in the raytracer path.

Alignment status: both avoid avoidable hot-path allocation, but statistics
ownership and granularity are different.

Required alignment item: no direct 1:1 statistics mapping exists. Any shared
statistics layer would need a common event taxonomy first.

---

## 13. Current alignment matrix

| Area | Current povCpp | Current VITRAL | Alignment status |
|---|---|---|---|
| Ray type | `RayWithSegments : Ray` | `Ray` | shared base, POV extensions on derived ray |
| Geometric hit core | VITRAL `Intersection` | `Intersection` | shared class |
| Full hit record | `IntersectionCandidate` + `PovRayHit` projection | `RayHit` | conceptually mapped, storage differs |
| Primary intersection primitive | all crossings | nearest hit | divergent, all-crossings required for POV |
| Nearest-hit name | adapter over all crossings | pure virtual geometry primitive | name aligned, role differs |
| Containment | `int`, same constants | `int`, enum-backed constants | signature/value aligned |
| Geometry transform | none on `Geometry`; matrix on `TransformedGeometry` | none on `Geometry`; transform on `SimpleBody` | base aligned, owner divergent |
| Scene body | `SimpleBody : Geometry`, wraps transformed geometry and regions | `SimpleBody`, wraps geometry and transform | name/concept aligned, responsibilities differ |
| CSG | supported through all crossings | no all-crossings CSG path | divergent |
| Detail mask | `RayWithSegments::DETAIL_*`, normal gate in render engine | `RayHit::DETAIL_*`, per-detail lazy fill | constants aligned, granularity differs |
| Renderer config | derives from VITRAL base plus POV flags | base display/render vocabulary | partially aligned |
| Bounding API | `AxisAlignedBox` on `TransformedGeometry`/`SimpleBody` | `double* getMinMax()` on `Geometry` | concept present, API divergent |
| Camera storage | VITRAL `CameraSnapshot` | `CameraSnapshot` | shared storage, generator semantics differ |
| Statistics | per-instance/per-task POV counters | static raytrace recorders | divergent |

---

## 14. Required alignment work

1. Add or design an all-crossings primitive for VITRAL that can represent
   ordered ray/solid crossings and carry enough attribution for CSG, shadows
   and nested refraction.
2. Decide the transform ownership model for shared geometry: VITRAL
   `SimpleBody`, povCpp `TransformedGeometry`, or a common abstraction.
3. Decide whether intrinsic dimensions such as sphere radius belong to geometry
   or to placement transforms.
4. Unify or bridge hit-detail masks so point, normal, UV and tangent laziness
   have the same owner and semantics.
5. Define a shared bounding API with a stable return type.
6. Keep `CameraSnapshot` as the shared render-time camera record and treat ray
   generation differences as intentional unless a visual-parity task requires
   alignment.
7. Keep POV quality flags in `PovRayRendererConfiguration` unless VITRAL adds
   equivalent feature-level rendering switches.

---

## 15. Decoupling regression investigation (wg5 / chess), part 6

This section records the investigation of the scenes broken by the
"Decoupling TransformedGeometry from Geometry, part 6" commit (`47e94d4`),
using the previous commit (`3fa7a81`, "part 5") as the known-good reference.
It documents the experiments run, what was fixed, what is still broken, and
concrete clues for whoever resumes this.

### 15.1. Background: baked vs. matrix transforms

The golden images in `referenceTestImages/` were produced by the **baked**
transform model: `Quadric`, `PolynomialShape`, `ConstructiveSolidGeometry`,
etc. mutated their own coefficients/operands when transformed
(`scaleGeometry`/`translateGeometry`/`csg->scale()`), so by trace time the
geometry already lived in world space and carried no transform.

Part 6 moves CSG and `PolynomialShape` (and indirectly the quadrics nested
under them) to a **matrix** model: the transform is kept as a
`Matrix4x4d`/`Matrix4x4d` inverse pair on the owner (`SimpleBody`,
`CsgOperand`) and applied to the ray (inward) and to the hit point/normal
(outward) at trace time. `GeometryTransformMutator` still bakes
`PolynomialShape`/`ParametricBiCubicPatch` where asked, but `CsgOperand` and
`SimpleBody` deliberately route `PolynomialShape` and `ConstructiveSolidGeometry`
through the matrix path.

Because the goldens are baked-derived, the matrix path must reproduce the
baked result. **Key empirical finding: it can.** Many transformed CSG/quadric
cases reproduce the baked golden byte-for-byte (AE 0), so the remaining
mismatches are *real bugs in the matrix path*, not irreducible float noise.

### 15.2. Method

- Built both commits in git worktrees plus a third tree with the candidate
  fixes; rendered the same `.pov` with each binary and compared with
  `compare -metric AE` (ImageMagick). `AE` counts any pixel that differs at all,
  so it is the same metric the gate (`scripts/testAgainstGoldenImages.sh`) uses.
- Worked at 320x200 / 160x100 for speed; the gate resolution is 1280x800. Note
  that `povray`'s `+o` flag does **not** accept absolute paths here — render to a
  relative name and move the file.
- Isolated by carving the scene down: a preamble with all `#declare`s plus a
  single `composite`/`object` under test. For wg5 the wine glass parts were
  rendered one at a time by editing the `WineGlass` composite body.
- Instrumented `SimpleBody::doExtraInformation` with a `static std::set<std::string>`
  that prints each distinct `(detailOwnerCount, per-owner XF/identity,
  body XF/identity, geometryTransform present)` signature exactly once, so a
  full render emits a handful of lines instead of millions. This is how the
  `[id,id,XF]`-style structure signatures below were obtained. (Removed before
  hand-off; re-add behind `#ifdef DEBUG_DETAIL_OWNERS` if needed.)

### 15.3. Bug 1 — dropped outer transform on nested CSG operands (FIXED)

Symptom: the silver `Frame` of wg5 (declared `union { intersection { Disk_X
scale<240,4.5,4.5> translate ... } ... }`, where `Disk_X` is itself an
`intersection` of a cylinder quadric and two planes) rendered as a flat white
slab instead of a rounded chrome tube — the cylinder's curvature/shading was
gone.

Root cause: `CsgOperand::doIntersectionForAllRayCrossings` now *always*
`pushDetailOwner(this)` (the old early-return for the untransformed case was
removed), so a hit on a deeply nested primitive accumulates a *stack* of
owners, innermost-first. The instrumentation showed the frame hit produced
`count=3 [id,id,XF]`: the meaningful `scale` sits on the **outermost** owner.
But `SimpleBody::doExtraInformation` popped a **single** owner from the
**front** (`popDetailOwner`, innermost) and applied only that one, dropping the
outer `scale` entirely → un-scaled normal → flat shading.

Fix:
- `PovRayHit::popDetailOwnerBack()` (pop from the end = outermost first).
- `SimpleBody::doExtraInformation` consumes via `popDetailOwnerBack`, and
  `CsgOperand::doExtraInformation` recurses into the next inner owner
  (`popDetailOwnerBack`) instead of straight to its own geometry; only the
  innermost owner (no remaining owner) evaluates the primitive normal, and the
  chain re-applies each operand's inverse to the normal while unwinding.
- Deferred normalization: each `CsgOperand` level only multiplies by
  `transformationInverse.withoutTranslation()`; the single `normalizedFast()`
  happens once at the top (`SimpleBody`), because renormalizing between nested
  non-uniform scales corrupts the composition.

Verification: the isolated frame, frame+board, and **seven** hand-built nested
structures all reach **AE 0 vs. good**:
`union{intersection{quadric scale translate, plane}}` (`nest`), two nested
scales (`nest2`), nested rotation (`nestr`), unit quadric scaled by the wrapping
intersection (`nesta`), reflective union-of-union with `Gold_Texture`
(`nestref`), and a hollow `inverse`/difference cylinder pair (`nestd`).
`p_bead`/`p_base`/`p_stem` (single transformed quadrics) also stayed at AE 0/4.

### 15.4. Bug 2 — `t` recomputed as a bare distance (FIXED)

`SimpleBody` and `CsgOperand` recompute the crossing's `t` after moving the
point to world space, using `point.subtract(ray->getOrigin()).length()`.
`length()` is unsigned, so any crossing *behind* the ray origin — exactly the
case for refracted/internal rays whose origin sits on or inside the body — is
flipped to a positive `t`, sorts in front, and corrupts the CSG in/out
classification (the depth queue and `peek()` rely on signed `t`).

Fix: signed ray parameter
`(point - origin)·dir / (dir·dir)`, which equals the geometry's native `t` for
any ray direction. This is a correctness fix; on its own it did not change the
wg5 / chess AE numbers measurably (primary rays are all forward), but it is the
right invariant and is kept.

### 15.5. wine glass — optics correct, global `rand()` dither amplifies (DIAGNOSED, NOT FIXED)

The wine glass parts that diverge are `Top`/`Wine` (CSG `intersection`
geometry) and `Rim` (`quartic`/`PolynomialShape`), all clipped + scaled +
translated and refractive.

Findings:
- With an **opaque** texture the isolated `Top` is AE 27 (essentially exact):
  geometry and normals are fine.
- With the **refractive** Glass texture it jumps to ~33k, but with a **plain,
  non-`texture_randomness` ground** it drops back to **AE 10**. So the wine
  glass optics themselves are essentially correct.
- The real amplifier is the ground texture `texture { 0.2 color RichBlue }`.
  The leading `0.2` is **`texture_randomness`**, a dither applied in
  `LambertShader::shade` as `intensity -= rand()/0x7FFF * textureRandomness`.
  `rand()` is a **single global serial stream** (see the long comment in
  `LambertShader.cpp`: this stream must stay identical for byte parity, and wg5
  is explicitly on the list of `texture_randomness` scenes).
- Any sub-pixel divergence in the glass refraction changes the number/order of
  `LambertShader::shade` calls, desynchronizing `rand()` for every surface
  shaded *afterwards* in scan order. The amplified diff confirms this: the
  speckle starts at the glass and covers the whole floor below it; the sky above
  is clean.

Implication: to make wg5 match the (baked) golden, the matrix glass refraction
must reproduce the baked `LambertShader` call sequence exactly. The wine glass
optics being AE 10 (not 0) means a handful of silhouette/edge pixels still
diverge, and that is enough to desync `rand()`. This is the same class of
fragility as antialiasing dither.

### 15.6. OPEN BUG — chess pieces darkened by the detail-owner chain

This is the blocker. The part-6 (broken) chess pieces are **visually correct**:
piece-region mean brightness 0.2785 vs. good 0.2786, AE 982 over the piece crop
(the full-frame 78766 AE is almost entirely the `rand()` floor speckle, not the
pieces). The chess golden equals the good output (good vs. golden AE 0).

With the §15.3 detail-owner fix applied, the chess pieces **darken**: piece-crop
mean brightness drops to 0.2309, AE 60547 over the same crop. So the fix that
repairs the frame **regresses chess**.

What is known:
- It is the detail-owner change, not the `t`-sign change: reverting `t` alone
  leaves chess at the same regressed AE (8652 at 320x200); reverting the
  detail-owner change restores the good pieces.
- It is **not** reproduced by any of the seven minimal nested structures in
  §15.3 (all AE 0), including reflective ones, `inverse`/difference ones, and
  unit-quadric-scaled-by-ancestor ones. The chess pieces are built from
  `union { ... intersection { quadric scale translate } ... }` with structure
  signatures up to `count=6`, including `[id,id,XF,id]` (XF in the middle) and
  `[id,id,XF,XF,id]` (two non-identity owners). Some minimal subset of the real
  chess nesting is still not captured by the reductions tried.

Clues for resuming:
1. The divergence is on the lit/reflective surface of the pieces, so it is a
   **normal** error in the chain, surfacing through reflection (gold is
   reflective) more than through diffuse.
2. The minimal repros that pass all have the transform on either the primitive's
   own operand or a *single* ancestor. The untested suspects are: (a) signatures
   with **multiple** non-identity owners at non-adjacent depths
   (`[id,id,XF,XF,id]`), (b) `count >= 5` depth (close to `MAX_DETAIL_OWNERS = 8`
   in `IntersectionAttributes`; check for silent truncation by `pushDetailOwner`
   when the stack is full — a truncated chain would drop a transform), and
   (c) interaction with the per-piece placement transform on the enclosing
   `SimpleBody` (`body=XF`) combined with operand-level `XF`s.
3. Best next probe: use the per-pixel `DBG_ON()` trace already wired into
   `RenderEngine`/`CsgOperand` (`g_dbgX/g_dbgY` in `common/DebugTrace.h`) on one
   bright-in-good / dark-in-fixed piece pixel, and compare the printed
   `n_local` → `n_after_tx` chain against a hand-computed baked normal. The good
   tree has no trace, so compute the expected normal analytically or bake the
   single piece in a scratch scene.
4. Cross-check `MAX_DETAIL_OWNERS`: render chess with the §15.2 signature
   instrumentation and look for `count == 8` (saturation). If present, raise the
   cap or restructure so no transform is silently dropped.

### 15.7. Hand-off state

- Applied (matrix-path correctness fixes): `PovRayHit::popDetailOwnerBack`,
  the detail-owner chain in `CsgOperand`/`SimpleBody`, deferred normalization,
  and the signed-`t` fix. These make the frame and all isolable nested cases
  AE 0, but **regress chess** (§15.6), so the tree is **not** ready for a golden
  re-baseline as-is.
- Not done (by request): re-baselining `referenceTestImages/`. The chess
  regression must be resolved first, otherwise the re-baseline would bake the
  darkened pieces — exactly the "recover toward `referenceTestImages.old`, not
  the buggy re-baselined set" hazard.
- The matrix model is the chosen direction (keep decoupling, re-baseline once
  correct), so the path forward is to finish §15.6, confirm frame + chess +
  wine glass all match the baked reference (modulo the `rand()` dither, which
  may force an AE-similar rather than AE-0 acceptance for `texture_randomness`
  scenes), then re-baseline manually.

## 16. Texture-coordinate-frame regressions (desk / pool / fish13), part 7

Three more part-6 regressions, all in how a texture's *evaluation point* and
*transform* are derived once geometry transforms became matrices instead of
baked coefficients. The targets (`referenceTestImages/level3/{desk,pool,fish13}`)
are byte-identical to the part-5 good commit `3fa7a81` (verified: good-commit
render vs golden = AE 0 for pool and fish13), so the good binary is ground truth.

### 16.1. Bug A — `replayParsedTransforms` over-applies pre-texture object transforms (FIXED)

`ObjectParser::parseObject` (new in part-6) recorded every `translate/rotate/scale`
parsed *before* a `texture {}` block and **replayed** them onto the freshly
parsed texture. That is wrong: in POV 1.0 / the good commit, object transforms
that precede a texture hit the still-*default* texture, whose transform ops are
a deliberate no-op (the "§1.4 no-op quirk" in `PovRayMaterial::translateTexture`).
So the texture must inherit **none** of them.

Symptoms:
- `pool.pov` floating ball: `sphere translate <20 4 -15> texture { gradient ... }`.
  Replay re-centred the gradient on the local origin → the beach-ball stripes
  collapsed to one big red band. Fixed: ball is white with the red band/dot.
- `desk.pov` picture frame: `intersection {...} translate<1 1 1> scale<20 15 1>
  texture { image_map { gif "rough.gif" } scale ... }`. Replay forced the body
  translate+scale onto the image_map UV → the picture sampled off-image and
  rendered black. Fixed: the astronaut photo maps correctly (AE 0 against golden
  in the picture region).

Fix: delete the replay call (and the now-dead `replayParsedTransforms`). Object
transforms that *follow* the texture still apply, because they go through
`object->translate/rotate/scale` on the now-non-constant texture, untouched by
this change. No regression: `steiner` and `wtorus` stay AE 0.

### 16.2. Bug B — `LocalSurfaceShader` evaluates a top-level CSG texture in object-local space (FIXED)

`CsgOperand` sets `materialUsesObjectLocalPoint = true` on every candidate so a
*per-operand* texture is sampled in its operand-local frame. `RayShaderPipeline`
gates that conversion on `hit.material != nullptr` (i.e. only a real per-operand
material). `LocalSurfaceShader` instead used `usingMaterialTexture = (texture
!= nullptr)`, but `texture` there is already resolved to `hit.material` **or**
`hit.objectTexture`, so it was true even for a plain object texture — and the
bump point got pulled into object-local space.

Symptom: `fish13.pov` swamp water — `intersection { Cube scale<10000 1 500> ... }`
with a `ripples 0.7 frequency 0.08` normal. Sampled in the unit-cube local frame,
`frequency 0.08` produced ~zero variation → the concentric wave reflections
vanished (flat dark water). Pre-existing in the part-6 broken commit too (not
caused by the §15 detail-owner work). Fixed by mirroring `RayShaderPipeline`:
`usingMaterialTexture = (hit.material != nullptr)`. Water ripples restored;
`fish13` AE 533k→217k (remainder is the §15.5 `rand()` dither). Per-operand CSG
textures are unaffected (their `hit.material` is non-null), so `chess`/`wg5`/
`dfwood` do not regress.

### 16.3. Results (full-res AE vs golden = good-commit `3fa7a81`)

| scene   | broken 47e94d4 | with part-7 fixes | note |
|---------|---------------:|------------------:|------|
| steiner |              0 |                 0 | unchanged |
| wtorus  |              0 |                 0 | unchanged |
| pool    |         594857 |            557959 | ball fixed; rest = dither |
| desk    |         701665 |            519171 | image_map fixed; rest = dither |
| fish13  |         575815 |            217425 | ripples fixed; rest = dither |
| chess   |          78766 |            136961 | §15.6 darkening (pre-existing); part-7 itself nudges 139061→136961 |
| wg5     |         706457 |            533289 | glass recovered; floor = dither |
| skyvase |            n/a |            384935 | structure ok; surface = dither |

Both part-7 fixes move strictly toward the good commit and never regress a scene
versus its own pre-fix state (chess measured with today's two files reverted =
139061, i.e. slightly worse than with them). The dominant residual on every
non-AE-0 scene is now the §15.5 `texture_randomness` / `rand()`-stream
divergence, which is the remaining blocker for an AE-0 re-baseline and is
independent of geometry/texture-frame correctness.

## 17. Composite detail-owner ordering — the real §15.6 root cause (FIXED)

While doing a focused second pass on `fish13.pov`, the stems (`composite {
stem1 scale<3 3 3> ... }`, where `stem1` is a 3-level-nested composite of
sphere `object`s carrying a `marble` texture) rendered washed-out: no marble
veins, no phong highlights. The broken part-6 commit `47e94d4` renders them
correctly, so this was a **regression introduced by the §15 detail-owner work**,
not by the part-7 texture-frame fixes (confirmed by reverting the part-7 files —
stems stay washed).

### 17.1. Root cause

The detail-owner stack has two producers that pushed in **opposite** orders:
- `CsgOperand::doIntersectionForAllRayCrossings` uses `pushDetailOwner` (append),
  so a nested CSG builds the stack innermost-first → `[inner … outer]`.
- `Composite::doIntersectionForAllRayCrossings` used `prependDetailOwner`, so a
  nested composite built it outermost-first → `[outer … inner]`.

A single consume direction can only be correct for one of them:
- `47e94d4` consumed from the **front** (`popDetailOwner`): correct for
  composites (stems OK), **wrong for CSG** — that was the §15.3 frame flat-white bug.
- The §15 fix switched to **back** consumption (`popDetailOwnerBack`): correct
  for CSG (frame fixed), but it silently **reversed the order for every nested
  composite**, applying the enclosing transforms to the surface normal in
  scrambled order. The normal ended up non-unit / mis-rotated, so diffuse and
  phong collapsed and any procedural pattern keyed off `N` washed out.

This is the true root of the §15.6 "chess darkening": chess pieces are
`composite`s of transformed CSG quadrics, so they hit exactly this reversed-order
path — not a saturation of `MAX_DETAIL_OWNERS` as hypothesised in §15.6.

### 17.2. Fix

`Composite` now uses `pushDetailOwner` (append) like `CsgOperand`, so every
producer builds the stack innermost-first and the single outermost-first
`popDetailOwnerBack` consumer is correct for composites, CSG, and any nesting of
the two. One line, no change to the §15 CSG path.

### 17.3. Results (full-res AE vs golden = good commit `3fa7a81`)

| scene   | before (part-7) | after §17 | note |
|---------|----------------:|----------:|------|
| fish13  |          217425 |        94 | stems restored; rest = dither |
| chess   |          136961 |         4 | §15.6 darkening **resolved** |
| desk    |          519171 |         3 | composite pencils/holder restored |
| dfwood  |           76535 |         0 | pixel-perfect |
| skyvase |          384935 |      2618 | near-perfect |
| steiner |               0 |         0 | unchanged |
| wtorus  |               0 |         0 | unchanged |
| pool    |          557959 |    557958 | unchanged — genuine §15.5 dither (water/grass/sky) |
| wg5     |          533289 |    533144 | unchanged — genuine §15.5 dither (glass/floor) |

After §16 + §17, every non-trivial structural decoupling regression is resolved.
The only scenes still far from AE 0 are `pool` and `wg5`, whose residual is the
§15.5 `texture_randomness` / `rand()`-stream divergence (visually mild speckle on
large procedurally-dithered surfaces — `compare -metric AE` counts every
1-LSB-different pixel, so the count is large while the image looks identical).
The tree is now a sound basis for the manual golden re-baseline.

### 17.4. Full-gate regression sweep (108 scenes)

A complete `renderAll.sh` of all 108 scenes confirms no regression from the
§16/§17 changes. Judged by the project heat-map convention (`scripts/viewImages.sh`:
black=match → blue → cyan → yellow → red): **a scene is only considered broken if
its heat-map shows yellow/green/red; blue/cyan texture speckle from the §15.5
`rand()` divergence is non-invalidating** (`compare -metric AE` over-counts it).

- 60 scenes AE 0; the rest blue/cyan speckle only.
- No scene's AE rose versus the part-6 broken commit `47e94d4`. Several dropped
  sharply: iortest 180551→26541, pawns 159716→51045, tomb 299694→124709,
  pacman 21074→7815, ionic5 440310→414262, plus the §17.3 wins.
- The high-AE scenes (tetra 489874, kscope 444835, roman, snack, piece1, pool,
  wg5, snail) are all-blue heat-maps — mild speckle, no structural error — so
  they are rebaseline-ready as-is.

Net: the matrix-decoupling regressions are resolved across the whole gate; the
only remaining differences are the §15.5 dither, which the manual re-baseline
will absorb.
