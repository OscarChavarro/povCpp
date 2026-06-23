# Plan: CSG by ray-segment classification (ROTH 1982)

Reference: `doc/references/[ROTH1982]_RayCastingForModelingSolids.pdf`
(Scott D. Roth, *Ray Casting for Modeling Solids*, Computer Graphics and Image
Processing 18, 109–144, 1982).

Sibling document (architectural comparison povCpp↔VITRAL, not a plan):
`doc/vitralNormalizationAnalysis.md`, §4 ("How povCpp does CSG today").

---

## 0. Goal and contract

Extend the raytracer so that **CSG classification can be done with two
selectable algorithms**:

1. **No flags (current behavior):** CSG by **point membership**
   (`CSG::allCsgIntersectIntersections` + `invert()`). **Byte-identical result to
   today's.** Mandatory gate:

   ```
   ./scripts/clean.sh; ./scripts/compile.sh; ./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh   # => "Test passed."
   ```

2. **With `-csgRoth`:** **only** the CSG parts change behavior, using **boolean
   ray-segment classification** in the style of Roth 1982 (In/Out segments + 1D
   interval algebra). Everything else (camera, shading, textures, refraction,
   non-CSG primitives) identical.

New gate script for the Roth path:

```
./scripts/renderCsgRoth.sh    # renders with -csgRoth
```

VITRAL is **not touched** in this plan. The whole new family of ray
classification operations lives in povCpp.

---

## Design decisions (settled)

| # | Decision | Choice |
|---|----------|--------|
| Q1 | Selection mechanism | **Class family** `CsgRoth : CSG`, chosen in `CsgParser` according to the flag (not a runtime `if`). |
| Q2 | Difference (`difference`) | **First-class operator** in the Roth branch (the Roth tree does not use `invert()`; explicit interval subtraction). |
| Q3 | Non-closed primitives in CSG | **Half-space**: a plane/open surface defines an In/Out boundary (classic POV semantics). |
| Q4 | Acceptance criterion for `-csgRoth` | **Default golden with RMSE tolerance** (Roth is not byte-identical at boolean edges). |
| Q5 | Scope of `renderCsgRoth.sh` | **All** scenes (confirms the non-CSG ones stay identical with the flag). |
| Q6 | Output model of the Roth branch | **Internal segments** materialized inside the CSG, reprojected onto the current `IntersectionCandidate` `depthQueue`. |

---

## 1. Algorithm selection: class family in the parser (Q1)

The flag must be known **at parse time** so that `CsgParser` instantiates the
correct class. The route:

1. **Flag parsing.** In `CommandLineOptions::parseOption`, recognize `csgRoth`
   (multi-character, just like `parallel`, checked before the single-letter
   switch so it doesn't collide with `-c`). It enables
   `RenderingConfiguration::CSG_ROTH = 2048u`.
2. **Propagation to the parser without coupling `environment`.** `ParserContext`
   deliberately avoids depending on `RenderingConfiguration` (see its
   `setDiagnostics`). Add a **`bool mCsgRoth`** with
   `setCsgRoth(bool)/usesCsgRoth()`, populated from `config` where
   `setDiagnostics(...)` is done today.
3. **Polymorphic instantiation.** `CsgParser::parse(...)` builds `CsgRoth` when
   `ctx.usesCsgRoth()`, and `CSG` otherwise. Since difference is treated as
   first-class (Q2), under Roth the parser **does not call `invert()`** on the
   children of a `difference`: it passes the `BooleanSetOperations` (incl.
   `DIFFERENCE`) as-is to `CsgRoth`.

**Advantage:** with `-csgRoth` absent, `CsgRoth` is never instantiated ⇒ the
default path is **byte-identical and has zero runtime cost** (not even a bit read
per ray). The scene tree differs by mode, but only when the flag is present.

`CsgRoth` inherits from `CSG` and **overrides** `allIntersections` (and the
classification helpers); it reuses the parent's child management, transforms, and
`copy()`.

---

## 2. Roth's algorithm (what to build)

Roth's ray-solid evaluator returns, per solid, **an ordered list of crossings
with In/Out parity**:

```
t[1], t[2], ..., t[n]      (enters at t[1], exits at t[2], enters at t[3], ...)
S[1], S[2], ..., S[n]      (surface pointer per crossing, for normal/material)
```

The `t[i]` alternate In/Out and define **segments** `[t[1],t[2]]`,
`[t[3],t[4]]`, … "inside the solid". Combination is **1D boolean interval
algebra**:

- **Union `+`**: inside if inside L **or** R → merges intervals.
- **Intersection `&`**: inside if inside L **and** R → intersects intervals.
- **Difference `-`**: inside if inside L **and not** R → subtracts intervals
  (first-class operator, Q2).

A merge pass over the ordered `t` values with an In/Out parity state machine per
subtree. nearest-hit = first `tIn` of the result.

### 2.1. Data structures (internal to the Roth branch, Q6)

```cpp
// A crossing of the ray with a surface, with the side (entering/exiting the solid).
struct RaySegmentCrossing {
    double t;
    bool   entering;             // true = In, false = Out
    IntersectionCandidate hit;   // carries point/normal + attributes (Roth's S[i])
};
using RaySegmentList = java::ArrayList<RaySegmentCrossing>;
```

The `+ & -` operators merge `RaySegmentList`s. The final output is
**reprojected onto the `IntersectionCandidate` `depthQueue`** that the rest of
the raytracer already consumes: only the crossings that survive the interval
algebra are `offer()`ed to the queue. **Blast radius contained in `CsgRoth`** —
no leaf solid nor the `RenderEngine` changes its interface.

### 2.2. Half-space semantics for open primitives (Q3)

A non-closed primitive (`InfinitePlane`, `Triangle`/`SmoothTriangle`, parametric
patches, open `HeightField`) does **not** produce entry/exit pairs. Under Roth it
is treated as a **half-space**: the crossing defines the boundary between In and
Out (the "inside" side according to the POV normal/orientation), generating a
segment `[tCrossing, +∞)` (or `(-∞, tCrossing]`) that the interval algebra clips
against the sibling closed solids. This is POV-Ray's classic semantics for planes
in booleans. Parity is derived from the direction of the normal at the crossing,
already available in the `IntersectionCandidate`.

---

## 3. Plan steps (each step keeps the default gate green)

> Golden rule: with `-csgRoth` **absent**, byte-identical golden and identical
> performance (guaranteed by Q1: `CsgRoth` is not even instantiated). Validate
> the full gate after each step.

### Step 1 — Flag infrastructure + empty class (no observable change)
- `RenderingConfiguration::CSG_ROTH = 2048u` + getters.
- Parse `csgRoth` in `CommandLineOptions::parseOption` (`parallel` pattern).
- `ParserContext::setCsgRoth/usesCsgRoth`; populate it from `config` alongside
  `setDiagnostics`.
- `CsgRoth : CSG` with an `allIntersections` that, for now, **delegates to
  `CSG::allIntersections`** (stub) → identical with and without the flag.
- `CsgParser` instantiates `CsgRoth` under the flag (and without `invert()` for
  `difference`, leaving `DIFFERENCE` first-class — but since it still delegates,
  the result may differ; that is why this step validates only "no crash" of the
  Roth path, not RMSE).
- Create `./scripts/renderCsgRoth.sh` (see §5).
- **Gate:** green by default; `renderCsgRoth.sh` runs without crashing.

### Step 2 — Materialize `RaySegmentList` per leaf solid
- In `CsgRoth`, build a `RaySegmentList` from the `depthQueue` each leaf child
  already emits: order by `t`, tag alternating parity for closed convex shapes
  (`Sphere`, `Box`, closed `Quadric`); apply half-space (Q3) for open ones.
- Cover closed non-convex/polynomial shapes (`PolynomialShape`, `Blob`) that emit
  N crossings, validating consistent parity.
- **Gate:** default intact.

### Step 3 — Interval algebra for UNION / INTERSECTION
- `mergeUnion`, `mergeIntersection` over `RaySegmentList` (single pass, per-subtree
  parity state machine).
- `CsgRoth::allIntersections` combines the children's lists according to
  `geometryType` and reprojects onto the `depthQueue`.
- **Gate:** default intact; compare `-csgRoth` vs golden with RMSE (Q4) on
  union/intersection scenes.

### Step 4 — First-class difference (interval subtraction, Q2)
- `mergeDifference` (L and-not R). The parser already delivered `DIFFERENCE`
  uninverted under the flag (Step 1), so `CsgRoth` subtracts intervals directly.
- **Gate:** default intact; validate `difference` scenes under `-csgRoth` with
  RMSE.

### Step 5 — (Optional) nested refraction from segments
- The In/Out segments **are** the "inside the solid" intervals: connect them to
  `containingTextures[]`/`containingIORs[]` of `RayWithSegments`. If deferred,
  document that under `-csgRoth` the media stack stays as today.
- **Gate:** default intact; review `iortest`, `glass*`, `magglass`.

### Step 6 — Cleanup and equivalence table
- Document `RaySegmentCrossing`/`RaySegmentList` ↔ Roth's `RAYCAST` output;
  `S[i]` ↔ `IntersectionAttributes::hitGeometry/material`.

---

## 4. Affected geometries (classification family)

CSG operates on leaf children (via `SimpleBody`):

- **Closed convex (direct):** `Sphere`, `Box`, `Quadric` (closed) → direct
  alternating parity.
- **Closed non-convex / polynomial:** `PolynomialShape`, `Blob` → N crossings,
  alternating parity if the surface is closed.
- **Non-closed (half-space, Q3):** `InfinitePlane`, `Triangle`, `SmoothTriangle`,
  parametric patches, `HeightField`.

---

## 5. `./scripts/renderCsgRoth.sh` and acceptance (Q4, Q5)

- Modeled on `renderAll.sh`, adding `-csgRoth`. **Renders all scenes** (Q5): the
  non-CSG ones must stay **byte-identical** (RMSE 0) — this proves the flag did
  not leak outside the CSG; the CSG ones are compared with **RMSE tolerance**.
- **Output and comparison:** write to a separate directory (e.g.
  `output-csgroth/`) and compare against the **same default golden** with an RMSE
  comparator (define a threshold, e.g. `RMSE < 0.01`). Reuses the existing
  golden; no new references to curate.
- A tolerance-aware comparator is needed (extend/parameterize
  `testAgainstGoldenImages.sh`, or a `testCsgRoth.sh` that invokes the RMSE
  metric already used in earlier project investigations).
- **CSG scenes** to fix with
  `grep -rl "union\|intersection\|difference" etc/`; mostly level 2/3
  (`dfwood`, `wtorus`, `crystal`, `skyvase`, `magglass`, …).

---

## 6. Risks

- **Byte-identity impossible under `-csgRoth`:** Roth derives In/Out by parity,
  not by `doContainmentTest` with `SMALL_TOLERANCE`; boolean edges will not be
  byte-identical (hence Q4 = RMSE).
- **Half-space (Q3):** the normal orientation of planes/triangles defines the
  "inside" side; validate against scenes with planes in booleans so the
  half-space is not inverted.
- **Flag leaking outside the CSG:** Q5 covers it — the non-CSG ones must give
  RMSE 0; any difference reveals an improper coupling.
- **First-class difference (Q2):** the parser stops inverting under the flag;
  verify that the Roth tree receives `DIFFERENCE` with the correct child order
  (the first child is the minuend).
- **Nested refraction:** if deferred (Step 5), leave the media stack as today
  under `-csgRoth` and document it.

---

## 7. Open implementation notes (minor)

- **Exact RMSE threshold** for `testCsgRoth`: fix it with the first real run
  (start loose, tighten as the algorithm matures).
- **RMSE comparator**: confirm whether we reuse the metric from earlier
  investigations (mentioned in earlier-phase memories) or add it to the script.
- **`copy()` of `CsgRoth`**: must return `CsgRoth` (not `CSG`) so scene copies
  preserve the algorithm; review `CSG`'s copy pattern.

---

## 8. Bug-fixing pass (see `doc/CSGFixesPlan.md`)

Two bugs flagged from the first full 108-scene visual triage were root-caused
and fixed in `CSGByRaySegment::buildRaySegments`, both in
`src/environment/geometry/volume/compound/CSGByRaySegment.cpp`:

- **Bug A** (`level1/cantelop`, hollow bowls rendering solid): per-crossing
  in/out parity was derived from the raw surface normal
  (`dir.normal < 0`), which does not consult a leaf's `inverse` flag
  (`Sphere::normal()`/`PolynomialShape::normal()` always return the
  unflipped geometric normal, unlike `doContainmentTest()`, which does flip).
  An `inverse`-flagged leaf used as an `intersection`/`union` operand got
  reversed parity, corrupting the merge.
- **Bug B** (`math/trough`, a chunk of the clip box's "cap" missing): the same
  family of bug, on a continuation/transparency ray. A secondary ray spawned
  from a point sitting exactly on a child's own surface (e.g. continuing past
  a `texture { color Clear }` clipping plane) hit the "no crossings" fallback,
  which sampled containment with a **negative** tolerance
  (`-Config::SMALL_TOLERANCE`) at a point offset by `2 * SMALL_TOLERANCE`
  along the ray. Combining a negative tolerance with an offset of the same
  order distorts the test into requiring `dir.normal <= -0.5`, misclassifying
  the child as OUTSIDE for any but a near-head-on approach.

**Fix, in two parts:**
1. Parity is no longer sampled independently per crossing. Only ONE
   `doContainmentTest()` sample is taken per child - the state just before
   the first crossing (or along the whole ray, if there are none) - and every
   later crossing simply **toggles** that state. This matches
   [ROTH1982].3.3 directly ("a ray alternates in/out at each successive
   crossing") and removes the fragility of re-sampling near every crossing
   (which broke down whenever two crossings ended up closer together than the
   sampling epsilon, e.g. near a tangential quartic crossing or where two
   cutting planes meet).
2. Both sampling sites (the first-crossing case and the no-crossings
   fallback) use **zero** tolerance, not negative. The crossing/offset point
   is already confidently off the surface by construction; a negative
   tolerance of the same magnitude as the offset was the source of the
   systematic misclassification in both Bug A and Bug B.

**Verified:** `level1/cantelop` RMSE 0.055 -> 0.0056; `math/trough` RMSE
0.048 -> 0.0053. Side effects (not separately investigated, just observed):
`level2/room`, `level2/esp01`, `level2/pacman`, `level1/ballbox1`,
`level3/snack` all dropped well under the 0.02 threshold; `level2/skyvase`
improved (0.168 -> 0.117) but still fails - it is missing reflections of the
vase on a separate reflective surface elsewhere in the scene, which looks
like a distinct, not-yet-diagnosed bug in how secondary reflection rays
interact with `CSGByRaySegment`. `level2/pawns` regressed slightly in degree
(0.034 -> 0.050) from the toggle-based parity change but was already failing
the gate before this fix (not a pass-to-fail regression); its CSG tree uses
`difference` over `sturm`-flagged quartics and was not investigated further.
`level2/iortest`, `level3/pool`, `level3/teapot`, `level3/wg5` also still fail
the gate, unchanged or mildly improved, and are unrelated pre-existing bugs.
`level1/blob`, `math/bezier`, `math/bezier0` remain in the accepted
numeric-noise category (see `doc/CSGFixesPlan.md` §1.1).
