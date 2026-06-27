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
