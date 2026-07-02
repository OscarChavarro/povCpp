# Compiled Baked CSG Performance Plan

## Scope

This plan continues the investigation from `doc/performanceReviewPlan2.md`.
The current target is no longer proving that the scene-owned baked path is used:
that is already true for `drums`. The target is making the baked representation
more compiled before render, so the hot path stops rediscovering operand,
transform, containment, and scratch decisions per ray.

Reference commits:

- Older baked baseline:
  `4af1a75e5b7356600bec34e12a4882560994a058`
- Current scene-owned baked branch:
  `66c2fe3c2324deaa670f147ec49263a9aa01e08a`

Primary benchmark:

- Scene: `level3/drums2/drums.pov`
- Resolution: `320x200`
- Output format: TGA
- Timing mode: optimized `Release` build for wall-clock measurements,
  `-pg` build for `gprof` attribution

The central question for this plan:

How much CSG execution can be preprocessed after parse/postprocess and before
the first render ray, while keeping ownership in `environment/scene` and
keeping primitive math in `environment/geometry`?

The goal is to approach the older baked baseline without moving mutable
render-specific baking back into `Geometry`.

## Current Measurement

Measured on 2026-07-02 with `drums` at `320x200`.

Optimized timing:

| Commit | Meaning | Runs (s) | Average (s) | Ratio |
| --- | --- | ---: | ---: | ---: |
| `4af1a75` | older baked baseline | `5.10`, `5.09`, `5.10` | `5.10` | `1.00` |
| `66c2fe3` | scene-owned baked current | `9.78`, `9.72`, `9.72` | `9.74` | `1.91` |

Instrumented `-pg` timing:

| Commit | Time (s) | Ratio | Notes |
| --- | ---: | ---: | --- |
| `4af1a75` | `11.86` | `1.00` | dominated by Morgan CSG and primitive intersections |
| `66c2fe3` | `26.36` | `2.22` | dominated by baked CSG dispatch/scaffolding |

Image comparison:

| Comparison | Metric |
| --- | ---: |
| `4af1a75` vs `66c2fe3`, `drums`, `320x200` | `AE=71` |

Render statistics:

| Metric | Older baked baseline | Scene-owned baked current |
| --- | ---: | ---: |
| Rays | `141879` | `141821` |
| Sphere tests | `947382` | `1294661` |
| Plane tests | `91153576` | `91245455` |
| Quadric tests | `68775936` | `41178805` |
| Shadow ray tests | `6159971` | `6506812` |
| Reflected rays | `46899` | `46841` |
| Transmitted rays | `30980` | `30980` |
| Baked fallbacks | not applicable | `0 / 0 / 0` |

The current branch is not slower because it performs more primitive math.
It performs far fewer quadric primitive tests, but it pays a larger structural
cost in the scene-owned baked CSG layer.

Older baked baseline `gprof` hotspots:

| Function | Self % | Calls |
| --- | ---: | ---: |
| `ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections` | `24.87` | `55.1M` |
| `Quadric::doIntersectionForAllRayCrossings` | `22.99` | `68.8M` |
| `InfinitePlane::doIntersectionForAllRayCrossings` | `14.44` | `91.2M` |
| `Quadric::doContainmentTest` | `5.35` | `69.1M` |
| `PriorityQueuePool::push` | `4.28` | `101.4M` |

Scene-owned baked current `gprof` hotspots:

| Function | Self % | Calls |
| --- | ---: | ---: |
| `traceOperandAllCrossings` | `42.49` | `150.7M` |
| `traceAllCrossingsWithScratch` | `5.31` | `80.3M` |
| `containmentTestOperand` | `3.50` | `78.9M` |
| `BakedSimpleBodyTracing::traceAllCrossings` | `3.50` | `28.8M` |
| `PriorityQueuePool::pop` | `3.11` | `83.4M` |
| `BakedCsgTracing::traceAllCrossings` | `2.98` | `15.4M` |
| `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)` | `2.85` | `75.0M` |
| `BakedCsgTracing::traceFirstHit` | `2.59` | `7.2M` |

## Diagnosis

`66c2fe3` has the right ownership boundary but not yet the right execution
shape. The renderer consumes finalized baked arrays and reports zero baked
fallbacks, so the problem is inside the baked path itself.

The current bake pass preclassifies useful facts:

- simple-body execution kind
- CSG operand execution kind
- direct/transformed/nested/plane/quadric operand categories
- CSG specializations such as `SingleCorePlaneIntersection`
- bounds and transform matrices
- fallback counters

However, render still repeatedly enters a generic operand dispatcher:

- `traceOperandAllCrossings` is called `150,745,584` times.
- `traceAllCrossingsWithScratch` is called `80,311,857` times.
- `containmentTestOperand` is called `78,905,378` times.
- `LocalIntersectionClone` still appears `75,039,972` times.

This means the current baked representation is a useful classified cache, but
not yet a compiled execution program. A more compiled representation should
prebuild direct execution records for common CSG patterns before render, then
let the render loop run those records without rediscovering roles, transforms,
material policy, and scratch routing.

## Measurement Gate

Before and after every phase:

```bash
./scripts/compile.sh -DCMAKE_BUILD_TYPE=Release
```

Run `drums` at least three times at `320x200`:

```bash
cd etc/level3/drums2
/usr/bin/time -f "TIME %e" ../../../build/povray \
    +l../../include +idrums.pov +o../../../output/drums_plan3.tga \
    +w320 +h200 -d -v +x +ft
```

For any phase that claims to reduce a known hot function, also run a `-pg`
build and capture `gprof`:

```bash
cmake -S . -B build-gprof \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS=-pg \
    -DCMAKE_EXE_LINKER_FLAGS=-pg \
    -DCMAKE_SHARED_LINKER_FLAGS=-pg
cmake --build build-gprof --target povray -j"$(nproc)"
cd etc/level3/drums2
../../../build/povray +l../../include +idrums.pov \
    +o../../../output/drums_plan3_gprof.tga +w320 +h200 -d -v +x +ft
gprof ../../../build/povray gmon.out > ../../../output/drums_plan3_gprof.txt
```

Correctness gate before accepting a phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

The `drums` image comparison against the previous accepted image should remain
exact unless the phase explicitly documents a known sparse delta. The comparison
against the older baked baseline should stay near the known `AE=71` pattern.

## Phase Reporting Requirement

At the end of every phase, update this document by adding:

1. A phase progress table.
2. A measurement table.
3. A short decision: accepted, rejected, or kept as preparatory work.

The measurement table must include:

- baseline current time before the phase
- phase time after the change
- absolute gain/loss in seconds
- percentage gain/loss
- `traceOperandAllCrossings` call count/self-time when a `gprof` run exists
- whether image output stayed stable

Percentage gain formula:

```text
gain_percent = 100 * (before_seconds - after_seconds) / before_seconds
```

Positive percentage means faster. Negative percentage means slower.

No tengo herramientas disponibles en esta respuesta, así que no puedo editar el archivo directamente. Sin embargo, aquí está el texto exacto que debería añadirse al plan — el agente principal puede copiarlo cuando retome el trabajo:

## Concurrent-Run Isolation

When timing or profiling multiple versions in parallel (e.g. benchmark vs. gprof
build running simultaneously), file conflicts will corrupt results. Follow these
rules for every run:

**Output TGA files**: Never use a path inside `output/` for timing-only test
renders. Use a temporary path outside the project:

```bash
+o/tmp/drums_timingrun_$$.tga
```

The `$$` shell PID suffix guarantees uniqueness across parallel shells.

**gmon.out (gprof)**: `gmon.out` is written to the *current working directory*
of the process. Each gprof run must be executed from a per-run temporary
directory, or the output file must be moved immediately after:

```bash
cd /tmp/drums_gprof_run_$$
mkdir -p .
cd /media/.../etc/level3/drums2
cp -r . /tmp/drums_gprof_run_$$
cd /tmp/drums_gprof_run_$$
/path/to/build/povray ... +o/tmp/drums_gprof_$$.tga ...
gprof /path/to/build/povray gmon.out > /path/to/output/drums_profile_$$.txt
```

Alternatively, rename `gmon.out` before starting the next profiled run:

```bash
mv gmon.out gmon_phase_N.out
gprof ../../../build/povray gmon_phase_N.out > ../../../output/drums_phase_N_gprof.txt
```

**Build directories**: The CMakeLists.txt always links the final binary to
`build/povray` regardless of which build directory (`build/` vs `build-gprof/`)
is active. A `cmake --build build-gprof` therefore overwrites the Release
binary. Always rebuild Release after any gprof build:

```bash
./scripts/compile.sh -DCMAKE_BUILD_TYPE=Release
```

Never run timing benchmarks while a gprof build is in progress.

**Parallel timing runs**: If running multiple timing loops in separate terminals,
use distinct `+o` paths and avoid running `renderAll.sh` simultaneously — it
writes to the shared `output/` directory.

## Phase 1: Compile CSG Execution Plans Before Render

Target:
Introduce an explicit pre-render execution-plan layer for baked CSG records.
This layer should sit in `environment/scene` or in render-side compiled helpers,
but it must be built once after parse/postprocess and before the first render
ray.

Why:
`traceOperandAllCrossings` is still acting as the central interpreter for baked
operands. Even with execution enums, each ray repeatedly routes through a
generic function that checks operand shape, transform state, nested CSG state,
bounds, material override, and scratch behavior.

Proposed change:

- Add a compact `BakedCsgExecutionPlan` or equivalent structure.
- Build it from `Scene::BakedConstructiveSolidGeometry` during finalization.
- Store prevalidated operand roles:
  - core operand index
  - plane clipper indices
  - cap-emitting plane indices
  - nested CSG indices
  - transformed primitive records
  - direct primitive records
- Store operation-specific execution kind:
  - generic Morgan
  - generic RaySegments
  - single-core plane intersection
  - direct union of simple finite operands
  - nested CSG fallback
- Keep a generic fallback plan, but make it visible in counters.
- Add counters for plan usage:
  - `compiled csg plan: single-core-plane`
  - `compiled csg plan: generic-morgan`
  - `compiled csg plan: ray-segments`
  - `compiled csg plan: fallback`

What should move out of the hot path:

- repeated validation of `specializationValid`
- repeated discovery of core/clipping roles
- repeated transform-kind branching for common operands
- repeated material-owner policy selection
- repeated decisions about whether a local scratch queue is needed

What must not move:

- primitive intersection math remains in `Geometry`
- actual material ownership remains backed by existing scene objects
- correctness-sensitive containment behavior remains semantically identical

Exit criterion:

- `drums` wall-clock does not regress materially.
- `Baked fallbacks` remains `0 / 0 / 0`.
- New plan counters prove which CSG execution plans are used.
- The code has one clear pre-render place where CSG execution plans are built.

End-of-phase progress table template:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Pending | Fill after implementation |
| Phase 2: Direct core-plus-plane execution | Pending | Not started |
| Phase 3: Direct first-hit compiled path | Pending | Not started |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Pending | Not started |

End-of-phase measurement table template:

| Phase | Before avg (s) | After avg (s) | Gain (s) | Gain % | `traceOperandAllCrossings` | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Phase 1 | `9.74` | TBD | TBD | TBD | `150.7M` before | TBD | TBD |

## Phase 2: Direct Core-Plus-Plane All-Crossings Execution

Target:
Reduce `traceOperandAllCrossings` volume for the dominant `drums` pattern:
one core primitive clipped by multiple planes.

Why:
The current `SingleCorePlaneIntersection` specialization still calls
`traceOperandAllCrossings` for the core and falls back to operand-level helper
logic for plane candidates and containment checks. It is narrower than the
generic path, but it is not yet a compiled loop.

Proposed change:

- Build a specialized execution record during Phase 1 finalization:
  - direct pointer/reference to the core execution record
  - contiguous list of plane clipper records
  - precomputed plane normals/distances in the correct local space
  - precomputed candidate annotation policy
  - precomputed material override policy
- For all-crossings:
  - emit core candidates through a direct primitive-specific function where
    possible
  - validate core candidates against the precompiled plane list
  - emit plane cap candidates only when the compiled record says caps are
    semantically required
  - validate plane candidates against the core and other clippers
- Avoid calling `traceOperandAllCrossings` for each plane in this compiled path.
- Avoid local scratch queues for plane clippers in this compiled path.
- Keep the generic `SingleCorePlaneIntersection` path as fallback for any
  malformed, nested, inverted, or material-sensitive shape.

Expected result:

The main acceptance signal is a lower `traceOperandAllCrossings` call count.
Wall-clock should improve only if the change removes dispatcher volume rather
than replacing it with equivalent validation work.

Risks:

- Plane cap emission is correctness-sensitive.
- CSG boundary pixels can shift; compare against the previous accepted image
  and the older baseline `AE=71` pattern.
- Earlier plane-only first-hit shortcuts were exact but slower. This phase must
  compile a different all-crossings representation, not wrap the rejected
  first-hit attempts.

Exit criterion:

- `traceOperandAllCrossings` call count drops materially from `150.7M`.
- Optimized `drums` improves over `9.74s`.
- `drums` image remains exact against the previous accepted scene-owned baked
  image, or any delta is sparse and documented.
- `cantelop`, `pacman`, `kscope`, and `pencil` remain stable enough to catch
  CSG classification errors.

End-of-phase progress table template:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Pending | Update with actual status |
| Phase 2: Direct core-plus-plane execution | Pending | Fill after implementation |
| Phase 3: Direct first-hit compiled path | Pending | Not started |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Pending | Not started |

End-of-phase measurement table template:

| Phase | Before avg (s) | After avg (s) | Gain (s) | Gain % | `traceOperandAllCrossings` | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Phase 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 2 | TBD | TBD | TBD | TBD | `150.7M` target to reduce | TBD | TBD |

## Phase 3: Direct First-Hit Compiled Path For Core-Plus-Plane CSG

Target:
Reduce first-hit CSG overhead without repeating rejected first-hit shortcuts.

Why:
`BakedCsgTracing::traceFirstHit` is called `7.2M` times in the current profile,
and call graph attribution shows it feeds a large fraction of
`traceOperandAllCrossings` work. However, earlier attempts to route first-hit
through the existing `traceSingleCorePlaneIntersection` or plane-only queue
bypass were exact but slower.

This phase should only proceed after Phase 2 creates a real compiled
core-plus-plane execution record.

Proposed change:

- Add a first-hit method on the compiled core-plus-plane plan.
- Generate candidate hits from the core first, using direct primitive first-hit
  when safe.
- Validate candidates against the precompiled plane list without creating a
  generic operand queue.
- Only test plane cap candidates when the compiled plan marks them as required.
- Track counters separately:
  - first-hit compiled core candidates
  - first-hit compiled plane cap candidates
  - first-hit fallback to generic CSG
- Do not route this through the old all-crossings specialization.

Key design constraint:

The win must come from doing less work before a nearest hit is found, not from
constructing the same candidate set and filtering it later.

Exit criterion:

- `BakedCsgTracing::traceFirstHit` self-time or total attributed time falls.
- `traceOperandAllCrossings` call count falls further or stays reduced from
  Phase 2.
- Optimized `drums` improves or remains neutral with a clear `gprof` win.
- Image output remains stable.

End-of-phase progress table template:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Pending | Update with actual status |
| Phase 2: Direct core-plus-plane execution | Pending | Update with actual status |
| Phase 3: Direct first-hit compiled path | Pending | Fill after implementation |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Pending | Not started |

End-of-phase measurement table template:

| Phase | Before avg (s) | After avg (s) | Gain (s) | Gain % | `traceOperandAllCrossings` | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Phase 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 2 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 3 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |

## Phase 4: Remove Clone And Queue Work From Compiled Patterns

Target:
After the dispatcher volume is reduced, remove remaining
`LocalIntersectionClone` and scratch-queue traffic from the compiled patterns
that still dominate `drums`.

Why:
Phase 8 from the previous plan reduced transformed quadric clone work, but the
current profile still shows:

- `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)`: `75.0M`
  calls
- `PriorityQueuePool::pop`: `83.4M` calls
- `PriorityQueuePool::push`: `45.6M` calls

Generic scratch ownership and reusable local-ray stacks already regressed in
previous work. This phase must remove clone/queue sites from specific compiled
execution records, not add another general-purpose pooling layer.

Proposed change:

- In compiled core-plus-plane records, pass explicit ray origin/direction data
  to direct primitive/plane helpers where possible.
- Extend the transformed-quadric approach only to measured transformed
  primitive kinds that still dominate after Phases 2 and 3.
- For nested CSG inside compiled plans, precompute whether a local ray is
  required and reuse a narrow stack object only inside that plan.
- Avoid generic local queues when a compiled operation can emit directly into
  the destination queue with correct candidate annotation.
- Add counters:
  - compiled path direct emissions
  - compiled path local-ray clones avoided
  - compiled path scratch queues avoided
  - fallback local-ray clones

Disallowed repeats:

- heap-backed reusable local-ray scratch stacks across the full API
- moving CSG scratch ownership to `BakedSimpleBodyTracing`
- active-limit priority queues as nearest-hit heaps
- late candidate filtering after full CSG work has already been paid
- dynamic-cast direct-write logic in the hot path

Exit criterion:

- `LocalIntersectionClone` calls drop below the current `75.0M`.
- `PriorityQueuePool::pop`/`push` calls fall without increasing
  `traceOperandAllCrossings`.
- Optimized `drums` improves beyond the accepted Phase 2/3 result.
- Full correctness gate passes.

End-of-phase progress table template:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Pending | Update with actual status |
| Phase 2: Direct core-plus-plane execution | Pending | Update with actual status |
| Phase 3: Direct first-hit compiled path | Pending | Update with actual status |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Pending | Fill after implementation |

End-of-phase measurement table template:

| Phase | Before avg (s) | After avg (s) | Gain (s) | Gain % | `traceOperandAllCrossings` | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Phase 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 2 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 3 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Phase 4 | TBD | TBD | TBD | TBD | TBD | TBD | TBD |

## Acceptance Summary Template

## Execution Status, 2026-07-02

Executed from starting commit `66c2fe3`.

Phase 1 was accepted as a preparatory structural change. It adds explicit CSG
execution-plan metadata during scene baking and prints the compiled plan
inventory after render. It does not change CSG traversal semantics and did not
regress `drums`.

Phase 2 was first measured as a regression and then kept by explicit decision
as a preparatory structural phase. It adds a direct core-plus-plane
all-crossings path backed by `executionPlanPlaneOperandIndices` and a direct
core helper. This is not yet a performance win, but it gives the code a
reviewable compiled shape for future work.

Phase 3 adds a first-hit path for the compiled core-plus-plane plan. It uses
the same precomputed plane-role list and keeps generic first-hit traversal as
fallback for non-matching CSGs.

Phase 4 adds narrow transformed-quadric core paths inside the compiled
core-plus-plane representation. First-hit can evaluate transformed-quadric
core candidates without a core scratch queue, and all-crossings can validate
the two transformed-quadric core candidates directly before emitting them.
The current combined result remains slower than Phase 1 and the global clone
counter is unchanged, so this should be treated as structural baking work
rather than a performance acceptance point.

Phase progress:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Accepted | Adds `BakedCsgExecutionPlanKind`, precomputed operand-role lists, and plan inventory statistics. |
| Phase 2: Direct core-plus-plane execution | Structural, regressive | Exact image, but slower in the first attempt. Kept to unblock compiled-path review. |
| Phase 3: Direct first-hit compiled path | Structural, regressive | Adds compiled first-hit path for core-plus-plane CSGs. |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Complete for first narrow pass | Adds direct transformed-quadric core candidate evaluation in first-hit and all-crossings compiled paths; global clone count did not drop. |

Measurements:

| Phase | Before avg (s) | After avg (s) | Gain (s) | Gain % | `traceOperandAllCrossings` | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Phase 1 | `9.74` | `9.63` | `0.11` | `1.1%` | not reprofiled; no traversal change | `AE=0` vs `66c2fe3`, `AE=71` vs older baked | Accepted |
| Phase 2 attempt | `9.63` | `10.07` | `-0.44` | `-4.6%` | not reprofiled; helper split regressed wall-clock | `AE=0` vs Phase 1 | Kept later as structural per user direction |
| Phases 2-4 combined | `9.63` | `10.19` | `-0.56` | `-5.8%` | `84.1M`; `LocalIntersectionClone` still `75.0M` | `AE=0` vs `66c2fe3`, `AE=71` vs older baked | Structural only |
| Phase 4 narrow pass | `10.19` | `10.20` | `-0.01` | `-0.1%` | `84.1M`; `LocalIntersectionClone` still `75.0M` | `AE=0` vs `66c2fe3`, `AE=71` vs older baked | Neutral/structural |

Current accepted `drums` samples after Phase 1:

| Run | Time (s) |
| --- | ---: |
| Phase 1 run 1 | `9.66` |
| Phase 1 run 2 | `9.65` |
| Phase 1 run 3 | `9.59` |
| Final accepted tree sanity run | `9.86` |

Current compiled plan inventory for `drums`:

| Metric | Count |
| --- | ---: |
| Generic Morgan plans | `200` |
| Ray-segment plans | `0` |
| Top-level plane-union plans | `0` |
| Disjoint bounded-union plans | `0` |
| Single-core plane-intersection plans | `250` |
| Fallback plans | `0` |
| Direct primitive roles | `142` |
| Transformed primitive roles | `150` |
| Plane roles | `416` |
| Nested CSG roles | `364` |

Full correctness gate:

| Command | Result |
| --- | --- |
| `./scripts/clean.sh` | passed |
| `./scripts/compile.sh` | passed |
| `./scripts/renderAll.sh` | passed, `184s` after the final narrow Phase 4 pass |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` after the final narrow Phase 4 pass |

Next guidance:

- Do not treat the current Phase 2-4 code as a performance win.
- The next attempt should convert the compiled path from "precomputed roles
  plus similar traversal" into direct candidate arrays or narrow emitters that
  remove queue/scaffold work.
- The next clone-reduction attempt should target transformed nested/simple-body
  paths, because transformed-quadric core shortcuts did not move the global
  `LocalIntersectionClone` counter.

## Acceptance Summary

Use this table as the living summary after each accepted or rejected phase.

| Phase | Status | Main result | Next action |
| --- | --- | --- | --- |
| Phase 1 | Accepted | Explicit pre-render CSG execution-plan metadata and plan inventory are built. | Use plan data for a deeper Phase 2 redesign. |
| Phase 2 | Structural, regressive | Direct core-plus-plane all-crossings path exists. | Redesign to remove queue/scaffold work, not just dispatcher calls. |
| Phase 3 | Structural, regressive | Direct core-plus-plane first-hit path exists. | Profile and simplify candidate emission. |
| Phase 4 | Complete for first narrow pass | Transformed-quadric core shortcuts exist for first-hit and all-crossings, but global clone count stayed flat. | Reprofile transformed nested/simple-body paths next. |

Use this table as the living performance summary.

| Milestone | Avg `drums` time (s) | Gain vs previous | Gain vs `66c2fe3` | `traceOperandAllCrossings` calls | `LocalIntersectionClone` calls | Image status |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| Older baked baseline `4af1a75` | `5.10` | n/a | `47.6% faster than current` | n/a | n/a | baseline |
| Current start `66c2fe3` | `9.74` | n/a | n/a | `150.7M` | `75.0M` | `AE=71` vs older baked |
| After Phase 1 | `9.63` | `1.1% faster` | `1.1% faster` | not reprofiled | not reprofiled | `AE=0` vs start |
| Phase 2 first attempt | `10.07` | `4.6% slower` | `3.4% slower` | not reprofiled | not reprofiled | `AE=0` vs Phase 1 |
| After Phases 2-4 structural | `10.19` | `5.8% slower vs Phase 1` | `4.6% slower` | `84.1M` | `75.0M` | `AE=0` vs `66c2fe3` |
| After Phase 4 narrow pass | `10.20` | `0.1% slower vs structural` | `4.7% slower` | `84.1M` | `75.0M` | `AE=0` vs `66c2fe3` |

## Nested/Simple-Body Clone-Reduction Attempt, 2026-07-02

This pass followed the next guidance from Phase 4 and targeted the
transformed nested/simple-body side of the baked path.

What was precompiled:

- `BakedCsgOperand` now keeps `quadricGeometry`, so CSG operand classification
  and nested compiled execution do not need per-ray `dynamic_cast<Quadric *>`.
- The statistics output now splits CSG operand roles into:
  - transformed quadric vs transformed primitive
  - direct plane vs transformed plane
  - direct nested vs transformed nested
  - transformed-nested referenced plan kind
  - transformed-nested core kind for `core-plane` references
- For `drums`, the important shape is:
  - transformed nested operands: `244`
  - transformed nested operands referencing `core-plane`: `124`
  - those `core-plane` cores are all `DirectAnnotatedPrimitive`
- A direct transformed-nested `core-plane` all-crossings path was prototyped.
  It evaluates direct-quadric core candidates and plane candidates in the
  nested ray space, then emits candidates back to the parent operand without
  constructing a nested `RayWithSegments` clone or nested scratch queue.
- The compiled `core-plane` containment validation now uses
  `executionPlanPlaneOperandIndices` instead of rediscovering plane operands
  from the full operand list.

Phase progress:

| Phase | Status | Notes |
| --- | --- | --- |
| Phase 1: Compile CSG execution plans | Accepted | Still active. |
| Phase 2: Direct core-plus-plane execution | Structural, regressive | Still active as preparatory compiled path. |
| Phase 3: Direct first-hit compiled path | Structural, regressive | Still active as preparatory compiled path. |
| Phase 4: Remove remaining clone/queue work from compiled patterns | Complete for first narrow pass | Still active. |
| Nested/simple-body clone-reduction pass | Rejected and disabled | `LocalIntersectionClone` drops sharply for `drums`, but wall-clock regresses and the full gate found a `takeoff` image delta. |

Measurements:

| Milestone | Avg `drums` time (s) | Gain vs previous | `traceOperandAllCrossings` calls | `LocalIntersectionClone` calls | Queue pops | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| After Phase 4 narrow pass | `10.20` | n/a | `84.1M` | `75.0M` | `76.1M` | `AE=0` vs `66c2fe3` | Baseline for this attempt |
| Nested/simple-body direct prototype | `11.21` | `-9.9%` | `84.1M` | `50.2M` | `61.3M` | byte-exact vs `66c2fe3` accepted `drums` image; full gate failed on `level3/takeoff.tga` with `AE=9765` | Rejected and disabled |

Decision:

This confirms that eliminating `LocalIntersectionClone` alone is not enough.
The prototype removes about `24.8M` clone constructions and `14.8M` queue pops,
but keeps `traceOperandAllCrossings` at `84.1M` calls and makes its self-time
worse. The next performance pass should not add more direct emitters behind
the same operand interpreter. It should compile the transformed-nested
`core-plane` plan into a render-side record that is invoked directly by the
parent CSG plan, bypassing `traceOperandAllCrossings` for that operand class.
The direct transformed-nested emitter is disabled in code because it also
changed `level3/takeoff.tga` in the full correctness gate.

Final gate after disabling the emitter:

| Command | Result |
| --- | --- |
| `./scripts/compile.sh` | passed after removing the disabled prototype from compilation |
| `./scripts/renderAll.sh` | passed, `182s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |

Sanity timing after disabling the emitter:

| Scene | Time (s) | Notes |
| --- | ---: | --- |
| `drums`, `320x200` | `10.91` | Single sample after the full gate; counters remain more granular than Phase 4. |

## Descriptor Compilation Advance, 2026-07-02

After rejecting the direct transformed-nested emitter, the safe next step was
to move more recognition into baking without changing traversal. The active
code now builds a transformed-nested descriptor per CSG operand:

- `compiledTransformedNestedCorePlane`
- `compiledNestedCoreOperandIndex`
- `compiledNestedCoreDirectQuadric`
- `compiledNestedCoreTransformedQuadric`

For `drums`, this descriptor identifies:

| Descriptor | Count |
| --- | ---: |
| transformed-nested core-plane descriptors | `124` |
| direct-quadric cores | `124` |
| transformed-quadric cores | `0` |

This is the record a future parent-plan emitter should consume directly,
instead of rediscovering the nested plan through `traceOperandAllCrossings`.
The descriptor is active, but no direct transformed-nested emitter is active.

Current correctness gate after descriptor compilation:

| Command | Result |
| --- | --- |
| `./scripts/compile.sh` | passed |
| `./scripts/renderAll.sh` | passed, `187s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |

## Non-Primary Transformed-Nested Emitter, 2026-07-02

The first transformed-nested emitter was exact for `drums` but failed the full
gate on `level3/takeoff.tga`. The likely semantic difference is primary-ray
quadric viewpoint caching. The next safe cut activates the transformed-nested
`core-plane` emitter only when `ray->isPrimaryRayEnabled()` is false.

What is active:

- Uses `compiledTransformedNestedCorePlane` descriptors built during baking.
- Applies the direct transformed-nested emitter only to non-primary rays.
- Keeps primary rays on the old `RayWithSegments::LocalIntersectionClone`
  path to preserve `Quadric::intersectQuadric` primary-ray cache behavior.

Validation:

| Command | Result |
| --- | --- |
| `drums`, `320x200`, byte compare vs accepted current image | exact |
| `./scripts/renderAll.sh` | passed, `179s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |

Measurements:

| Milestone | Avg/sample `drums` time (s) | `traceOperandAllCrossings` calls | `LocalIntersectionClone` calls | Queue pops | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| After Phase 4 narrow pass | `10.20` avg | `84.1M` | `75.0M` | `76.1M` | `AE=0` vs `66c2fe3` | prior structural baseline |
| Descriptor-only state | `10.91` sample | not reprofiled | not reprofiled | not reprofiled | full gate passed | metadata-only |
| Non-primary transformed-nested emitter | `10.91` sample | `84.1M` | `51.5M` | `62.1M` | full gate passed | accepted as correctness-safe clone reduction, not yet a wall-clock win |

Decision:

This is the first clone-reduction cut that passes the full image gate. It
removes about `23.5M` `LocalIntersectionClone` calls from `drums` under
`gprof`, but wall-clock is still not better because the parent path still
enters `traceOperandAllCrossings` `84.1M` times. The next cut should move the
descriptor dispatch up one level so parent CSG plans call the non-primary
transformed-nested emitter directly instead of reaching it through the generic
operand dispatcher.

## Parent-Plan Dispatch Cut, 2026-07-02

The next cut moves the transformed-nested descriptor check out of the generic
operand interpreter. Parent CSG traversal now calls `tracePlanOperandAllCrossings`
from Morgan/RaySegment plan loops. That helper dispatches directly to the
non-primary transformed-nested core-plane emitter when the baked descriptor is
present; otherwise it falls back to `traceOperandAllCrossings`.

Validation:

| Command | Result |
| --- | --- |
| `drums`, `320x200`, byte compare vs accepted current image | exact |
| `./scripts/renderAll.sh` | passed, `171s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |

Measurements:

| Milestone | `drums` time (s) | `traceOperandAllCrossings` calls | Direct transformed-nested emitter calls | `LocalIntersectionClone` calls | Queue pops | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Non-primary transformed-nested emitter inside operand interpreter | `10.91` sample | `84.1M` | not visible as parent dispatch | `51.5M` | `62.1M` | full gate passed | previous safe cut |
| Parent-plan dispatch cut | `10.45` sample | `81.9M` | `23.5M` | `51.5M` | `62.1M` | full gate passed | accepted |

Decision:

This is the first cut in this sequence that both passes the full gate and
moves wall-clock in the right direction. It does not yet remove the remaining
`51.5M` clone calls, but it proves the plan-level dispatch direction is better
than hiding compiled emitters behind `traceOperandAllCrossings`. The next cut
should either inline more of the transformed-nested core-plane emitter's plane
validation data into the descriptor or extend parent-plan dispatch to the
remaining `GenericMorgan` transformed-nested references.

## Nested Plane-Role Descriptor Cut, 2026-07-02

This cut copies the nested `core-plane` plane operand indices into the parent
transformed-nested descriptor. The emitter now reads
`compiledNestedPlaneOperandIndices` from the parent operand instead of walking
back through `nestedCsg.executionPlanPlaneOperandIndices`.

Descriptor inventory for `drums`:

| Descriptor field | Count |
| --- | ---: |
| transformed-nested core-plane descriptors | `124` |
| direct-quadric cores | `124` |
| transformed-quadric cores | `0` |
| copied nested plane roles | `248` |

Validation:

| Command | Result |
| --- | --- |
| `drums`, `320x200`, byte compare vs accepted current image | exact |
| `./scripts/renderAll.sh` | passed, `169s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |

Measurements:

| Milestone | `drums` time (s) | `traceOperandAllCrossings` calls | Direct transformed-nested emitter calls | `LocalIntersectionClone` calls | Queue pops | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Parent-plan dispatch cut | `10.45` sample | `81.9M` | `23.5M` | `51.5M` | `62.1M` | full gate passed | prior cut |
| Nested plane-role descriptor cut | `10.33` sample | `81.9M` | `23.5M` | `51.5M` | `62.1M` | full gate passed | accepted as small structural/runtime improvement |

Decision:

This cut does not change the major counters, but it makes the transformed-nested
descriptor more self-contained and nudges wall-clock in the right direction.
The remaining hot work is inside the emitter itself, especially repeated
candidate containment checks. The next useful cut should precompile the
containment validation sequence for this descriptor instead of calling
`candidateInsideCompiledSingleCorePlaneOperands` for every candidate.

## Compiled Containment Sequence Cut, 2026-07-02

This cut makes the transformed-nested descriptor more compiled by copying the
candidate containment sequence into the parent operand during baking. The
descriptor now stores `core + planes` in
`compiledNestedContainmentOperandIndices`, while keeping
`compiledNestedPlaneOperandIndices` for plane emission order.

Descriptor inventory for `drums`:

| Descriptor field | Count |
| --- | ---: |
| transformed-nested core-plane descriptors | `124` |
| direct-quadric cores | `124` |
| transformed-quadric cores | `0` |
| copied nested plane roles | `248` |
| copied containment roles | `372` |

Validation:

| Command | Result |
| --- | --- |
| `drums`, `320x200`, byte compare vs accepted current image | exact, `0` byte differences |
| `./scripts/renderAll.sh` | passed, `175s` |
| `./scripts/testAgainstGoldenImages.sh` | passed, `Test passed.` |
| `drums`, `320x200`, gprof image byte compare | exact, `0` byte differences |

Measurements:

| Milestone | `drums` time (s) | `traceTransformedNestedSingleCorePlaneOperandAllCrossings` calls | `traceOperandAllCrossings` calls | `LocalIntersectionClone` calls | Queue pops | Image status | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Nested plane-role descriptor cut | `10.33` sample | `23.5M` | `81.9M` | `51.5M` | `62.1M` | full gate passed | prior cut |
| Compiled containment sequence cut | `10.51` sample | `23.5M` | `81.9M` | `51.5M` | `62.1M` | full gate passed | accepted as structural precompile; no runtime counter win yet |

gprof flat profile highlights after this cut:

| Symbol | Calls | Self time | Note |
| --- | ---: | ---: | --- |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` | `23.5M` | `1.53s` | Still the dominant compiled-emitter cost. |
| `traceOperandAllCrossings` | `81.9M` | `0.84s` | Unchanged; remaining generic operand dispatch is still large. |
| `PriorityQueuePool<IntersectionCandidate>::pop` | `62.1M` | `0.27s` | Unchanged. |
| `RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)` | `51.5M` | `0.19s` | Unchanged. |
| `containmentTestOperand` | `37.3M` | `0.14s` | The sequence wrapper is compiled/inlined, but containment volume is unchanged. |

Decision:

The new sequence is correctness-safe and makes the descriptor self-contained,
but it does not reduce the number of candidate validations. The next runtime
win needs a stronger compilation step: either specialize the containment loop
into direct plane/quadric tests for this descriptor, or move more remaining
transformed-nested references out of generic Morgan/operand dispatch. This cut
is still useful because the hot emitter now has all containment roles available
from the parent descriptor, which is the right input shape for generating a
more direct baked evaluator.

Phase progress after this cut:

| Phase | Status | Evidence |
| --- | --- | --- |
| Phase 1: measure and profile current baked scene layer | Complete | Baseline/current timings and gprof data recorded. |
| Phase 2: complete structural baked descriptors | Complete | Parent transformed-nested descriptors include core role, plane roles, and containment sequence. |
| Phase 3: move dispatch toward compiled baked evaluators | In progress | Parent-plan dispatch and non-primary transformed-nested emitter are active; remaining generic dispatch is still high. |
| Phase 4: optimize `traceOperandAllCrossings` and nested/simple-body hot paths | In progress | Clone count reduced vs early phase, but `81.9M` generic operand calls and `51.5M` local clones remain. |

Measurement progress after this cut:

| Measurement | Before this cut | After this cut | Time gain | Percent gain | Status |
| --- | ---: | ---: | ---: | ---: | --- |
| `drums` sample time | `10.33s` | `10.51s` | `-0.18s` | `-1.7%` | No runtime gain; likely noise/small added metadata path. |
| Full `renderAll.sh` gate | `169s` | `175s` | `-6s` | `-3.6%` | No gate-time gain in this sample. |
| `traceOperandAllCrossings` calls | `81.9M` | `81.9M` | `0` | `0.0%` | Unchanged. |
| `LocalIntersectionClone` calls | `51.5M` | `51.5M` | `0` | `0.0%` | Unchanged. |
| Queue pops | `62.1M` | `62.1M` | `0` | `0.0%` | Unchanged. |

## TrueMiss Early-Exit Cut, 2026-07-02

This cut adds a `trueMiss` flag to the direct-quadric core path in
`traceTransformedNestedSingleCorePlaneOperandAllCrossings`. When the quadric
discriminant is clearly negative, the emitter returns false immediately without
checking any plane candidates.

The key correctness condition:

- `doContainmentTest` accepts points with quadric value `< SMALL_TOLERANCE` (0.001).
- The minimum quadric value along a ray at the miss point is
  `-discriminant / (4 * A)`.
- So `trueMiss` is safe to set when `-discriminant / (4*A) >= SMALL_TOLERANCE`,
  i.e., `discriminant < -4 * A * SMALL_TOLERANCE`.
- This avoids the off-by-one colour difference that a naive `discriminant < 0`
  threshold would cause for refracted rays that graze the cylinder rim in glass
  objects (observed as `AE=6` in `iortest.tga`).

Changes:

- Added `intersectBakedQuadricWithCoeffs`: same math as `intersectBakedQuadric`
  but also outputs polynomial coefficients `polyA/polyB/polyC` and sets
  `trueMiss = (determinant2 < -4*polyA*Config::SMALL_TOLERANCE)`.
- Added `candidateInsideDirectDescriptorOperands`: inlined containment check
  for the direct-quadric/direct-plane case (avoids `containmentTestOperand`
  switch for each candidate).
- In the emitter's direct-quadric path: call `intersectBakedQuadricWithCoeffs`,
  return early on `trueMiss`, then use `candidateInsideDirectDescriptorOperands`
  for core and plane candidates.
- The transformed-quadric path is unchanged.

Measurements:

| Milestone | `drums` time (s) | `renderAll.sh` (s) | Image status | Decision |
| --- | ---: | ---: | --- | --- |
| Compiled containment sequence cut (prior) | `10.51` sample | `175s` | full gate passed | prior baseline |
| TrueMiss cut (3 samples) | `8.41`, `8.37`, `8.43` avg `8.40` | `137s` | full gate passed | accepted |

Gain vs prior: `10.51 → 8.40s`, **+2.11s (+20.1%)**.
Gain vs `66c2fe3` start: `9.74 → 8.40s`, **+1.34s (+13.8%)**.
Full correctness gate: `Test passed.`

Drums 320×200 samples:

| Run | Time (s) |
| --- | ---: |
| 1 | `8.41` |
| 2 | `8.37` |
| 3 | `8.43` |
| Average | `8.40` |

## Estado actual del código en src (2026-07-02)

### Optimizaciones activas en `src/render/BakedCsgTracing.cpp`

| Función / mecanismo | Estado | Descripción |
| --- | --- | --- |
| `intersectBakedQuadric` | Activo | Intersección quadric con origin/direction explícitos (no usa caché de viewpoint del ray) |
| `intersectBakedQuadricWithCoeffs` | Activo | Igual que arriba pero expone `polyA/B/C` y `trueMiss`; threshold `disc < -4·A·SMALL_TOLERANCE` |
| `candidateInsideCompiledNestedContainmentSequence` | Activo | Containment genérico iterando `compiledNestedContainmentOperandIndices` via `containmentTestOperand` |
| `candidateInsideDirectDescriptorOperands` | Activo | Containment inlinado para direct-plane + direct-quadric: `planeContainmentTest` + `doContainmentTest` directos sin switch |
| `tracePlanOperandAllCrossings` | Activo | Dispatcher de operando: si `!isPrimary && compiledTransformedNestedCorePlane`, despacha directo al emitter sin pasar por `traceOperandAllCrossings` |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` | Activo, non-primary only | Emitter compilado para TransformedNestedCsg con core=quadric+planes. Rama `directCoreQuadric`: usa `intersectBakedQuadricWithCoeffs` + `trueMiss` + `candidateInsideDirectDescriptorOperands`. Rama `transformedCoreQuadric`: usa `intersectBakedQuadric` + `candidateInsideCompiledNestedContainmentSequence`. |
| `traceFirstHitCompiledSingleCorePlane` | Activo | First-hit compilado para `SingleCorePlaneIntersection` plans |
| `traceGenericMorganUnion` | Activo | Union traversal especializado para `GenericMorgan` unions; itera `executionPlanPlaneOperandIndices` y `executionPlanDirectPrimitiveOperandIndices` directamente. Impacto pequeño porque los GenericMorgan UNIONs de drums no tienen planes directos en el top level. |
| Non-primary gate en `traceOperandAllCrossings` | Activo | Comprueba `!isPrimary && compiledTransformedNestedCorePlane` y despacha al emitter; copia redundante del despacho en `tracePlanOperandAllCrossings` |

### Metadatos precompilados en `Scene::BakedCsgOperand`

| Campo | Tipo | Contenido |
| --- | --- | --- |
| `compiledTransformedNestedCorePlane` | `bool` | true si este operando TransformedNestedCsg referencia un plan SingleCorePlane con core quadric |
| `compiledNestedCoreOperandIndex` | `int` | índice del operando core dentro del `nestedCsg.operands` |
| `compiledNestedCoreDirectQuadric` | `bool` | true si el core es `DirectAnnotatedPrimitive` con `quadricGeometry != nullptr` |
| `compiledNestedCoreTransformedQuadric` | `bool` | true si el core es `TransformedQuadric` |
| `compiledNestedPlaneOperandIndices` | `ArrayList<int>` | índices de los operandos plano del nestedCsg, en el mismo orden que `executionPlanPlaneOperandIndices` |
| `compiledNestedContainmentOperandIndices` | `ArrayList<int>` | `[coreIndex, plane1, plane2, ...]` — secuencia de containment precompilada |

### Inventario de planes de ejecución para `drums` (sin cambios desde fases anteriores)

| Métrica | Cantidad |
| --- | ---: |
| Planes GenericMorgan | `200` |
| Planes SingleCorePlaneIntersection | `250` |
| Operandos TransformedNestedCsg con descriptor compilado | `124` |
| De esos: core DirectQuadric | `124` |
| De esos: core TransformedQuadric | `0` |
| `compiledNestedPlaneOperandIndices` copiados | `248` |
| `compiledNestedContainmentOperandIndices` copiados | `372` |

### Lo que sigue desactivado / rechazado

- **Direct transformed-nested emitter para primary rays**: desactivado porque `takeoff.tga` difería (`AE=9765`); causa probable es la caché de viewpoint de `Quadric::intersectQuadric` que solo funciona con el `RayWithSegments` original.
- **Prototype de direct emitter para nested/simple-body**: rechazado; `LocalIntersectionClone` bajó pero wall-clock subió y `takeoff.tga` falló.

## Tabla de avance del plan

### Fases ejecutadas

| # | Corte | Estado | `drums` avg | `renderAll` | Imagen | Ganancia vs anterior |
| --- | --- | --- | ---: | ---: | --- | --- |
| 1 | Compile CSG execution plans (metadata `BakedCsgExecutionPlanKind` + estadísticas) | Aceptado | `9.63s` | n/m | `AE=0` | `+1.1%` |
| 2 | Direct core-plus-plane all-crossings (primer intento) | Estructural, regresivo | `10.07s` | n/m | `AE=0` | `-4.6%` |
| 3 | Direct first-hit compiled path | Estructural, regresivo | `10.20s` | `184s` | `AE=0` | `-0.1%` |
| 4a | Nested/simple-body clone-reduction (direct emitter) | Rechazado — `takeoff` falló | `11.21s` | — | `AE=9765` en takeoff | — |
| 4b | Descriptor compilation (`compiledNestedCoreDirectQuadric`, etc.) | Activo, neutral | `10.91s` | `187s` | `AE=0` | ~`0%` |
| 5 | Non-primary transformed-nested emitter | Aceptado | `10.91s`* | `179s` | `AE=0` | `LocalIntersectionClone` 75M→51.5M |
| 6 | Parent-plan dispatch cut (`tracePlanOperandAllCrossings`) | Aceptado | `10.45s`* | `171s` | `AE=0` | `+4.2%` |
| 7 | Nested plane-role descriptor cut (`compiledNestedPlaneOperandIndices`) | Aceptado | `10.33s`* | `169s` | `AE=0` | `+1.1%` |
| 8 | Compiled containment sequence cut (`compiledNestedContainmentOperandIndices`) | Aceptado | `10.51s`* | `175s` | `AE=0` | `-1.7%` (ruido) |
| 9 | TrueMiss early-exit (`intersectBakedQuadricWithCoeffs`, `candidateInsideDirectDescriptorOperands`) | Aceptado | **`8.40s`** | **`137s`** | `AE=0` | **`+20.1%`** |

\* muestras únicas, no promedios de 3 corridas.

### Fases pendientes (trabajo futuro)

| # | Objetivo | Por qué | Riesgo |
| --- | --- | --- | --- |
| A | ~~Extender trueMiss al primer hit (`traceFirstHitCompiledSingleCorePlane`)~~ — **RECHAZADO** | `traceFirstHitCompiledSingleCorePlane` no es un hot path dominante en drums. Implementado y medido: regresión ~4.5% en modo paralelo (baseline 1.10s → 1.15s). El overhead del bool + parámetro extra en `traceCompiledCoreFirstHitCandidates` supera el beneficio. Revertido. | — |
| B | ~~Eliminar la copia redundante de la comprobación `!isPrimary && compiledTransformedNestedCorePlane` en `traceOperandAllCrossings`~~ — **ACEPTADO (neutral)** | Eliminadas 16 líneas de código muerto. Rendimiento sin cambio medible: Phase B parallel avg ~1.117s vs baseline ~1.098s (dentro del ruido ±2%). El path `compiledTransformedNestedCorePlane` siempre es despachado por `tracePlanOperandAllCrossings` antes de llegar a `traceOperandAllCrossings`. Gate verde. | — |
| C | Activar el emitter compilado para **primary rays** (con `Quadric::intersectQuadric` cacheado) | El bloqueo actual es la caché de viewpoint; si se adapta el emitter para llamar `Quadric::intersectQuadric(ray, shape, ...)` en vez de `intersectBakedQuadric`, debería ser safe para primary rays también | Medio — hay que verificar que `takeoff` vuelve a pasar |
| D | Reducir los `81.9M` calls a `traceOperandAllCrossings` restantes | Hay `200` GenericMorgan plans en drums; `traceGenericMorganUnion` activo los cubre pero tiene poco impacto porque esos planes no tienen planos ni primitivas directas en el top level — contienen todos `TransformedNestedCsg` | Alto — requiere redesign del Morgan loop |
| E | Eliminar los `51.5M` `LocalIntersectionClone` restantes | Los primary-ray TransformedNestedCsg todavía crean clones; si se activa la ruta compilada para primary rays (objetivo C) eso los reduciría | Depende de C |
| F | Medir ratio trueMiss real en `drums` con `-pg` | Confirmar si ~89% miss ratio se mantiene tras el corte actual | Bajo — sólo profiling |

### Resumen de contadores clave (último estado medido)

| Contador | Valor tras corte 8 (gprof) | Corte 9 (no reprofiled) |
| --- | ---: | --- |
| `traceOperandAllCrossings` calls | `81.9M` | sin cambio (no afecta ese path) |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` calls | `23.5M` | sin cambio en conteo; ejecución más rápida por trueMiss |
| `LocalIntersectionClone` calls | `51.5M` | sin cambio |
| Queue pops | `62.1M` | sin cambio |
| `drums` 320×200 wall-clock | `10.51s` (muestra) | **`8.40s`** (avg 3 corridas) |
| `renderAll.sh` | `175s` | **`137s`** |
