# Plan: Decouple `Geometry` from transformations, `Material` and `ColorRgba` via `TranslatedBody`

## Goal

Currently `Geometry` inherits from `TransformableElement` and every concrete
shape (`Sphere`, `Box`, `Quadric`, `Blob`, `HeightField`, `Triangle`,
`SmoothTriangle`, `ParametricPatch`/`ParametricBiCubicPatch`,
`PolynomialShape`, `InfinitePlane`, `Light`, `CSG`, `Composite`,
`SimpleBody`) carries its own `material` (`Material*`) and `shapeColor`
(`ColorRgba*`) fields, and implements `translate/rotate/scale/invert` by
**baking** the transform directly into its own geometric parameters (e.g.
`Sphere::translate` mutates `center`; `Quadric::translate` remultiplies its
coefficient matrices) while also calling
`MaterialUtils::instance().xTexture(&material, vector)` to keep the
material's `textureTransformation`/`textureTransformationInverse`
(`Matrix4x4d*`) in sync.

The goal is to introduce a new class `TranslatedBody` (in
`environment/scene`) that owns a `Geometry*`, a `Material*`, a `ColorRgba*`
and (additively) a cumulative `Matrix4x4d transform`/`transformInverse`,
leaving `Geometry` as a purely geometric type (intersection math only, no
material/color/transform-with-material concerns).

## Key architectural finding

The current primitives do **not** store a transform and defer it to
intersection time (the "modern raytracer" approach of transforming the ray
into object space). Instead they permanently bake every
`translate/rotate/scale` into their own geometric data. Switching to a
deferred-transform model would change the floating-point operation order for
almost every scene that uses transforms (~40 of ~50 reference scenes), which
is incompatible with the strict pixel-exact gate
(`compare -metric AE` must report 0 difference, i.e. `Test passed.`).

Therefore this plan does **not** change the intersection algorithm at all. It
only relocates *which class owns which fields/methods*, while keeping the
exact same arithmetic and operation order — so byte-identical (AE=0) output
is achievable at every gate.

`material`/`shapeColor` are **per-geometry-node** attributes today (each
leaf shape, each CSG child, each triangle in a mesh can carry its own
override), set individually by ~11 parsers under `io/pov/geometry` and
`io/pov/light`. After the refactor, every such node becomes a
`TranslatedBody` wrapping a pure `Geometry`.

`TransformableElement` remains as the shared polymorphic interface used by
`Camera`, `Light`, and the `(TransformableElement*)ctx.constants()[id]`
pattern in parsers for transforming previously-declared identifiers.
`TranslatedBody` implements it with "full" semantics (geometry + material +
bookkeeping); `Geometry` implements it with "pure geometry" semantics — no
collision since they are different classes.

## Gate (run after every phase)

```
./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh
```

Expected output: `Test passed.`

## Suggested model per phase

| Phase | Model | Why |
| --- | --- | --- |
| A1-A10 | **Haiku** | Pure mechanical code motion, one well-specified pattern repeated per file, each sub-phase self-checked by the gate. |
| B | **Haiku** | New class from an explicit spec, no judgment calls. Quick Sonnet review recommended since Phase C depends on this pattern. |
| C | **Sonnet or Opus** | Cross-cutting "big bang" across ~25 files with un-specified judgment calls (constant wrapping, `dynamic_cast` targets). Failure mode is a silent AE mismatch — needs careful reasoning, not Haiku. |
| D | **Sonnet** | Identifying genuinely dead code requires tracing Phase C's result. |
| E | **Sonnet** (Haiku plausible with a worked example) | Mirrors the existing `MaterialUtils` accumulation pattern. |
| F | **Sonnet** | Naming/interface review and final polish. |

---

## Phase A — Extract "pure geometry" per shape type (10 sub-phases, mechanical, AE=0 trivial)

> **Model: Haiku**

For each type, split the current `translate/rotate/scale/invert` into:

- `xGeometry(vector)` — exactly today's geometric code (mutate `center`,
  `radius`, quadric coefficient matrices, etc.), **without** any
  `MaterialUtils` call.
- The current public method becomes
  `{ xGeometry(v); MaterialUtils::instance().xTexture(&material, v); }` —
  identical body to today, just reorganized.

This is pure code motion: no arithmetic change, no reordering. Trivial gate
each time.

1. **A1** — `Sphere` (`environment/geometry/volume/Sphere.h/.cpp`)
2. **A2** — `Box` (`environment/geometry/volume/Box.h/.cpp`)
3. **A3** — `InfinitePlane` (`environment/geometry/surface/InfinitePlane.h/.cpp`)
4. **A4** — `Quadric` (`environment/geometry/volume/Quadric.h/.cpp`)
5. **A5** — `Blob` (`environment/geometry/volume/Blob.h/.cpp`)
6. **A6** — `HeightField` (`environment/geometry/volume/HeightField.h/.cpp`)
7. **A7** — `Triangle` + `SmoothTriangle` (`environment/geometry/elements/Triangle.*`, `SmoothTriangle.h`)
8. **A8** — `ParametricPatch`/`ParametricBiCubicPatch` (`environment/geometry/surface/parametric/*`)
9. **A9** — `PolynomialShape` (`environment/geometry/volume/polynomial/PolynomialShape.*`)
10. **A10** — `Light` base (covers `PointLight`/`SpotLight`) (`environment/light/Light.*`)

> `CSG`, `Composite`, `SimpleBody` are deferred to Phase C because their
> recursion (`for shapes[i] -> GeometryOperations::translate(...)`) changes
> the element type (`Geometry*` -> `TranslatedBody*`); splitting them earlier
> would not make sense.

---

## Phase B — `TranslatedBody` skeleton (additive, unused so far)

> **Model: Haiku** (Sonnet review recommended before Phase C starts)

**Prerequisite step (important, not optional):** Phase A added
`translateGeometry/rotateGeometry/scaleGeometry/invertGeometry` as plain
(non-virtual) methods on each of the 11 concrete classes (Sphere, Box,
InfinitePlane, Quadric, Blob, HeightField, Triangle, SmoothTriangle,
ParametricBiCubicPatch, PolynomialShape, Light). For `TranslatedBody` to call
`geometry->translateGeometry(v)` polymorphically through a `Geometry*`, these
four methods must be declared **virtual** on `TransformableElement` (mirroring
the existing `virtual void translate(Vector3Dd*) {}` etc., with empty default
bodies so nothing that doesn't implement them breaks — e.g. `Camera`). Then
add the `override` keyword to the matching declarations in each of the 11
concrete headers (no `.cpp` changes needed there — the bodies already exist
and match).

- New file `environment/scene/TranslatedBody.h/.cpp`.
- Fields: `Geometry *geometry; Material *material; ColorRgba *shapeColor; Matrix4x4d transform; Matrix4x4d transformInverse;`
- Implements `TransformableElement` with:
  - `allIntersections/inside/normal/copy` -> delegate to `geometry->...`
  - `translate/rotate/scale` -> `geometry->xGeometry(v)` +
    `MaterialUtils::instance().xTexture(&material, v)` (identical to what
    each `Geometry::x` does today)
  - `invert` -> `geometry->invertGeometry()`
  - getters/setters: `getMaterial/setMaterial/getShapeColor/setShapeColor`
- Not wired into anything yet (dead but compilable code). Gate: build +
  golden unchanged (no behavior change is possible).

---

## Phase C — Structural rewiring (the "big bang", single phase, gate at the end)

> **Model: Sonnet or Opus**
>
> **STATUS: DONE — gate `Test passed.` (AE=0).** Executed as a single pass.
> Key decisions taken during implementation (details the plan had glossed):
> - `Intersection::Shape` is now `TranslatedBody*`. Leaf shapes still stamp
>   their own `Geometry*` address as a sentinel; `TranslatedBody::allIntersections`
>   delegates to the wrapped geometry then rewrites only the entries whose
>   `Shape == (TranslatedBody*)geometry` to `this`. This needed non-const
>   `begin()/end()` on `java::PriorityQueue` (added).
> - **`Light` keeps its own `material`/`shapeColor`** (it is reached directly as
>   a `Light*` by `LightSamplerShader` and the `lightSources` list, never shaded
>   through `Intersection::Shape`). Only `Light` is exempt from the strip.
> - Parsers now build the leaf, wrap it via the new `ModelBuilder::wrap(Geometry*)`
>   helper, and route material/colour/transform onto the `TranslatedBody`,
>   returning it. `LightSourceParser` keeps applying transforms to the `Light`
>   directly (Light retains its full transform methods) and only wraps at return.
> - `CSG`/`SimpleBody`/`Composite` child lists became `ArrayList<TranslatedBody*>`;
>   `CSG` gained `*Geometry` recursion so it works both directly (parse-time) and
>   when itself wrapped in a `TranslatedBody`.
> - `ParseHelpers::postProcessShape` now takes `TranslatedBody*` and
>   `dynamic_cast`s on `shape->geometry`.

Internal checklist (all in the same phase because the type tree must be
consistent to compile):

1. **`Geometry.h`**: remove `material`, `shapeColor`, their getters/setters,
   and the "full" `translate/rotate/scale/invert` (only the `xGeometry`
   methods from Phase A remain, renamed back to
   `translate/rotate/scale/invert` since `Geometry` no longer has a semantic
   collision).
2. **`CSG.h/.cpp`**: `shapes` becomes `ArrayList<TranslatedBody*>` (from
   `ArrayList<Geometry*>`). `CSG::translate/rotate/scale/invert/copy/
   allIntersections/inside` recurse via `shapes[i]->translate(v)` etc. (now
   full, on `TranslatedBody`).
3. **`SimpleBody.h` / `Composite.cpp`**: `geometry`, `boundingShapes`,
   `clippingShapes` become `TranslatedBody*` /
   `ArrayList<TranslatedBody*>`. Adjust
   `SimpleBody::translate/rotate/scale/invert/copy/allIntersections/inside`.
4. **`GeometryOperations.h`**: add overloads (or variants) operating on
   `TranslatedBody*` where needed (intersection/inside still need the pure
   `Geometry*` via `translatedBody->geometry`).
5. **`Intersection.h`**: `Shape: Geometry*` -> `Shape: TranslatedBody*` (so
   shading can call `Shape->getMaterial()/getShapeColor()`).
6. **11 geometry/light parsers** (`SphereParser`, `BoxParser`,
   `QuadricParser`, `BlobParser`, `HeightFieldParser`, `TriangleParser`,
   `SmoothTriangleParser`, `BicubicPatchParser`, `PlaneParser`, `PolyParser`,
   `LightSourceParser`): construct a `TranslatedBody` wrapping the newly
   created `Geometry`; move `setShapeColor`/`material = ...` from the
   `Geometry` to the `TranslatedBody`.
7. **`ObjectParser.cpp`**: `GeometryOperations::translate/rotate/scale/invert
   (container/object, ...)` now operate on `TranslatedBody*`.
8. **`DeclarationParser.cpp` / `ctx.constants()`**: declared identifiers that
   wrap shapes must store `TranslatedBody*` (so a later `translate` on the
   identifier also transforms the material).
9. **`render/RayShaderPipeline.cpp`,
   `shaders/LocalSurfaceShader.cpp`, `shaders/LightSamplerShader.cpp`,
   `shaders/DirectLightShader.cpp`**:
   `rayIntersection->Shape->material/shapeColor` ->
   `rayIntersection->Shape->getMaterial()/getShapeColor()`.
10. **`ParseHelpers.cpp`** (`postProcessShape`, `dynamic_cast<CSG*>`,
    `dynamic_cast<Light*>`): adjust to the new types in `shapes`/`geometry`.

Gate: a single run at the end of the whole phase (it won't compile halfway
through). If AE fails, suspect #1 is a spot where `Geometry::translate`
(full) used to be called and now only the pure-geometric variant runs
without going through `TranslatedBody` (e.g. a `boundingShape`/
`clippingShape` that was not wrapped).

---

## Phase D — Cleanup of leftovers

> **Model: Sonnet**

- Remove any dead `material`/`shapeColor` left in concrete subclasses (some
  set `this->material = nullptr` in constructors — `PointLight`, `SpotLight`,
  etc., already covered by A10 but double-check).
- Remove any now-unused overload in `GeometryOperations`.
- Confirm `SceneObject = SimpleBody` (alias in `scene/SceneObject.h`) is
  still correct.

Standard gate.

---

## Phase E — Populate `transform`/`transformInverse` (additive)

> **Model: Sonnet** (Haiku plausible with a worked example)

- In `TranslatedBody::translate/rotate/scale`, in addition to what Phase
  B/C already does, compose `transform`/`transformInverse` the same way
  `MaterialUtils` composes `textureTransformation`/`Inverse` (same existing
  pattern, just replicated at the `TranslatedBody` level).
- Purely additive (new fields, read by nobody else) -> AE=0 guaranteed.
- This satisfies the "possibly the Matrix4x4d transforms and inverses" part
  of the request without touching the intersection algorithm.

Standard gate.

---

## Phase F — Final polish

> **Model: Sonnet**

- Review naming (`xGeometry` -> final names if a different convention is
  preferred).
- Confirm `TransformableElement.h` still makes sense as a shared interface
  (`Camera`, `Light`, `TranslatedBody`, `Geometry`), or whether it should be
  split into two interfaces (full `Transformable` vs pure
  `GeometricallyTransformable`).
- Final build + golden run.

---

## Risks to watch

- **Phase C is large** (~25 files) but mechanical and compiler-guided — the
  real risk is in places **not** caught by the compiler: any
  `dynamic_cast<X*>(geometry)` that relied on `geometry` being directly the
  concrete type (`CSG`, `Light`, `Composite`) and now has a `TranslatedBody`
  in between. `ParseHelpers::postProcessShape`/`postProcessObject` use
  `dynamic_cast<CSG*>`/`dynamic_cast<Light*>` — decide whether the cast is
  done on `translatedBody->geometry` or whether `TranslatedBody` exposes a
  typed getter.
- Scenes with **per-component CSG textures** (different textures per child)
  and **triangle meshes with per-triangle color** exercise the per-node
  `material`/`shapeColor` the most — good candidates to validate Phase C
  individually (single-scene render) before running the full gate.
