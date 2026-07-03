# Performance Review Plan 5: Parse-Time Transform Recording and Scene-Owned Coefficient Baking

## Position in the Plan Sequence

This is the first of six sequential plans (Plans 5–10) derived from the
diagnostic conclusions of `doc/performanceReviewPlan4.md`. The sequence is:

| Plan | Subject | Depends on |
| --- | --- | --- |
| **5 (this one)** | Elementary transform recording + baked coefficient copies | — |
| 6 | Full rebuild of `render/bakedScene` around the truly baked model | 5 |
| 7 | `render/raySharedCache`: per-ray shared aggregate caches | 5, 6 |
| 8 | Fused CSG execution kernels (collapse the interpreter) | 5, 6, 7 |
| 9 | Candidate staging and traversal scaffolding cleanup | 8 |
| 10 | Consolidation: full-suite re-profile and baseline comparison | 5–9 |

Plan 5 runs first because it is the numeric foundation: every later plan
assumes that transformed quadric/sphere/plane operands have been replaced by
world-space baked copies. It is also the riskiest step numerically (the
composed-matrix variant of this idea is a recorded dead end), so it must be
proven in isolation, inside the existing baked structures, before the
structural rebuild of Plan 6 begins.

## Reference Commits and Benchmarks

- Older baked baseline: `4af1a75e5b7356600bec34e12a4882560994a058`
- Current scene-owned baked branch: `main` at the start of this plan
  (successor of `0d2a21cdfec10d3b3e7d41b53bba6e08333a08be`)

Primary benchmark (unchanged from Plans 1–4):

```bash
cd etc/level3/drums2
/usr/bin/time -f "TIME %e" ../../../build/povray \
    +l../../include +idrums.pov +o../../../output/drums_plan5.tga \
    +w320 +h200 -d -v +x +ft
```

At least three runs per measurement point. Current state: drums ≈ 8.55 s vs
baseline ≈ 5.10 s (1.68×).

Secondary benchmark panel:

```bash
./scripts/benchmarkPanel.sh
```

(`level2/spline`, `level3/ntreal`, `level3/piece3`, `level2/iortest`,
`level1/shapes2` at 320×200.)

`gprof` recipe for phases that claim a hot-counter reduction:

```bash
cmake -S . -B build-gprof -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-pg" -DCMAKE_EXE_LINKER_FLAGS="-pg"
cmake --build build-gprof --target povray -j"$(nproc)"
cd etc/level3/drums2
../../../build-gprof/povray +l../../include +idrums.pov \
    +o../../../output/drums_plan5_gprof.tga +w320 +h200 -d -v +x +ft
gprof ../../../build-gprof/povray gmon.out > ../../../output/drums_plan5_gprof.txt
```

Rebuild the normal Release binary afterwards with `./scripts/compile.sh`.

## Motivation (from Plan 4, Conclusions §1 and §6.1)

The baseline's baking was **destructive coefficient rewriting at parse time**:
`Quadric::transformQuadric` (baseline `Quadric.cpp:276`) folded every
`translate` / `rotate` / `scale` keyword into the 10 quadric coefficients via
the congruence `M' = T⁻¹ M T⁻ᵀ`, applied **incrementally, one elementary
transform at a time**. After parsing, "transformed operand" did not exist:
every primitive was a world-space primitive, and the render loop paid ~5 dot
products per (ray × quadric).

The current branch bakes only routing metadata. The transform math runs per
ray: `traceOperandAllCrossings` performs forward point/direction transforms,
back-transforms every hit, and recomputes `t` — at 244.5 M calls across the
sweep. Per transformed-quadric test the cost went from ~25 FLOPs to ~110+.

This plan builds the **non-destructive equivalent**: a *newly created
coefficient rewriting at parse/bake time*. The parsed scene stays intact
(the `io` → `environment/scene` model is unchanged and remains the full
model); the baked layer holds a **baked copy** of each transformed
quadric/sphere/plane whose coefficients were rewritten with the exact
baseline formulas and the exact baseline operation order. The math is
actually baked — it never re-executes per ray.

### Why the previous attempt failed and why this one is different

The recorded dead end ("quadric coeff-baking breaks byte-exact", Plan 2 era)
applied the **composed** `Matrix4x4d` in one congruence step. Floating-point
rounding of one composed application differs from the baseline's sequence of
per-keyword applications. The fix is numeric, not architectural: **replay the
same elementary transform sequence**, one congruence per translate / rotate /
scale, using formulas transcribed verbatim from the baseline. That requires
the parser to record the elementary steps, which is exactly Phase 1.

## Architectural Constraints

- `environment/geometry` stays as it is now: primitive math and
  geometry-local intersection routines. **No transform baking, no mutable
  render state, no new baking hooks are added to `Geometry` classes.**
- `environment/scene` remains the parsed scene owner. It gains only passive
  parse-time data: the recorded elementary transform steps.
- The baked coefficient copies and the replay engine live in
  `render/bakedScene` (the layer Plan 6 will rebuild; the replay engine
  written here carries over into the rebuild unchanged).
- The parsed model keeps everything the `io` layer reads today. Baking is a
  side-car copy by design; the RAM cost of storing transformed quadrics twice
  is accepted (a 10-coefficient quadric copy is far smaller than the two
  `Matrix4x4d` it replaces in the hot path).

## Phase 0 — Extract Baseline Transform Semantics (evidence, no code)

Before writing any code, transcribe the exact baseline math into a reference
section of this document:

```bash
git show 4af1a75:src/environment/geometry/volume/Quadric.cpp   # transformQuadric, quadricToMatrix, matrixToQuadric, translate/rotate/scaleGeometry, invertGeometry
git show 4af1a75:src/environment/geometry/volume/Quadric.h
git show 4af1a75:src/environment/geometry/volume/Sphere.cpp    # sphere translate/rotate/scale semantics, degradation rules if any
git show 4af1a75:src/environment/geometry/surface/InfinitePlane.cpp 2>/dev/null \
  || git show 4af1a75 --name-only | grep -i plane                # locate the baseline plane file first
```

Record, per primitive kind:

1. Which elementary transforms rewrite coefficients in place
   (baseline behavior), and the exact formula and operation order used.
   Key facts already verified: baseline `translateGeometry` builds
   `translation(-v).transpose()`, `rotateGeometry` uses
   `axisRotationRodrigues` (inverse out-parameter), `scaleGeometry` builds
   `scale(1/x, 1/y, 1/z)`; each then calls `transformQuadric(shape, Tinv)`
   which computes `Tinv · M · Tinvᵀ` via `quadricToMatrix` /
   `matrixToQuadric` and re-derives the square-term flag.
2. Whether any kind degrades under a transform (e.g., whether baseline
   spheres stay spheres under all transforms or become quadrics under
   rotation/non-uniform scale). Whatever the baseline did, the baked copy
   must do the same. **Dead-end reminder:** Phase H (Plan 3) proved
   `intersectBakedQuadric` is *not* numerically interchangeable with
   `intersectSphere`; a sphere must never be silently promoted to a baked
   quadric unless the baseline itself did that promotion for the same
   transform sequence.
3. What `invert` did to each kind (baseline `invertGeometry` negates all
   quadric terms).

Deliverable: a "Baseline Transform Semantics" appendix in this file, with the
formulas inlined, so later phases never need to consult the baseline commit
again.

## Phase 1 — Record Elementary Transform Steps at Parse Time

Currently only composed matrices survive parsing
(`SimpleBody::transformation` / `transformationInverse`,
`Scene::BakedCsgOperand::objectToLocal/localToObject` at `Scene.h:110-111`).
The elementary sequence is lost inside `SimpleBodyBuilder::translate/rotate/
scale` (`src/io/pov/geometry/SimpleBodyBuilder.h:39-44`) and the analogous
CSG-operand and geometry-level paths.

Steps:

1. Add a small passive value type, e.g.
   `src/environment/scene/TransformStep.h`:

   ```cpp
   struct TransformStep {
       enum class Kind { Translate, Rotate, Scale, Invert };
       Kind kind;
       Vector3Dd vector;   // unused for Invert
   };
   ```

   It lives in `environment/scene` because it is parsed-scene data (what the
   `.pov` file said), not render policy.

2. `SimpleBodyBuilder::translate/rotate/scale/...OwnerOnly` append a step to
   a `java::ArrayList<TransformStep>` alongside the existing matrix updates.
   The list is released into `SimpleBody` next to
   `transformation`/`transformationInverse`.

3. Trace every call path that composes transforms into a `CsgOperand` or a
   nested CSG child (CSG parsing in `src/io/pov/geometry/CsgParser.cpp`,
   `ObjectParser.cpp`, and the `SimpleBody` propagation methods noted in
   `SimpleBody.h:56` and `SimpleBody.h:93-94`) and record steps on the same
   objects the matrices land on. Nested composites propagate parent steps by
   **appending the parent's steps after the child's own**, in the same order
   the matrix composition applies them — verify the order against the matrix
   code, do not assume.

4. Also record steps for the object-level (`SimpleBody`) transform of
   standalone transformed primitives, since Phase 4 bakes those too.

Verification for this phase (no behavior change intended):

- Full gate must be byte-identical: `Test passed.` with the same accepted
  diff list as Plan 4 (see "Gate Reference State" below).
- Add a temporary debug dump (behind an env var or `#ifdef`) that prints,
  for `drums.pov` and `level1/shapes2.pov`, each object's recorded step list
  and the composed matrix rebuilt from the steps; assert the rebuilt matrix
  matches the stored composed matrix to within exact bit equality (it uses
  the same code path) — this proves the recording is complete. Remove or
  disable the dump before the phase closes.

## Phase 2 — The Replay Engine: `BakedGeometryBaker`

New files `src/render/bakedScene/BakedGeometryBaker.{h,cpp}`. This is the
"newly created coefficient rewriting": a scene-compile-time engine that takes
a `Geometry` (read-only) plus a `TransformStep` list and produces a baked,
world-space copy.

1. Implement the quadric congruence exactly as the baseline appendix from
   Phase 0 dictates: `quadricToMatrix` → per-step `Tinv · M · Tinvᵀ` →
   `matrixToQuadric` → square-term flag update, **one elementary step at a
   time, in recorded order**. These are static methods of
   `BakedGeometryBaker` operating on a *copy*; nothing in
   `environment/geometry` changes. The `Quadric` class already exposes its
   coefficients and `updateSquareTermFlag()` (`Quadric.h:64`); if a needed
   accessor is missing, add a read accessor only — no mutating API.
2. Implement sphere and plane replay per the Phase 0 appendix (moved
   centers, re-normalized plane equations), again as copies.
3. Implement `invert` replay.
4. Unit-style verification harness (a temporary `bakeVerify` mode or a small
   test target): for every transformed quadric operand in `drums.pov`,
   `level2/iortest.pov`, and `level1/texture1.pov`, evaluate the original
   path (local-space quadric + per-ray transform) and the baked copy on a
   grid of ~1000 sample rays and compare intersection `t` values bitwise.
   Bitwise equality is *not* expected in general (that is the whole point of
   the accepted image-delta caveat), but the harness quantifies the
   divergence and catches gross errors (wrong order, wrong inverse) before
   any image is rendered.

## Phase 3 — Collapse Transformed CSG Operand Kinds

Wire the baker into the existing compilation in `Scene::buildCompiledTracingScene`
/ the `render/bakedScene` classification (wherever
`BakedCsgOperandExecutionKind::TransformedQuadric / TransformedPlane /
TransformedSphere` are assigned):

1. `BakedCsgOperand` gains an owned baked geometry slot, e.g.
   `Quadric bakedQuadric; bool hasBakedQuadric;` (or a small tagged union /
   `std::unique_ptr<Geometry>`; prefer by-value for the quadric to avoid a
   pointer chase — this struct is hot).
2. At classification time, when an operand has a recorded step list and its
   kind would be `TransformedQuadric` / `TransformedSphere` /
   `TransformedPlane`, run the baker and classify it as
   `DirectPrimitive` / `DirectPlane` (etc.) pointing at the baked copy.
   `hasTransform` becomes false for the tracing path; the matrices stay in
   the record for debugging but are no longer read per ray.
3. The per-ray paths in `BakedCsgTracing` for these kinds
   (`BakedCsgTracing.h:203-206`, `240-243`, `283-286`, `308-309`) simply stop
   being reached; do **not** delete them in this plan (Plan 6 deletes the
   layer wholesale). Add a bake-time statistics line (mirroring the existing
   plan-inventory statistics) reporting how many operands were collapsed and
   how many remain transformed, per scene.
4. Operands whose geometry kind has no baseline rewrite rule (Box, Torus,
   patches, nested CSG with non-bakeable children) remain on the transformed
   path untouched. They are Plan 7/8 material.

Expected effect on `drums` (Group A, quadric/plane dominated): the
`TransformedQuadric` and `TransformedPlane` operand populations drop to zero;
`traceOperandAllCrossings` self-time and the `LocalIntersectionClone` count
fall sharply; the per-ray transform FLOPs disappear for these operands.

## Phase 4 — Collapse Standalone Transformed Primitives

Same collapse for `BakedSimpleBodyExecutionKind::TransformedPrimitive` bodies
whose geometry is a quadric/sphere/plane with a recorded step list: bake a
copy into `BakedSimpleBody` and reclassify as `DirectPrimitive`. This attacks
the `BakedSimpleBodyTracing::traceAllCrossings` clone share (34% of the
50.2 M clones per Plan 4 pending directions).

## Golden-Image Evaluation Protocol (critical for this plan)

The baked math reproduces *baseline* numerics, so images will shift **toward
the older baseline reference** on scenes whose goldens were re-baselined to
current-branch output (cf. the accepted drums `AE=71` delta). Per Plan 4
§6.1 this must be evaluated as recovery, not regression:

1. Before Phase 3 lands, render and archive the full suite at `4af1a75`
   (or reuse an existing archived baseline render set) as
   `../referenceTestImages.baseline4af1a75/` for comparison.
2. After Phases 3–4, for every scene the gate flags:
   - compute the diff vs the current golden **and** vs the baseline render;
   - if the new image is closer to the baseline render than the current
     golden is, the change is a recovery: record the scene, both AE/RMSE
     values, and re-baseline the golden, listing it in this document;
   - if it is closer to neither, it is a genuine regression: bisect the
     responsible operand kind by disabling its collapse (keep per-kind
     enable flags during this plan for exactly this purpose).
3. The final accepted-diff table of this plan replaces the Plan 4 table as
   the gate reference state for Plan 6.

## Measurement Gate

Every phase (0 excepted) must pass:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

plus the drums timing (3 runs) and, for Phases 3–4, the gprof recipe and the
benchmark panel.

### Gate Reference State (inherited from Plan 4)

`Test passed.` with accepted non-zero diffs:
`level1/ballbox1 AE=8`, `level1/texture3 AE=327`, `level2/illum2 AE=21`,
`level2/pawns AE=5`, `level3/car AE=6`, `level3/ionic5 AE=145114`,
`math/folium AE=10`, `math/helix AE=92`, `math/monkey AE=21`,
`math/quarpara AE=133`, `math/tcubic AE=48`, `math/trough AE=155`,
`math/witch AE=175`.

Phases 1–2 must not change this table at all. Phases 3–4 may change it only
through the Golden-Image Evaluation Protocol above.

## Acceptance Criteria for the Whole Plan

- All transformed quadric/sphere/plane CSG operands and standalone bodies in
  the suite are collapsed to direct kinds (bake statistics prove it).
- `drums` wall-clock improves measurably; target ≤ 7.0 s (from ≈ 8.55 s),
  i.e. recovering roughly half of the per-ray transform + clone cost that
  Plan 4 attributes to these operands. Miss the number but reduce the
  counters → acceptable if the panel does not regress; regress the panel →
  not acceptable.
- gprof on drums: `LocalIntersectionClone` call count and
  `traceOperandAllCrossings` call count both drop by the operand-population
  share (record before/after values).
- Golden protocol fully documented; no unexplained diffs.

## Known Dead Ends (do not repeat)

- Composed-matrix quadric coefficient baking (breaks byte-exact; the entire
  reason this plan replays elementary steps).
- Sphere → baked-quadric promotion when the baseline kept it a sphere
  (Plan 3 Phase H, 35 gate failures).
- Append-only scratch queues (breaks byte-exact; irrelevant here but adjacent).
- Any mutation of parsed `Geometry` objects — the baseline did it
  destructively; this plan exists to do it as a copy.
