# Comparative analysis of the intersection model: povCpp <-> VITRAL

Status snapshot comparing the current `povCpp` raytracer with the current
[VITRAL library](https://github.com/OscarChavarro/vitral).

The focus is the required alignment surface between both codebases: rays,
intersection results, traversal primitives, transforms, scene bodies, detail
selection, renderer configuration, bounding information and camera snapshots.

Re-verified against both trees on 2026-07-05.

---

## 1. Shared primitives already in use

### 1.1. `RayWithTracingState` extends VITRAL `Ray`

`povCpp` uses VITRAL's `Ray` directly:

```cpp
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class RayWithTracingState : public Ray { ... };
```

VITRAL `Ray` stores `origin`, `direction` and `t`, and exposes immutable-style
builders (`withOrigin`, `withDirection`, `withT`) plus setters.

`RayWithTracingState` adds POV-specific render state:

- Quadric cached terms: `position2`, `direction2`, `positionDirection`,
  `mixedPositionPosition`, `mixedDirectionDirection`,
  `mixedPositionDirection`, guarded by `quadricConstantsCached`.
- Per-ray AABB slab-test reciprocals (`invDirectionX/Y/Z` plus degenerate-axis
  flags), cached lazily and invalidated together with the quadric cache.
- Nested medium state: `containingTextures`, `containingIORs`,
  `containingIndex`.
- Ray classification: `isShadowRay`, `isPrimaryRay`.
- Per-ray detail mask using VITRAL-compatible `DETAIL_*` constants.
- Runtime pointers: `GeometryStatistics`, `PovRayRendererConfiguration`,
  `PriorityQueuePool<IntersectionCandidate>`. The ray only carries the
  per-primitive `GeometryStatistics` counters; the render-level
  `PovRayRenderStatistics` counters that are not tied to a single ray
  (reflected/refracted/transmitted/shadow ray totals) are reached through
  `TraceService::getStatistics()` instead (see §12).

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

- `hitGeometry` and `hitBody` (the owning `RayOperationOwner`)
- the detail-owner chain (up to `MAX_DETAIL_OWNERS = 8` `RayOperationOwner*`
  entries, pushed innermost-first by nested CSG/composite wrappers)
- `material`
- `objectTexture`
- `objectColor`
- `noShadowFlag`
- `materialUsesObjectLocalPoint`

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
struct GeometryIntersectionEmissionContext {
    Material *materialOverride = nullptr;
    RayOperationOwner *detailOwner = nullptr;
    bool materialUsesObjectLocalPoint = false;
};

virtual int doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride = nullptr);
virtual int doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context);
virtual bool hasNativeAnnotatedCrossings() const;
virtual Geometry *getWrappedGeometry() const;
bool doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out);
virtual bool doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride = nullptr);
virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
virtual void doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit);
virtual AxisAlignedBoundingBox getMinMax() const;
virtual void *copy() = 0;
virtual void invertGeometry();
```

`doIntersectionForAllRayCrossings` is the primary povCpp operation. Shapes push
every ray crossing into a priority queue ordered by `Intersection::t`.

The `...Annotated` variant lets a wrapper (CSG operand, composite) stamp an
emission context — detail owner and object-local-point flag — onto every
candidate a child emits, without re-sorting the queue. Shapes that can stamp
while emitting return `true` from `hasNativeAnnotatedCrossings()`; for the
rest the base class stamps in a post-pass (via a fresh pooled queue when the
target queue is not empty).

`doIntersectionFirstHit` is the virtual *direct* nearest-hit primitive — the
same name and role VITRAL uses. Most shapes get a default (non-overridden)
implementation that always misses; shapes with a cheap closed-form first hit
(e.g. `Sphere`) override it directly. Shapes without one still support
nearest-hit through `Geometry::doIntersectionFirstHitViaCrossings`, a
non-virtual adapter that derives it from all-crossings:

```cpp
bool Geometry::doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out)
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

Alignment status: both projects expose `doIntersectionFirstHit` as the direct
nearest-hit virtual, with the same meaning. The queue-deriving adapter has its
own, non-colliding name (`doIntersectionFirstHitViaCrossings`).

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
| `requiredDetailMask` | `RayWithTracingState::requiredDetailMask` | constants aligned; owner differs |
| consumed `Ray` | `RayWithTracingState` plus `Intersection::t` | no stored consumed-ray object in candidate |
| no VITRAL field | `hitGeometry`, `hitBody`, detail-owner chain, `objectColor`, `noShadowFlag`, `materialUsesObjectLocalPoint` | POV-specific |

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

`povCpp` `Geometry` is transform-free, and so is every concrete shape
(`Sphere`, `Box`, `Quadric`, `TriangleMesh`, `InfinitePlane`, `Blob`,
`HeightField`, `ParametricBiCubicPatch`, `PolynomialShape`,
`ConstructiveSolidGeometry`). Placement is owned by the two wrapper types
that also own detail production (`RayOperationOwner` implementors):

- `SimpleBody` holds **two** lazily-allocated matrix layers: an object-level
  pair (`transformation`/`transformationInverse`) and an inner geometry-level
  pair (`geometryTransformation`/`geometryTransformationInverse`), plus the
  chronological `TransformStep` lists (`bodySteps`, `geometrySteps`) those
  matrices were composed from, and a `bakedTransformFolded` flag set when the
  baked-scene build collapsed the transforms into world-space coefficients.
- `CsgOperand` holds one `transformation`/`transformationInverse` pair per
  CSG operand, its own `TransformStep` list, the same `bakedTransformFolded`
  flag, and a cached transformed AABB used for union-operand culling.

`nullptr` matrices mean identity. Matrices are allocated lazily and
accumulated at parse time; rays are transformed inward and hit details
outward at trace time, exactly VITRAL's scheme.

Alignment status: transform ownership is now **conceptually aligned** — both
codebases keep `Geometry` transform-free and own placement at the scene-body/
operand level. What still differs is the representation: VITRAL `SimpleBody`
stores a decomposed `position`/`scale`/`rotation` (+quaternion) with
fast-path identity flags, while povCpp stores composed matrix pairs plus the
`TransformStep` replay log, in two layers, with the bake-folded flag.

### 4.3. Canonical shapes

`povCpp` `Sphere` carries its own `radius` (default `1.0`) in the geometry;
placement (translation/rotation/any additional scale) is owned by the
`SimpleBody`/`CsgOperand` transform, which maps the ray into the sphere's
local space before intersecting.

VITRAL `Sphere` carries `radius_`/`radiusSquared_` in the geometry and relies on
`SimpleBody` for body placement.

Alignment status: intrinsic-dimension ownership is aligned — both put radius
on the shape and placement on the owning body/operand.

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

`povCpp` `SimpleBody` is **not** a `Geometry` subclass: it implements
`RayOperationOwner` (the povCpp-only interface whose single contract is
`doExtraInformation`) and is the scene object stored in `Scene::Objects`:

```cpp
class SimpleBody : public RayOperationOwner {
  protected:
    java::ArrayList<SimpleBody*> boundingShapes;
    java::ArrayList<SimpleBody*> clippingShapes;
    Geometry *geometry;
    Material *geometryMaterial;
    Matrix4x4d *transformation = nullptr;          // object layer
    Matrix4x4d *transformationInverse = nullptr;
    Matrix4x4d *geometryTransformation = nullptr;  // inner geometry layer
    Matrix4x4d *geometryTransformationInverse = nullptr;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
    java::ArrayList<TransformStep> bodySteps;
    java::ArrayList<TransformStep> geometrySteps;
    bool bakedTransformFolded = false;
};
```

It handles:

- Bounding region tests before object intersection.
- Clipping region filtering after object crossings are gathered.
- Object-level material, texture, quick color and no-shadow attribution.
- Transform ownership (§4.2): world→local ray mapping on the way in,
  point/normal mapping on the way out, in `doExtraInformation`.
- `translate`/`rotate`/`scale`/`invert` accumulation into its matrix layers
  and propagation into bounding shapes, clipping shapes, material and object
  texture (virtual, so nested `Composite` children receive outer transforms).
- `getAABB()` from bounding regions or the wrapped geometry's `getMinMax()`.
- `doIntersectionFirstHit` (virtual here, unlike the `Geometry` adapter) and
  `doIntersectionForAllRayCrossings` traversal entry points.

`Composite` extends `SimpleBody` and contains nested `SimpleBody*` children.
Its transforms propagate into children and region shapes.

Alignment status: the `SimpleBody` name is shared, both are scene-object-level
concepts, and both own placement at this level. Residual differences:
povCpp's `SimpleBody` also carries POV attribution (quick color, no-shadow),
bounding/clipping regions and the two-layer matrix scheme; VITRAL's carries
render-time material/texture/normal-map pointers and a modification-version
check.

Required alignment item: preserve the shared scene-body concept; converge the
transform representation (decomposed-with-flags vs matrix-pairs-with-step-log)
when geometry migration starts.

---

## 6. CSG and all-crossings in povCpp

`ConstructiveSolidGeometry` is an abstract `Geometry` holding a
`BooleanSetOperations` type (`UNION`, `INTERSECTION`, `DIFFERENCE`) and an
`ArrayList<CsgOperand*>`. Each `CsgOperand` (a `RayOperationOwner`, like
`SimpleBody`) owns one child `Geometry*`, an optional per-operand material
override, its own `transformation`/`transformationInverse` pair with
`TransformStep` log (§4.2), and a cached transformed AABB used to cull union
operands.

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

`povCpp` mirrors the same constants on `RayWithTracingState`. The mask lives on the
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

povCpp `Geometry` now exposes the same-named method directly on the base, as
a non-pure virtual with an unbounded default:

```cpp
virtual AxisAlignedBoundingBox getMinMax() const
{ return AxisAlignedBoundingBox::unbounded(); }
```

Concrete povCpp geometries overriding it: `Sphere`, `Box`, `Blob`, `Quadric`,
`HeightField`, `TriangleMesh`, `ParametricBiCubicPatch` and
`ConstructiveSolidGeometry` (which unions its operands' transformed bounds).
`CsgOperand` caches the child's bounds transformed by its operand matrix for
union culling; `SimpleBody::getAABB()` returns the intersection of
bounding-shape AABBs when bounding regions exist, otherwise the wrapped
geometry AABB.

Alignment status: the **owner is now aligned** — both codebases put
`getMinMax()` on `Geometry`. Only the return type diverges: VITRAL returns a
raw `double*`, povCpp a value-type `AxisAlignedBoundingBox` with an explicit
unbounded state.

Required alignment item: agree the return type. povCpp's
`AxisAlignedBoundingBox` is the stronger API (no raw pointer, explicit
unbounded semantics) and is the proposed winner.

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

povCpp `PovRayRenderStatistics` (`src/render/shaders/PovRayRenderStatistics.h`)
is per-render/per-task and records POV-specific counters: ray totals, shadow
tests, reflected/refracted/transmitted rays and related values, plus two
embedded sub-objects: `SolidTextureStatistics` for texture-specific counts,
and `GeometryStatistics` (`base/src/main/vsdk/toolkit/common/statistics/`,
alongside `SolidTextureStatistics`, built into `vitral_base`) for the
per-primitive test/success pair counters and bounding/clipping region
counters. `RayWithTracingState` carries only a `GeometryStatistics *`; the
render-level counters that are not tied to a single ray are reached by
shaders through `TraceService::getStatistics()` instead, since `TraceService`
already threads through the whole shader call chain (`RayShaderPipeline` →
`LocalSurfaceShader` → `MirrorReflectionShader`/`DirectLightShader`/
`TransmissionRefractionShader`).

VITRAL `RaytraceStatistics` exposes static recorders for ray categories and
object intersection tests.

povCpp uses `PriorityQueuePool<IntersectionCandidate>` to avoid allocating a
new queue for every intersection operation. VITRAL uses reusable hit/workspace
records in the raytracer path.

Alignment status: both avoid avoidable hot-path allocation, but statistics
ownership and granularity are different. `GeometryStatistics` is colocated
with `SolidTextureStatistics` in the shared `base/` location, but unlike
`SolidTextureStatistics` (which VITRAL-side code — `TextureUtils`,
`ProceduralNoise` — actually consumes), no VITRAL-side code references
`GeometryStatistics` yet: it is positioned for sharing, not yet actually
shared.

Required alignment item: no direct 1:1 statistics mapping exists. A shared
statistics layer needs a common event taxonomy against VITRAL's
`RaytraceStatistics`.

---

## 13. Current alignment matrix

| Area | Current povCpp | Current VITRAL | Alignment status |
|---|---|---|---|
| Ray type | `RayWithTracingState : Ray` | `Ray` | shared base, POV extensions on derived ray |
| Geometric hit core | VITRAL `Intersection` | `Intersection` | shared class |
| Full hit record | `IntersectionCandidate` + `PovRayHit` projection | `RayHit` | conceptually mapped, storage differs |
| Primary intersection primitive | all crossings | nearest hit | divergent, all-crossings required for POV |
| Nearest-hit name | `doIntersectionFirstHit` = direct virtual; queue adapter is `doIntersectionFirstHitViaCrossings` | `doIntersectionFirstHit` = pure virtual direct primitive | name and meaning aligned |
| Containment | `int`, same constants | `int`, enum-backed constants | signature/value aligned |
| Geometry transform | none on `Geometry`; matrix layers on `SimpleBody`/`CsgOperand` | none on `Geometry`; transform on `SimpleBody` | owner aligned (scene level); representation differs |
| Scene body | `SimpleBody : RayOperationOwner`, owns geometry + transforms + regions | `SimpleBody`, wraps geometry and transform | concept and transform ownership aligned; attribution differs |
| CSG | supported through all crossings | no all-crossings CSG path | divergent |
| Detail mask | `RayWithTracingState::DETAIL_*`, normal gate in render engine | `RayHit::DETAIL_*`, per-detail lazy fill | constants aligned, granularity differs |
| Renderer config | derives from VITRAL base plus POV flags | base display/render vocabulary | partially aligned |
| Bounding API | `AxisAlignedBoundingBox getMinMax()` on `Geometry` | `double* getMinMax()` on `Geometry` | owner aligned; return type divergent |
| Camera storage | VITRAL `CameraSnapshot` | `CameraSnapshot` | shared storage, generator semantics differ |
| Statistics (render-level) | `PovRayRenderStatistics`, per-instance/per-task POV counters | static raytrace recorders | divergent |
| Statistics (per-primitive) | `GeometryStatistics`, colocated in `base/` with `SolidTextureStatistics` | static raytrace recorders | positioned for sharing, not yet consumed by VITRAL-side code |

---

## 14. Required alignment work

1. Add or design an all-crossings primitive for VITRAL that can represent
   ordered ray/solid crossings and carry enough attribution for CSG, shadows
   and nested refraction.
2. Converge the transform *representation* on `SimpleBody`/`CsgOperand`:
   decomposed position/scale/rotation with fast-path flags (VITRAL) vs
   composed matrix pairs with a `TransformStep` replay log in two layers
   (povCpp). Ownership (§4.2) is already aligned; only the representation
   differs.
3. Unify or bridge hit-detail masks so point, normal, UV and tangent laziness
   have the same owner and semantics.
4. Define a shared bounding API. The owner is already aligned (`getMinMax()`
   on `Geometry` in both trees, §10); what remains is adopting one return
   type — povCpp's `AxisAlignedBoundingBox` is the proposed winner.
5. Keep `CameraSnapshot` as the shared render-time camera record and treat ray
   generation differences as intentional unless a visual-parity task requires
   alignment.
6. Keep POV quality flags in `PovRayRendererConfiguration` unless VITRAL adds
   equivalent feature-level rendering switches.
