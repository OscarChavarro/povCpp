# Plan: Replace the C-style `Methods` vtable with real C++ virtual methods

> **Audience:** this document is an implementation plan to be executed by Haiku.
> Follow it **phase by phase**, in order. Do **not** skip ahead. Each phase ends
> with a mandatory **gate** and a mandatory **STOP & notify the user** step.

---

## 0. Background — what exists today

The project implements polymorphism over geometric shapes (and a few
non-geometric scene elements) using a **C-style virtual table**: a plain struct
of function pointers called `Methods` (`src/environment/geometry/Methods.h`).

```cpp
class Methods {
  public:
    ALL_INTERSECTIONS_METHOD allIntersectionsMethod;  // int (*)(SimpleBody*, RayWithSegments*, PriorityQueue<Intersection>*)
    INSIDE_METHOD            insideMethod;             // int (*)(Vector3Dd*, SimpleBody*)
    NORMAL_METHOD           normalMethod;              // void(*)(Vector3Dd*, SimpleBody*, Vector3Dd*)
    COPY_METHOD             copyMethod;                // void*(*)(SimpleBody*)
    TRANSLATE_METHOD        translateMethod;           // void(*)(SimpleBody*, Vector3Dd*)
    ROTATE_METHOD           rotateMethod;              // void(*)(SimpleBody*, Vector3Dd*)
    SCALE_METHOD            scaleMethod;               // void(*)(SimpleBody*, Vector3Dd*)
    INVERT_METHOD           invertMethod;              // void(*)(SimpleBody*)
};
```

### How it is used

1. **Every shape class holds a method table.** Each declares a
   `static Methods methodTable;` (some declare two — see inventory below) whose
   slots are filled with `static` functions of that class. Example
   (`Sphere.cpp`):

   ```cpp
   Methods Sphere::methodTable = {
       Sphere::allSphereIntersections, Sphere::insideSphere, Sphere::sphereNormal,
       Sphere::copySphere, Sphere::translateSphere, Sphere::rotateSphere,
       Sphere::scaleSphere, Sphere::invertSphere};
   ```

2. **Each object instance points at its table** via a `Methods *methods;`
   member, set by the builder/factory, e.g.
   `newShape->methods = &Sphere::methodTable;` (`ModelBuilder.cpp`).

3. **Dispatch goes through `GeometryOperations`**
   (`src/environment/geometry/GeometryOperations.h`), which dereferences the
   table:

   ```cpp
   static inline int allIntersections(SimpleBody *x, RayWithSegments *y,
                                      java::PriorityQueue<Intersection> *z)
   { return ((*((x)->methods->allIntersectionsMethod))(x, y, z)); }
   ```

4. **Type punning.** `Geometry`, `SimpleBody`, `Composite`, and `Camera` are
   **separate classes**, each beginning with `Methods *methods;` and sharing a
   layout-compatible prefix. Pointers are freely `(SimpleBody *)`-cast between
   them so the dispatcher can read `->methods`. Inside each static function the
   pointer is cast back to the real type, e.g. `Sphere *shape = (Sphere *)object;`.

This is exactly the behaviour we will re-express as real C++ virtual methods.

### Target design (what we are building)

The 8 `Methods` slots describe two *kinds* of things mixed together:

* Operations that only make sense for things placed in the **scene's geometry
  graph** (shapes, CSG nodes, composites): `allIntersections`, `inside`,
  `normal`.
* Operations that any **transformable scene element** supports, including
  things that are *not* geometry in the ray-tracing sense — most notably the
  `Camera`: `copy`, `translate`, `rotate`, `scale`, `invert`.

To avoid forcing non-geometric elements (the camera) into the `Geometry`
hierarchy just to get the transform operations, we introduce a new interface:

* **`TransformableElement`** — a new abstract class in
  `src/environment/TransformableElement.h`. It declares **all 8** operations
  (the full `Methods` slot list) as `virtual` methods with safe no-op/zero
  default implementations (mirroring the `nullptr` slots of today), plus a
  `virtual` destructor. This class is the **sole successor of `Methods`** — it
  is where "the structure of methods from `Methods`" is re-expressed as C++
  virtuals.
* **`Geometry : public TransformableElement`** — the base class for everything
  in the scene's geometry graph (shapes, `CSG`, `SimpleBody`, `Composite`,
  `Light` — `Light` already derives from `Geometry` today and keeps doing so,
  see note below). `Geometry` itself adds no new virtuals; it inherits the 8
  from `TransformableElement`.
* **`Camera : public TransformableElement`** — **directly**, *not* via
  `Geometry`. The camera is not part of the geometry graph (it has no
  `geometryType`/`nextObject`/`material`/`shapeColor`, and never did), but it
  does need `copy`/`translate`/`rotate`/`scale` (its `allIntersections`/
  `inside`/`normal`/`invert` slots were always `nullptr`, so it gets those from
  `TransformableElement`'s defaults).
* **`GeometryOperations`** keeps its 8 public static entry points, but their
  parameter type becomes `TransformableElement *` (the common base of both
  `Geometry` and `Camera`), and their bodies become virtual calls
  (`x->translate(y)` etc.). This lets the *same* dispatcher serve both shapes
  and the camera, exactly as it does today through `Methods`.
* The `Methods` class, all `*MethodTable` objects, all `Methods *methods`
  members, and `Methods.h` are **deleted** at the end. `TransformableElement.h`
  is the new, permanent file that replaces it.

> **About `Light`.** `Light` already derives from `Geometry` in the current
> codebase (`class Light : public Geometry`), and relies on several
> `Geometry`-inherited fields: `material`, `shapeColor`, `geometryType` (to
> distinguish `POINT_LIGHT_TYPE`/`SPOT_LIGHT_TYPE`), and `nextObject`. This plan
> **does not** change that inheritance edge — `Light` continues to derive from
> `Geometry`, and therefore transitively from `TransformableElement`, exactly as
> before. Re-parenting `Light` away from `Geometry` would require duplicating
> those four fields and touching the scene-graph traversal code for no benefit
> to this refactor's goal (replacing `Methods` with virtuals), so it is
> explicitly **out of scope**. If a future cleanup wants `Light` to stop being a
> `Geometry`, that should be its own separate plan.

### Signature mapping (Methods slot → virtual method on `TransformableElement`)

The self-pointer argument (`SimpleBody *object` / `SimpleBody *x`) **becomes
`this`** and is removed from the parameter list. Everything else is preserved
verbatim. **Do not change return types** (`copy` keeps returning `void *`).

| Methods slot (today)                                            | New virtual method on `TransformableElement`                                            |
|------------------------------------------------------------------|----------------------------------------------------------------------------------------|
| `int  (*)(SimpleBody*, RayWithSegments*, PriorityQueue<Intersection>*)` | `virtual int  allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)` |
| `int  (*)(Vector3Dd*, SimpleBody*)`                            | `virtual int  inside(Vector3Dd *point)`                                                 |
| `void (*)(Vector3Dd*, SimpleBody*, Vector3Dd*)`                | `virtual void normal(Vector3Dd *result, Vector3Dd *intersectionPoint)`                  |
| `void*(*)(SimpleBody*)`                                         | `virtual void *copy()`                                                                  |
| `void (*)(SimpleBody*, Vector3Dd*)`                            | `virtual void translate(Vector3Dd *vector)`                                             |
| `void (*)(SimpleBody*, Vector3Dd*)`                            | `virtual void rotate(Vector3Dd *vector)`                                                |
| `void (*)(SimpleBody*, Vector3Dd*)`                            | `virtual void scale(Vector3Dd *vector)`                                                 |
| `void (*)(SimpleBody*)`                                         | `virtual void invert()`                                                                 |

> Note the argument **order quirks** preserved from `Methods`: `inside` takes the
> point first; `normal` takes `result` first and `intersectionPoint` last.

### Inventory of dispatchable classes

| Class                 | Today derives from | New base class                | Tables it owns                          | Notes / special cases |
|-----------------------|--------------------|--------------------------------|-------------------------------------------|-----------------------|
| `Sphere`              | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `Box`                 | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `Quadric`             | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `InfinitePlane`       | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `Blob`                | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `HeightField`         | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `PolynomialShape`     | `Geometry`         | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `ParametricBiCubicPatch` | `Geometry`      | `Geometry` (unchanged)         | `methodTable`                           | full 8 |
| `Triangle`            | `Geometry`         | `Geometry` (unchanged)         | `methodTable`, `smoothMethodTable`      | see Triangle/SmoothTriangle note |
| `SmoothTriangle`      | `Geometry` (sibling)| `Triangle`                    | (uses `Triangle::smoothMethodTable`)   | see note |
| `Light`               | `Geometry`         | `Geometry` (unchanged — see note above) | `methodTable`                  | `normal` slot is `nullptr` |
| `CSG`                 | `Geometry`         | `Geometry` (unchanged)         | `unionMethodTable`, `intersectionMethodTable` | `normal` slot `nullptr`; **runtime table swap** in `invertCsg` |
| `SimpleBody`          | *(standalone)*     | `Geometry`                     | (uses `Composite::basicObjectMethodTable`) | |
| `Composite`           | *(standalone)*     | `SimpleBody`                   | `compositeMethodTable`, `basicObjectMethodTable` | |
| `Camera`              | *(standalone)*     | `TransformableElement` (**not** `Geometry`) | `methodTable`              | only `copy/translate/rotate/scale`; rest `nullptr` |

---

## 1. Global rules (apply to every phase)

1. **The gate.** After every phase, run, from the repository root, exactly:
   ```bash
   ./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh
   ```
   A phase is **only** complete when **all** of these hold:
   * `./scripts/compile.sh` finishes with **no errors and no warnings**
     (the build uses `-O3 -Wall -pedantic`; treat every warning as a failure).
   * `./scripts/testAgainstGoldenImages.sh` prints **`Test passed.`**
   * You **report to the user the wall-clock time in seconds** that
     `./scripts/renderAll.sh` took. That script prints a line ending with
     `Total execution time: <N> seconds.` — quote that number.
2. **Byte-identical output is the contract.** This is a pure refactor. The
   golden-image test must pass with zero differing images at every phase.
   If any image differs, you changed behaviour — revert and fix.
3. **Do not touch git.** No `git add`, no `git commit`, no branch operations.
4. **Stop & notify after each phase.** At the end of each phase, post a short
   summary to the user: what changed, the gate result, and the renderAll time
   in seconds. Then **wait** — the user reviews manually and commits if they
   approve. Do not start the next phase until told to proceed.
5. **Smallest viable diff.** Match the surrounding code style. Do not reformat
   unrelated code, do not rename things that the plan does not ask you to rename.

---

## 2. Phase 1 — Unify the class hierarchy (no dispatch change yet)

**Goal:** make `SimpleBody`, `Composite`, and `SmoothTriangle` part of the
`Geometry` inheritance hierarchy and remove the duplicated data members, **while
leaving the C-style `Methods` dispatch fully intact and working.** `Camera` is
**not** touched in this phase (it does not join the `Geometry` hierarchy — see
Phase 2). After this phase the program still dispatches through `->methods`; we
have only re-parented the classes so that a later phase can introduce
`TransformableElement` and have everything implement it.

This phase isolates the riskiest structural change (layout / inheritance / field
renames) from the dispatch change.

### 2.1 Re-parent the standalone classes

* `SimpleBody.h`: change `class SimpleBody {` → `class SimpleBody : public Geometry {`.
  * **Remove** the members now inherited from `Geometry`:
    `Methods *methods;`, `GeometryTypes type;`, `SimpleBody *nextObject;`.
  * Keep `boundingShapes`, `clippingShapes`, `geometry`, `noShadowFlag`,
    `objectColor`, `objectTexture`.
  * Remove the now-unnecessary `class Methods;` forward declaration only if it
    becomes unused; otherwise leave it.
* `Composite.h`: change `class Composite {` → `class Composite : public SimpleBody {`.
  * **Remove** the members now inherited (from `Geometry` via `SimpleBody`):
    `Methods *methods;`, `GeometryTypes type;`, `SimpleBody *nextObject;`,
    `Geometry *boundingShapes;`, `Geometry *clippingShapes;`.
  * Keep `SimpleBody *simpleBodies;` and the two static tables and the static
    method declarations (unchanged for now).
* `SmoothTriangle.h`: change `class SmoothTriangle : public Geometry {` →
  `class SmoothTriangle : public Triangle {`.
  * **Remove** every member it now inherits from `Triangle`:
    `normalVector`, `Distance`, `VPNormDotOrigin`, `VPCached`, `Dominant_Axis`,
    `Inverted`, `vAxis`, `P1`, `P2`, `P3`, `degenerateFlag`.
  * Keep only the smooth-specific members: `N1`, `N2`, `N3`, `Perp`, `BaseDelta`,
    and its static method declarations.
  * `SmoothTriangle.h` must now see the complete definition of `Triangle`, but
    `Triangle.h` currently includes `SmoothTriangle.h` (because `Triangle` has a
    private helper taking `SmoothTriangle *`). Resolve the include cycle with
    the standard "leapfrog" trick:
    1. In `Triangle.h`, forward-declare `class SmoothTriangle;` before the
       `Triangle` class definition (sufficient for the pointer parameter of
       `computeSmoothTriangle`).
    2. Move the `#include "environment/geometry/elements/SmoothTriangle.h"` line
       in `Triangle.h` to **after** the `Triangle` class definition (still before
       `#endif`).
    3. In `SmoothTriangle.h`, `#include "environment/geometry/elements/Triangle.h"`
       at the top (after its own include guard).
    4. Trace through both inclusion orders (whichever header is included first)
       and confirm via the include guards that `Triangle` is fully defined
       before `SmoothTriangle : public Triangle` is parsed, with no infinite
       recursion. Verify it compiles cleanly.

### 2.2 Rename the `type` field references

`Geometry` already has `GeometryTypes geometryType;`. The standalone classes
`SimpleBody`/`Composite` used `type`. After re-parenting, that name no longer
exists; rename the field references (these are the only relevant sites — the
`intervals[].type` in `Blob.cpp` is an unrelated `BlobInterval` field, **leave it
alone**; `Camera::Type` is untouched in this phase — see 2.4):

* `src/environment/scene/SimpleBodyFactory.cpp:32` `newObject->type` → `newObject->geometryType`
* `src/environment/scene/ModelBuilder.cpp:28` `newComposite->type` → `newComposite->geometryType`
* `src/environment/geometry/volume/compound/Composite.cpp:41` `newObject->type` → `newObject->geometryType`
* `src/io/pov/parser/ParseHelpers.cpp:40` `object->type` → `object->geometryType`
* `src/render/SceneDump.cpp:15` `obj->type` → `obj->geometryType`

### 2.3 Fix `nextObject` typing fallout

`Geometry::nextObject` is `Geometry *`. The removed `SimpleBody *nextObject` was
`SimpleBody *`. Linked lists of objects (`renderFrame().Objects`,
`Composite::simpleBodies`, etc.) contain `SimpleBody`/`Composite` instances, so
storing them in a `Geometry *` slot is fine, but **reading them back into a
`SimpleBody *` / `Composite *` local now needs an explicit cast.**

* Build will fail on statements like `localObject = localObject->nextObject;`
  where `localObject` is `SimpleBody *`. Fix each by wrapping the right-hand side:
  `localObject = static_cast<SimpleBody *>(localObject->nextObject);`
  (use `Composite *` where the local is `Composite *`). The object genuinely is
  that derived type, so the downcast is safe.
* Known sites to expect (not necessarily exhaustive — let the compiler guide you):
  `Composite.cpp` (the `simpleBodies` / `localObject` loops),
  `RenderEngine.cpp` (`object = object->nextObject`),
  `DirectLightShader.cpp` (`blockingObject = blockingObject->nextObject`),
  and any `ObjectParser.cpp` / `LightSourceParser.cpp` object-list walks.
* The **existing** `(SimpleBody *)` / `(SimpleBody **)` C-style casts in
  `Composite.cpp` / `CSG.cpp` (e.g. `(SimpleBody **)&(copiedShape->nextObject)`)
  keep compiling unchanged — leave them for now; they are cleaned up in Phase 3.

### 2.4 Leave dispatch and `Camera` untouched

* Do **not** modify `Methods.h` in this phase.
* Do **not** modify `Camera.h` / `Camera.cpp` in this phase — `Camera` keeps its
  own `Methods *methods;` and `GeometryTypes Type;` members, untouched, and is
  re-parented (to `TransformableElement`, not `Geometry`) only in Phase 2.
* All `*MethodTable` definitions and all `newX->methods = &...` assignments stay.

### 2.5 Gate & stop

Run the gate (Section 1). Confirm `Test passed.`, zero warnings, and report the
renderAll time in seconds. Then **STOP and notify the user.**

---

## 3. Phase 2 — Introduce `TransformableElement` and flip dispatch

**Goal:** create the `TransformableElement` interface, make `Geometry` and
`Camera` both implement it, give every subclass concrete overrides that
**delegate to the existing static functions**, and switch `GeometryOperations`
to call the virtual methods. The `Methods` tables still exist but become
**unused by the dispatcher** — proving the new dispatch path is behaviourally
identical before we delete anything.

### 3.1 Create `TransformableElement` and wire up the two base classes

1. **New file** `src/environment/TransformableElement.h`:

   ```cpp
   #ifndef __TRANSFORMABLE_ELEMENT_H__
   #define __TRANSFORMABLE_ELEMENT_H__

   #include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
   #include "java/util/PriorityQueue.h"
   #include "environment/geometry/Intersection.h"
   #include "environment/geometry/elements/RayWithSegments.h"

   class TransformableElement {
     public:
       virtual int   allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) { return 0; }
       virtual int   inside(Vector3Dd *point) { return 0; }
       virtual void  normal(Vector3Dd *result, Vector3Dd *intersectionPoint) {}
       virtual void *copy() { return nullptr; }
       virtual void  translate(Vector3Dd *vector) {}
       virtual void  rotate(Vector3Dd *vector) {}
       virtual void  scale(Vector3Dd *vector) {}
       virtual void  invert() {}
       virtual ~TransformableElement() = default;
   };

   #endif
   ```

   Check for include cycles between `TransformableElement.h`,
   `Intersection.h`, `RayWithSegments.h`, and `Geometry.h` (`Intersection.h`
   currently includes `Geometry.h`). Use forward declarations if needed so that
   `TransformableElement.h` does not need to see the full `Geometry` definition
   (it only needs `Vector3Dd`, `RayWithSegments`, the `Intersection` template
   argument, and `java::PriorityQueue`). If `Intersection.h` → `Geometry.h` →
   `TransformableElement.h` → `Intersection.h` would cycle, break it by having
   `TransformableElement.h` forward-declare `class Intersection;` and including
   only what `java::PriorityQueue<Intersection>` strictly requires (a forward
   declaration is sufficient for a template parameter used only as
   `PriorityQueue<Intersection> *`). Verify the chosen approach compiles cleanly.

2. **`Geometry.h`**: add `#include "environment/TransformableElement.h"` and
   change `class Geometry {` → `class Geometry : public TransformableElement {`.
   `Geometry` adds no new members in this step (the 8 operations are inherited).
   Leave `Methods *methods;` on `Geometry` for now — it is removed in Phase 3.

3. **`Camera.h`**: add `#include "environment/TransformableElement.h"` (or rely
   on the transitive include via `GeometryOperations.h` → `Geometry.h` →
   `TransformableElement.h`, but an explicit include is clearer) and change
   `class Camera {` → `class Camera : public TransformableElement {`.
   * **Remove** `Methods *methods;` from `Camera` (it is no longer needed once
     dispatch is virtual — but see the timing note below).
   * **Keep** `GeometryTypes Type;` exactly as-is — `Camera` is not a `Geometry`
     subclass, so this field is *not* a duplicate of anything and is not renamed.
   * Timing note: `Camera::initializeDefaults` currently does
     `this->methods = (Methods *)&Camera::methodTable;`. Since `methods` is
     removed from `Camera` in this step, also remove that line now (it would
     otherwise fail to compile). This is safe in this phase because by the end
     of 3.2 `Camera`'s virtual overrides are already in place and nothing reads
     `Camera::methods` any more (`GeometryOperations` is flipped to virtual calls
     in 3.3, within the same phase). Do steps 3.1–3.4 together before gating.

### 3.2 Add delegating overrides to every subclass

For each class in the inventory, add `override` declarations in the header and
define them in the `.cpp` to **call the existing static function, passing
`(SimpleBody *)this`** (for `Camera`, pass `(SimpleBody *)this` too — the static
camera functions already take `SimpleBody *` and only ever cast it back to
`Camera *`). Keep the static functions exactly as they are for now.

Recipe (Sphere shown; replicate for every class/operation it currently fills):

```cpp
// Sphere.h — inside class body:
int  allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
int  inside(Vector3Dd *point) override;
void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
void *copy() override;
void translate(Vector3Dd *vector) override;
void rotate(Vector3Dd *vector) override;
void scale(Vector3Dd *vector) override;
void invert() override;

// Sphere.cpp:
int  Sphere::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{ return allSphereIntersections((SimpleBody *)this, ray, depthQueue); }
int  Sphere::inside(Vector3Dd *point)            { return insideSphere(point, (SimpleBody *)this); }
void Sphere::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{ sphereNormal(result, (SimpleBody *)this, intersectionPoint); }
void *Sphere::copy()                             { return copySphere((SimpleBody *)this); }
void Sphere::translate(Vector3Dd *vector)        { translateSphere((SimpleBody *)this, vector); }
void Sphere::rotate(Vector3Dd *vector)           { rotateSphere((SimpleBody *)this, vector); }
void Sphere::scale(Vector3Dd *vector)            { scaleSphere((SimpleBody *)this, vector); }
void Sphere::invert()                            { invertSphere((SimpleBody *)this); }
```

Apply the same mechanical recipe to: `Box`, `Quadric`, `InfinitePlane`, `Blob`,
`HeightField`, `PolynomialShape`, `ParametricBiCubicPatch`, `Light`.

**Per-class overrides to add — only the operations whose table slot was non-null:**

* **`Light`**: all except `normal` (its `normalMethod` slot is `nullptr` — do not
  override `normal`; the `TransformableElement` default applies).
* **`SimpleBody`** (was the `basicObjectMethodTable` owner): override
  `allIntersections` → `Composite::allObjectIntersections`,
  `inside` → `Composite::insideBasicObject`,
  `copy` → `Composite::copyBasicObject`,
  `translate`→`Composite::translateBasicObject`,
  `rotate`→`Composite::rotateBasicObject`,
  `scale`→`Composite::scaleBasicObject`,
  `invert`→`Composite::invertBasicObject`.
  Do **not** override `normal` (slot was `nullptr`).
  Add these declarations/definitions on `SimpleBody` itself (you may keep the
  bodies as one-line delegations to the existing `Composite::...BasicObject`
  statics).
* **`Composite`** (was the `compositeMethodTable` owner): override
  `allIntersections`→`allCompositeIntersections`, `inside`→`insideCompositeObject`,
  `copy`→`copyCompositeObject`, `translate`→`translateCompositeObject`,
  `rotate`→`rotateCompositeObject`, `scale`→`scaleCompositeObject`,
  `invert`→`invertCompositeObject`. No `normal`.
* **`Camera`**: override only `copy`→`copyCamera`, `translate`→`translateCamera`,
  `rotate`→`rotateCamera`, `scale`→`scaleCamera`. The other four
  (`allIntersections`, `inside`, `normal`, `invert`) use the
  `TransformableElement` default (never called on a camera).
* **`Triangle`**: override all 8 → `allTriangleIntersections`, `insideTriangle`,
  `triangleNormal`, `copyTriangle`, `translateTriangle`, `rotateTriangle`,
  `scaleTriangle`, `invertTriangle`.
* **`SmoothTriangle`** (now `: public Triangle`): **inherits** `allIntersections`
  and `inside` from `Triangle` (this matches `smoothMethodTable`, which reused
  `Triangle::allTriangleIntersections` / `Triangle::insideTriangle`). Override
  only `normal`→`smoothTriangleNormal`, `copy`→`copySmoothTriangle`,
  `translate`→`translateSmoothTriangle`, `rotate`→`rotateSmoothTriangle`,
  `scale`→`scaleSmoothTriangle`, `invert`→`invertSmoothTriangle`.
* **`CSG` — the runtime-swap case.** Today a single `CSG` object switches between
  `unionMethodTable` and `intersectionMethodTable` (in `getCsgUnion` /
  `getCsgIntersection` and, at run time, inside `invertCsg`). Since one C++
  object has one fixed vtable, encode the union/intersection choice by
  **branching on `geometryType`** inside the overrides:

  ```cpp
  int CSG::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) {
      if (geometryType == GeometryTypes::CSG_INTERSECTION_TYPE)
          return allCsgIntersectIntersections((SimpleBody *)this, ray, depthQueue);
      return allCsgUnionIntersections((SimpleBody *)this, ray, depthQueue);
  }
  int CSG::inside(Vector3Dd *point) {
      if (geometryType == GeometryTypes::CSG_INTERSECTION_TYPE)
          return insideCsgIntersection(point, (SimpleBody *)this);
      return insideCsgUnion(point, (SimpleBody *)this);
  }
  void *CSG::copy()              { return copyCsg((SimpleBody *)this); }
  void CSG::translate(Vector3Dd *v) { translateCsg((SimpleBody *)this, v); }
  void CSG::rotate(Vector3Dd *v)    { rotateCsg((SimpleBody *)this, v); }
  void CSG::scale(Vector3Dd *v)     { scaleCsg((SimpleBody *)this, v); }
  void CSG::invert()             { invertCsg((SimpleBody *)this); }
  ```

  In `CSG::invertCsg` (still a static for now), the two lines that reassign
  `csg->methods = &CSG::unionMethodTable;` / `&CSG::intersectionMethodTable;`
  become **no-ops once `methods` is gone** — but in this phase `methods` still
  exists on `Geometry` (only `Camera` lost it), so **leave those two lines for
  now**; the `geometryType` flip it also performs is what the new
  `allIntersections`/`inside` branch on, so behaviour is already correct through
  the virtual path. (They are removed in Phase 3.)

### 3.3 Flip `GeometryOperations` to virtual dispatch

Rewrite the bodies in `GeometryOperations.h` to call the virtual methods, and
change the parameter types from `SimpleBody *` to `TransformableElement *` — the
common base that now carries the interface for **both** `Geometry`-rooted shapes
**and** `Camera`:

```cpp
static inline int  allIntersections(TransformableElement *x, RayWithSegments *y, java::PriorityQueue<Intersection> *z) { return x->allIntersections(y, z); }
static inline int  inside(Vector3Dd *x, TransformableElement *y)                 { return y->inside(x); }
static inline void normal(Vector3Dd *x, TransformableElement *y, Vector3Dd *z)   { y->normal(x, z); }
static inline void *copy(TransformableElement *x)                                { return x->copy(); }
static inline void translate(TransformableElement *x, Vector3Dd *y)              { x->translate(y); }
static inline void rotate(TransformableElement *x, Vector3Dd *y)                 { x->rotate(y); }
static inline void scale(TransformableElement *x, Vector3Dd *y)                  { x->scale(y); }
static inline void invert(TransformableElement *x)                               { x->invert(); }
```

* Keep `GeometryOperations::intersect(...)` but change its parameter to
  `TransformableElement *x` (it already only forwards to `allIntersections`). Its
  body is otherwise unchanged. (In practice `intersect`/`allIntersections`/
  `inside`/`normal` are only ever called with `Geometry`-rooted arguments, but
  typing the parameter as `TransformableElement *` keeps all 8 entry points
  uniform and matches `Methods`' original "one table shape for everything"
  contract.)
* `GeometryOperations.h` must `#include "environment/TransformableElement.h"`.
  It no longer needs `#include ".../Methods.h"` for these bodies, but **do not
  delete the include yet** if other code in the header still needs it; remove
  dead includes in Phase 4.

### 3.4 Remove now-redundant casts at call sites

Because the entry points now take `TransformableElement *`, the `(SimpleBody *)`
casts that were only there to satisfy the old signature become unnecessary.
**Remove** them at the ~90 call sites — any `TransformableElement`-derived
pointer (`Geometry *`, `Sphere *`, `Composite *`, `Camera *`, etc.) converts
implicitly. These are in:

* `src/render/RenderEngine.cpp`, `src/render/RayShaderPipeline.cpp`,
  `src/render/shaders/DirectLightShader.cpp`,
  `src/render/shaders/LocalSurfaceShader.cpp`
* all of `src/io/pov/**` parsers (`ObjectParser.cpp` has the bulk, plus
  `Box/Sphere/Quadric/Poly/Plane/Triangle/SmoothTriangle/Blob/HeightField/BicubicPatch`
  parsers, `CameraParser.cpp`, `LightSourceParser.cpp`)
* the internal recursive calls inside `Composite.cpp` and `CSG.cpp`
  (e.g. `GeometryOperations::translate((SimpleBody *)localShape, vector)` →
  `GeometryOperations::translate(localShape, vector)`).

> Mechanical rule: wherever an argument to a `GeometryOperations::` call is
> written `(SimpleBody *)expr`, replace it with `expr`. This applies equally to
> `CameraParser.cpp`'s `GeometryOperations::translate/rotate/scale((SimpleBody
> *)givenVp, ...)` calls — `givenVp` is `Camera *`, which now converts directly
> to `TransformableElement *`. Leave `copy`'s **return** casts (`(Sphere
> *)GeometryOperations::copy(...)`) alone — `copy()` still returns `void *`.

### 3.5 Gate & stop

Run the gate. Confirm `Test passed.`, zero warnings, report renderAll seconds.
**STOP and notify the user.**

---

## 4. Phase 3 — Inline the bodies and delete `Methods`

**Goal:** move each static function's body into its virtual method, delete the
static functions, the method tables, the `methods` member (now only on
`Geometry`, since `Camera` lost it in Phase 2), and `Methods.h`.

Do this **one class at a time** to keep diffs reviewable, but the gate is run
once at the end of the phase (it cannot pass mid-class because the tables
reference the statics). If you prefer extra safety, you may run the gate after
each class — but a class is only independently buildable once both its statics
and its table entry are gone together.

### 4.1 For each dispatchable class

1. **Move the body** of each static function into the corresponding virtual
   method. Replace the self-parameter with `this`:
   * Delete the local re-cast line (`Sphere *shape = (Sphere *)object;`) and use
     members directly, **or** replace it with `Sphere *shape = this;` to minimise
     the diff. Replace remaining `object` / `x` references with `this`/members.
   * Remove the now-pointless `(SimpleBody *)` casts that were only bridging the
     old signature.
   * Internal recursive dispatch calls already went through `GeometryOperations`
     in Phase 2 — leave those as `GeometryOperations::...(geometryPtr, ...)`.
2. **Delete the static function declarations** (headers) and **definitions**
   (`.cpp`) once their bodies have moved. Keep genuinely-private helpers that are
   not Methods slots (e.g. `Sphere::intersectSphere`, `Triangle::computeTriangle`,
   `Triangle::findTriangleDominantAxis`, `Light::attenuateLight`,
   `Light::cubicSpline`, `Camera::initializeDefaults`, `Composite::createBasicObject`,
   the `link*` helpers) — but update their signatures/bodies as needed (see 4.2).
3. **Delete the table object(s)**: the `static Methods methodTable;` (and
   `smoothMethodTable`, `unionMethodTable`, `intersectionMethodTable`,
   `compositeMethodTable`, `basicObjectMethodTable`, and `Camera::methodTable`)
   declarations in the headers and their definitions in the `.cpp`s.

### 4.2 Remove the `methods` member and its assignments

* Delete `Methods *methods;` from `Geometry.h`. (`Camera` already lost its copy
  in Phase 2.)
* Delete every `newX->methods = &...;` assignment in `ModelBuilder.cpp`,
  `SimpleBodyFactory.cpp`, and `Composite.cpp` (`createBasicObject`). (Object
  identity now comes from the C++ type used with `new`, which is already correct
  in every builder.)
* In `CSG`:
  * `getCsgUnion` / `getCsgIntersection` (`ModelBuilder.cpp`): drop the
    `newShape->methods = ...` lines; **keep** the `geometryType` assignment — that
    is what the virtual `allIntersections`/`inside` now branch on.
  * `CSG::invert` (formerly `invertCsg`): delete the two
    `csg->methods = &CSG::...MethodTable;` lines; **keep** the `geometryType`
    flip between `CSG_UNION_TYPE` and `CSG_INTERSECTION_TYPE`.
  * `CSG::copy` (formerly `copyCsg`): delete the `newShape->methods = shape->methods;`
    line; **keep** `newShape->geometryType = shape->geometryType;`. (A freshly
    `new CSG` already has the correct vtable; `geometryType` carries the
    union/intersection distinction.)
* Clean up the leftover punning casts now that everything is one hierarchy:
  the `(SimpleBody *)` / `(SimpleBody **)` casts in the `link*` helpers and copy
  routines (`Composite.cpp`, `CSG.cpp`). Where a list is `Geometry *`-typed,
  retype the helper parameters to `Geometry *` / `Geometry **`; where it is
  `SimpleBody *`-typed (e.g. `Composite::simpleBodies`), keep `SimpleBody`.
  Use `static_cast` (not C-style casts) for any remaining required up/down casts.
  The build with `-Wall -pedantic` must stay clean.

### 4.3 Delete `Methods` itself

* Delete the file `src/environment/geometry/Methods.h`.
* Remove every `#include "environment/geometry/Methods.h"` and every
  `class Methods;` forward declaration (`Geometry.h`, `SimpleBody.h`,
  `GeometryOperations.h`, and any shape headers/`.cpp`s that referenced it).
* Confirm there are **zero** remaining references:
  ```bash
  grep -rn "Methods\b\|methodTable\|->methods\|\.methods" src --include=*.h --include=*.cpp
  ```
  (The only legitimate remaining hits are the unrelated
  `ImageToSolidTextureProjectionMethods` in `TextureParser.cpp` — leave those.)

### 4.4 Gate & stop

Run the gate. Confirm `Test passed.`, zero warnings, report renderAll seconds.
**STOP and notify the user.**

---

## 5. Phase 4 — Final cleanup & verification

**Goal:** remove dead includes/declarations and do a final confirmation pass.

1. Remove now-unused `#include`s introduced or orphaned during the migration
   (e.g. headers that were only needed for `Methods`). Do not remove includes
   that are still required — let the clean build confirm.
2. Re-run the no-reference grep from 4.3 to confirm `Methods` is fully gone.
3. Confirm `GeometryOperations` is now a thin façade of virtual-call forwarders
   over `TransformableElement *` (plus the unchanged `intersect` helper). This is
   the intended end state — `GeometryOperations` itself is **not** deleted (only
   `Methods` is). `TransformableElement.h` is a new permanent file and is also
   not deleted.
4. Run the full gate one last time. Report:
   * compile: no errors / no warnings,
   * `Test passed.`,
   * the final `renderAll.sh` time in seconds (and note it against the Phase 1
     baseline so the user can see the refactor did not regress performance).
5. **STOP and notify the user** with the final summary.

---

## 6. Acceptance checklist (the definition of done)

* [ ] `Methods` class, `Methods.h`, all `*MethodTable` objects, and every
      `Methods *methods` member are deleted.
* [ ] `src/environment/TransformableElement.h` exists, declaring the 8 operations
      from the mapping table as `virtual` methods with safe defaults, plus a
      `virtual` destructor.
* [ ] `Geometry : public TransformableElement` and `Camera : public
      TransformableElement` (directly — `Camera` does **not** derive from
      `Geometry`).
* [ ] `Light` continues to derive from `Geometry` (unchanged from today), and
      therefore transitively implements `TransformableElement`.
* [ ] Every dispatchable subclass provides concrete overrides for the operations
      it used to register; `nullptr` slots map to the inherited
      `TransformableElement` default.
* [ ] `SimpleBody : public Geometry`, `Composite : public SimpleBody`,
      `SmoothTriangle : public Triangle`.
* [ ] `GeometryOperations` dispatches via virtual calls on `TransformableElement *`.
* [ ] `./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh`
      builds with **no warnings/errors** and prints **`Test passed.`**
* [ ] The `renderAll.sh` time in seconds was reported to the user at every phase.
* [ ] No git operations were performed by the implementer.
