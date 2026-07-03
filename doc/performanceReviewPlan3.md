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

No tools are available in this response, so I cannot edit the file directly. However, here is the exact text that should be added to the plan — the main agent can copy it when resuming the work:

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

## Current Code State in src (2026-07-03, Phases B–E included)

### File Locations After Refactoring (staged, pending commit)

The `Baked*.cpp/h` files have been moved from `src/render/` to `src/render/bakedScene/`.
The current HEAD (`9a2d86e`) still has the files in `src/render/`; the refactoring is staged but not committed.

| File | Location in HEAD (`9a2d86e`) | Location after refactoring commit |
| --- | --- | --- |
| `BakedCsgTracing.cpp/h` | `src/render/` | `src/render/bakedScene/` |
| `BakedCompositeTracing.cpp/h` | `src/render/` | `src/render/bakedScene/` |
| `BakedSimpleBodyTracing.cpp/h` | `src/render/` | `src/render/bakedScene/` |
| `BakedTracingCommon.cpp/h` | `src/render/` | `src/render/bakedScene/` |
| `CsgScratchContext.h` | did not exist | `src/render/bakedScene/` (new) |

Additionally, anonymous namespaces in the `.cpp` files were converted to classes with private static methods, and `CsgScratchContext` was extracted to its own `.h` with an explicit constructor and inline getters.

Refactoring performance: drums 1280×800 -parallel: baseline `9a2d86e` = 12.78 s → with refactoring = 12.84 s (+0.06 s, noise). Gate green (`Test passed.`).

### Quadric Functions in `src/render/bakedScene/BakedCsgTracing.cpp`

| Function | Viewpoint cache | `trueMiss` | Extra outputs | Main callers |
| --- | --- | --- | --- | --- |
| `intersectBakedQuadric` | ✓ for primary rays (Phase E) | ✗ | none | `traceOperandAllCrossings` (TransformedQuadric), `traceCompiledCoreOperandAllCrossings`, `traceCompiledCoreFirstHitCandidates`, emitter (transformedCoreQuadric branch) |
| `intersectBakedQuadricWithTrueMiss` | ✓ for primary rays (Phase E) | ✓ (`bool &`) | none | specialized emitter (directCoreQuadric branch) — **sole hot path in drums** |
| `intersectBakedQuadricWithCoeffs` | ✓ for primary rays (Phase E) | ✓ (`bool &`) | `polyA`, `polyB`, `polyC` | `traceMorganIntersectionGeneric` (Phase D pre-scan) |

### Active Optimizations in `src/render/bakedScene/BakedCsgTracing.cpp`

| Function / mechanism | Description |
| --- | --- |
| `intersectBakedQuadric` | Quadric intersection with explicit origin/direction. `objectVpConstant` cache for primary rays (Phase E); trueMiss threshold not available. |
| `intersectBakedQuadricWithTrueMiss` | Same as `intersectBakedQuadric` + `trueMiss` flag (`disc < -4·A·SMALL_TOLERANCE`) without coefficient outputs. Viewpoint cache included. New in Phase E. |
| `intersectBakedQuadricWithCoeffs` | Same as above but exposes `polyA/B/C`. Viewpoint cache included (Phase E). Used by the Phase D pre-scan in `traceMorganIntersectionGeneric`. |
| `candidateInsideCompiledNestedContainmentSequence` | Generic containment iterating `compiledNestedContainmentOperandIndices`. |
| `candidateInsideDirectDescriptorOperands` | Inlined direct-plane + direct-quadric containment without switch; avoids the generic path for 100% of hits in drums. |
| `tracePlanOperandAllCrossings` | Dispatcher: if `compiledTransformedNestedCorePlane` (all rays, primary and non-primary — Phase C removed the gate), dispatches to the emitter. Otherwise calls `traceOperandAllCrossings`. |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` | Compiled emitter for TransformedNestedCsg with core=quadric+planes. `directCoreQuadric` branch: calls `intersectBakedQuadricWithTrueMiss` + `candidateInsideDirectDescriptorOperands` (Phase E). `transformedCoreQuadric` branch: calls `intersectBakedQuadric` + `candidateInsideCompiledNestedContainmentSequence`. Active for **all rays** (Phase C). |
| `traceFirstHitCompiledSingleCorePlane` | Compiled first-hit for `SingleCorePlaneIntersection` plans. Not hot in drums. |
| `traceGenericMorganUnion` | Specialized union traversal; iterates `executionPlanPlaneOperandIndices` and `executionPlanDirectPrimitiveOperandIndices`. In drums, GenericMorgan UNIONs only have TransformedPrimitive/Quadric at the top level. |
| `traceMorganIntersectionGeneric` | Phase D pre-scan: if any positive TransformedQuadric (`polyA>0`) gives trueMiss, the entire INTERSECTION fails → early return. Neutral in drums (X-Tube rarely evaluated due to `bounded_by`). |

All these symbols are in `src/render/bakedScene/BakedCsgTracing.cpp` (after the staged refactoring) or in `src/render/BakedCsgTracing.cpp` (in HEAD `9a2d86e` before the refactoring commit).

### Precomputed Metadata in `Scene::BakedCsgOperand`

| Field | Type | Content |
| --- | --- | --- |
| `compiledTransformedNestedCorePlane` | `bool` | true if this TransformedNestedCsg operand references a SingleCorePlane plan with a quadric core |
| `compiledNestedCoreOperandIndex` | `int` | index of the core operand within `nestedCsg.operands` |
| `compiledNestedCoreDirectQuadric` | `bool` | true if the core is `DirectAnnotatedPrimitive` with `quadricGeometry != nullptr` |
| `compiledNestedCoreTransformedQuadric` | `bool` | true if the core is `TransformedQuadric` |
| `compiledNestedPlaneOperandIndices` | `ArrayList<int>` | indices of the plane operands of the nestedCsg |
| `compiledNestedContainmentOperandIndices` | `ArrayList<int>` | `[coreIndex, plane1, plane2, ...]` — precomputed containment sequence |

### Changes in `src/environment/geometry/volume/Quadric.h` (Phase E)

The `objectVpConstant` and `constantCached` fields are now `mutable` and their setters are const-correct, allowing the cache to be updated from functions that receive `const Quadric &shape`. The semantics are identical to `Quadric::intersectQuadric`; there is no coherence risk because the `directCoreQuadric` of `compiledTransformedNestedCorePlane` operands are only accessed via the specialized emitter (never via `Quadric::intersectQuadric` in the same render).

### Execution Plan Inventory for `drums` (unchanged since Phase 9)

| Metric | Count |
| --- | ---: |
| GenericMorgan plans | `200` |
| SingleCorePlaneIntersection plans | `250` |
| TransformedNestedCsg operands with compiled descriptor | `124` |
| Of those: DirectQuadric core | `124` |
| Of those: TransformedQuadric core | `0` |
| copied `compiledNestedPlaneOperandIndices` | `248` |
| copied `compiledNestedContainmentOperandIndices` | `372` |

### What Remains Disabled / Rejected

- **TrueMiss in `traceFirstHitCompiledSingleCorePlane`** (Phase A): rejected; `traceFirstHitCompiledSingleCorePlane` is not a hot path in drums — overhead > benefit. Reverted.
- **Direct emitter prototype for nested/simple-body**: rejected; `LocalIntersectionClone` count dropped but wall-clock increased and `takeoff.tga` failed (`AE=9765`).
- **Phase C reverted by Path G** (2026-07-03): the `!ray->isPrimaryRayEnabled()` gate was restored in `tracePlanOperandAllCrossings`. Primary rays return to the old path. Phase C regression (−6.2%) partially recovered: drums 8.92→8.55 s (+4.2%), renderAll 151→141 s.

## Plan Progress Table

### Executed Phases and Final Status

| # | Cut | Status | `drums` 320×200 | `renderAll` | Image | Δ vs previous |
| --- | --- | --- | ---: | ---: | --- | --- |
| 1 | Compile CSG execution plans (metadata + statistics) | Accepted | `9.63s` | n/m | AE=0 | −1.1% |
| 2 | Direct core-plus-plane all-crossings (first attempt) | Structural | `10.07s` | n/m | AE=0 | −4.6% |
| 3 | Direct first-hit compiled path | Structural | `10.20s` | `184s` | AE=0 | −0.1% |
| 4a | Nested/simple-body clone-reduction (direct emitter) | **Rejected** — `takeoff` AE=9765 | `11.21s` | — | ✗ | — |
| 4b | Descriptor compilation (`compiledNestedCoreDirectQuadric`, etc.) | Accepted (neutral) | `10.91s`* | `187s` | AE=0 | ~0% |
| 5 | Non-primary emitter (`traceTransformedNestedSingleCorePlaneOperandAllCrossings`) | Accepted | `10.91s`* | `179s` | AE=0 | LocalIntersectionClone 75M→51.5M |
| 6 | Parent-plan dispatch (`tracePlanOperandAllCrossings`) | Accepted | `10.45s`* | `171s` | AE=0 | **+4.2%** |
| 7 | Nested plane-role descriptor (`compiledNestedPlaneOperandIndices`) | Accepted | `10.33s`* | `169s` | AE=0 | +1.1% |
| 8 | Compiled containment sequence (`compiledNestedContainmentOperandIndices`) | Accepted | `10.51s`* | `175s` | AE=0 | −1.7% (noise) |
| 9 | TrueMiss early-exit (`intersectBakedQuadricWithCoeffs`, `candidateInsideDirectDescriptorOperands`) | Accepted | **`8.40s`** | **`137s`** | AE=0 | **+20.1%** |
| A | TrueMiss in `traceFirstHitCompiledSingleCorePlane` | **Rejected** — not a hot path, 4.5% regression | — | — | — | — |
| B | Remove redundant `compiledTransformedNestedCorePlane` copy in `traceOperandAllCrossings` | Accepted (neutral / cleanup) | ~`8.40s` | ~`137s` | AE=0 | ~0% |
| C | Activate emitter for **primary rays** (remove `!isPrimaryRayEnabled()` gate) | Accepted (**regression, then reverted by Path G**) | `8.92s` | `151s` | AE=0 | **−6.2%** drums |
| D | Pre-scan trueMiss in `traceMorganIntersectionGeneric` (INTERSECTION with positive quadric) | Accepted (neutral in drums) | ~`8.92s` | ~`151s` | AE=0 | ~0% |
| E | Viewpoint cache in `intersectBakedQuadric`/`WithCoeffs`; new `intersectBakedQuadricWithTrueMiss`; emitter uses lightweight function | Accepted (partial improvement) | **`8.92s`** | **`151s`** | AE=0 | +1.7% vs Phase E v1 |

\* single samples, not averages of 3 runs.

**Net state after Phases A–F:** drums `8.92s` (+6.2% regression from Phase C vs Phase 9). **Path G executed (2026-07-03):** regression partially recovered → drums `8.55s`, renderAll `141s`.

### Executed Phases — F

| # | Objective | Status | `drums` 320×200 | `renderAll` | Δ |
| --- | --- | --- | ---: | ---: | --- |
| F | Reprofile `drums` with `-pg` — confirm time distribution after Phases B–E | **COMPLETED** — results in counter table | — | — | — |

**Phase F result (gprof `drums` 320×200, Phases A–E active):**

Top hotspots (total profile ~6s, 64K primary rays):

| Rank | Function | % time | Calls |
| ---: | --- | ---: | ---: |
| 1 | `traceOperandAllCrossings` | 15.3% | **79.3M** |
| 2 | `traceTransformedNestedSingleCorePlaneOperandAllCrossings` | 13.1% | **24.8M** |
| 3 | `traceAllCrossingsWithScratch` | 8.5% | 55.5M |
| 4 | `BakedSimpleBodyTracing::traceAllCrossings` | 6.5% | 28.8M |
| 5 | `tracePlaneOperandCandidate` | 6.0% | 34.5M |
| 6 | `candidateInsideDirectDescriptorOperands` | 5.0% | 53.1M |
| 7 | `rayIntersectsAabbForward` | 4.7% | 29.9M |
| 8 | `offerCompiledSingleCorePlaneFirstHitCandidate` | 3.2% | 23.7M |
| 9 | `traceAllCrossingsInCompositeSpace` | 3.2% | 14.9M |
| 10 | `PriorityQueuePool::pop` | 2.7% | 61.3M |
| 11 | `BakedCsgTracing::traceFirstHit` | 2.7% | 7.2M |
| 12 | `LocalIntersectionClone` ctor | 2.5% | **50.2M** |
| 13 | `passesBoundingShapes` | 2.5% | 36.3M |
| 14 | `BakedCsgTracing::traceAllCrossings` | 2.2% | 15.4M |
| 15 | `BakedCompositeTracing::traceAllCrossings` | 2.2% | 14.2M |

**Attribution of the 50.2M `LocalIntersectionClone` calls:**

| Caller | Calls | % of total |
| --- | ---: | ---: |
| `traceOperandAllCrossings` | 20.0M | 40% |
| `BakedSimpleBodyTracing::traceAllCrossings` | 17.0M | 34% |
| `BakedCompositeTracing::traceAllCrossings` | 11.8M | 23% |
| firstHit / doExtraInformation / misc | 1.4M | 3% |

**Phase F conclusions:**

1. **`intersectBakedQuadricWithTrueMiss` fully inlined** — does not appear in gprof as a separate function; the compiler merged it into `traceTransformedNestedSingleCorePlaneOperandAllCrossings`. The visible quadric functions are `Quadric::doIntersectionForAllRayCrossingsAnnotated` (3.0M, generic non-emitter path).

2. **24.8M emitter vs 23.5M Phase 8** — the +1.3M corresponds to primary rays now routed through the emitter (Phase C). The 50.2M − 51.5M = −1.3M fewer clones also correspond to this. The clone gain from Phase C was minimal: ~1.3M of 51.5M = **2.5%** of the total.

3. **The remaining 50.2M clones are structural** — distributed across 3 distinct paths (`traceOperandAllCrossings` 40%, `BakedSimpleBodyTracing::traceAllCrossings` 34%, `BakedCompositeTracing::traceAllCrossings` 23%). Each one needs the clone to transform the ray to the object's local space. These cannot be eliminated with the same specialized emitter approach.

4. **There is no single dominant attackable function** — time is distributed across ≥10 functions at 2–15% each. The residual is structural: the ray-transform + CSG evaluation architecture inherently distributes the work.

5. **`traceOperandAllCrossings` 79.3M calls** (slightly less than 81.9M in Phase 8, due to Phases B and D which removed operations and code paths). Still the largest hotspot but has no obvious "low-hanging fruit".

### Key Counter Summary (final state after Phases A–F)

| Counter | After Phase 8 (gprof) | After Phase 9 | After Phases A–F (final gprof state) |
| --- | ---: | ---: | --- |
| `traceOperandAllCrossings` calls | `84.1M` | `81.9M` | **`79.3M`** (−2.6M from Phases B+D) |
| `traceTransformedNestedSingleCorePlaneOperandAllCrossings` calls | `23.5M` | `23.5M` | **`24.8M`** (+1.3M primary rays from Phase C) |
| `LocalIntersectionClone` calls | `51.5M` | `51.5M` | **`50.2M`** (−1.3M from Phase C; 40% `traceOperandAllCrossings`, 34% `BakedSimpleBodyTracing::traceAllCrossings`, 23% `BakedCompositeTracing::traceAllCrossings`) |
| `candidateInsideDirectDescriptorOperands` calls | — | `53.1M` | `53.1M` (unchanged) |
| Queue pops | `62.1M` | `62.1M` | **`61.3M`** |
| `Quadric::doIntersectionForAllRayCrossingsAnnotated` calls | — | ~`3M` | **`3.0M`** (generic non-emitter path) |
| `drums` 320×200 wall-clock | `10.51s` | **`8.40s`** | `8.92s` with Phase C; **`8.55s` after Path G** |
| `renderAll.sh` | `175s` | **`137s`** | `151s` with Phase C; **`141s` after Path G** |

## Pending Optimization Paths (post-Phase F, 2026-07-02)

### Phase C Regression Diagnosis

Phase C (`!isPrimaryRayEnabled()` removed in `tracePlanOperandAllCrossings`) aimed to eliminate 51.5M `LocalIntersectionClone` calls by redirecting primary rays to the compiled emitter. But gprof Phase F shows it only eliminated **1.3M** of them (2.5% of the total): the remaining 50.2M come from `traceOperandAllCrossings`, `BakedSimpleBodyTracing::traceAllCrossings`, and `BakedCompositeTracing::traceAllCrossings`, which are entirely distinct paths from the emitter. The net cost was negative: the old path's viewpoint cache (`Quadric::intersectQuadric`) + better instruction footprint outweigh the small clone reduction.

### Path G — Revert Phase C (guaranteed, maximum recovery)

**Objective:** Restore the `!ray->isPrimaryRayEnabled()` gate in `tracePlanOperandAllCrossings`. Primary rays return to the old path (LocalIntersectionClone + `Quadric::intersectQuadric` with native cache).

**Expected effect:** `drums` 8.92→8.40s (−0.52s, +6.2%); `renderAll` 151→137s (−14s). Gate green (AE=0 unchanged).

**Risk:** Zero — it is reverting one line; the gate correctness was already proven in Phase 9.

**Code impact:** Keeps `intersectBakedQuadricWithTrueMiss` and the Phase E cache (useful for shadow rays); only the `tracePlanOperandAllCrossings` dispatch becomes conditional again.

---

### Path H — Sphere operand → TransformedQuadric in baking (eliminate 20M clones from `traceOperandAllCrossings`)

**Phase F analysis + code reading (2026-07-02):**

In `traceOperandAllCrossings` (`BakedCsgTracing.cpp`), the function `classifyBakedCsgOperand` (`Scene.cpp:88`) assigns `executionKind` as follows:
- Nested CSG → `TransformedNestedCsg` / `NestedCsg`
- `InfinitePlane` → `TransformedPlane` / `DirectPlane`
- With transform + `quadricGeometry != nullptr` → **`TransformedQuadric`** (does NOT create a clone)
- With transform + no quadric → **`TransformedPrimitive`** (creates clone, 20M in drums)
- Without transform → `DirectAnnotatedPrimitive` / `DirectPrimitive`

`baked.quadricGeometry` is filled via `dynamic_cast<Quadric *>(baked.geometry)` (`Scene.cpp:390`). A `Sphere` with a transform does not pass this cast → falls into `TransformedPrimitive` → creates `LocalIntersectionClone`.

**Fix:** A unit sphere centered at the origin is a quadric with exact coefficients: `object2Terms=(1,1,1)`, `objectMixedTerms=(0,0,0)`, `objectTerms=(0,0,0)`, `objectConstant=−1`. In `bakeCsgOperand` (`Scene.cpp`), when `geometry` is a `Sphere`, create a synthetic `Quadric` with those coefficients, store it in an ownership list of the compiled scene, and point `baked.quadricGeometry` to it. `classifyBakedCsgOperand` automatically returns `TransformedQuadric`, using the already-optimized `intersectBakedQuadric` path (with Phase E viewpoint cache). Zero changes in the tracing hot path.

**Implementation:**
1. Add `java::ArrayList<Quadric *> ownedSyntheticQuadrics` to `Scene::CompiledTracingScene` (or equivalent ownership)
2. In `bakeCsgOperand` (`Scene.cpp`), after `baked.quadricGeometry = dynamic_cast<Quadric *>(...)`:
   ```cpp
   if (baked.quadricGeometry == nullptr && baked.hasTransform) {
       if (Sphere *sphere = dynamic_cast<Sphere *>(baked.geometry)) {
           double constant = sphere->isInverted() ? 1.0 : -1.0;
           Quadric *q = new Quadric(
               Vector3Dd(1.0, 1.0, 1.0),  // object2Terms: x²+y²+z²
               Vector3Dd(0.0, 0.0, 0.0),  // objectMixedTerms
               Vector3Dd(0.0, 0.0, 0.0),  // objectTerms
               constant);                  // -1 = unit sphere, +1 = inverted
           compiledScene.ownedSyntheticQuadrics.add(q);
           baked.quadricGeometry = q;
       }
   }
   ```
3. Destroy the `Quadric *` when cleaning up the compiled scene
4. The containment test (`containmentTestOperand`, case `TransformedQuadric`) already works with the new quadric — nothing else needs to change

**Handling `Sphere::isInverted()`:** The Quadric with `constant=+1` represents the sphere's exterior as interior (inverts membership). Verify that `Quadric::doContainmentTest` handles this correctly; if not, add `invertGeometry()` to the synthetic Quadric.

**Risk:** Low — only changes the classifier at scene compile time; the tracing hot path does not change. Correctness is verifiable via gate (AE=0).

**Limitation:** Only eliminates clones for `Sphere` CSG operands with a transform. `TransformedPrimitive` of type Box, Torus, etc. continue creating clones (but are less frequent in drums).

---

### Path H-bis — Sphere CSG operand without clone via `Sphere::intersectSphereLocalSpace`

**Diagnosis (Phase F):** 20M of the 50.2M `LocalIntersectionClone` calls come from
`traceOperandAllCrossings` (40% of the total). Within that path, `TransformedPrimitive`
operands of type `Sphere` create a clone to transform the ray to local space and call
`Sphere::doIntersectionForAllRayCrossings`. The same operation can be done without a clone
if `intersectSphere` accepts explicit origin/direction.

**Reason Path H (synthetic Quadric) was rejected:** `intersectBakedQuadric` uses the
discriminant formula (`4(d·p)² − 4|d|²(|p|²−1)`) while `intersectSphere` uses the chord
formula (`tHalfChordSquared = 1 − |p|² + tc²`). They differ by ~1 ULP at boundary cases;
CSG is sensitive to the exact order of t values → 35 gate failures.

**Correct fix:** Extract the math from `intersectSphere` into a public static method
`intersectSphereLocalSpace(origin, direction, stats, &d1, &d2)` that does not require
`RayWithSegments`. Then `intersectSphere` delegates to it; and in `traceOperandAllCrossings`
the path for `TransformedSphere` calls `intersectSphereLocalSpace` directly with
`localToObject.transformPoint/Direction(ray)`. Numerically identical to the clone because
it applies the same function to the same input values.

**Files and changes:**

1. **`src/environment/geometry/volume/Sphere.h`** — add public static method:
   ```cpp
   static bool intersectSphereLocalSpace(
       const Vector3Dd &origin,
       const Vector3Dd &direction,
       Statistics *stats,
       double *depth1,
       double *depth2);
   ```
   Requires `#include "common/statistics/Statistics.h"` if not already included.

2. **`src/environment/geometry/volume/Sphere.cpp`** — move the body of `intersectSphere`
   to `intersectSphereLocalSpace`; have `intersectSphere` delegate:
   ```cpp
   bool Sphere::intersectSphereLocalSpace(
       const Vector3Dd &p, const Vector3Dd &d,
       Statistics *stats,
       double *depth1, double *depth2)
   {
       stats->incrementRaySphereTests();
       const double directionLength = java::Math::sqrt(d.dotProduct(d));
       const Vector3Dd unitDirection = d.multiply(1.0 / directionLength);
       const double ocSquared = p.dotProduct(p);
       const bool inside = (ocSquared < 1.0);
       const double tClosestApproach = -p.dotProduct(unitDirection);
       if (!inside && tClosestApproach < Config::SMALL_TOLERANCE) { return false; }
       const double tHalfChordSquared =
           1.0 - ocSquared + tClosestApproach * tClosestApproach;
       if (tHalfChordSquared < Config::SMALL_TOLERANCE) { return false; }
       const double halfChord = java::Math::sqrt(tHalfChordSquared);
       *depth1 = (tClosestApproach + halfChord) / directionLength;
       *depth2 = (tClosestApproach - halfChord) / directionLength;
       if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
           if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE))
               return false;
           *depth1 = *depth2;
       } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
           *depth2 = *depth1;
       }
       stats->incrementRaySphereTestsSucceeded();
       return true;
   }

   bool Sphere::intersectSphere(const RayWithSegments *ray, const Sphere *,
       double *depth1, double *depth2)
   {
       return intersectSphereLocalSpace(
           ray->getOrigin(), ray->getDirection(),
           ray->getStatistics(), depth1, depth2);
   }
   ```

3. **`src/environment/scene/Scene.h`** — add `TransformedSphere` to the enum:
   ```cpp
   enum class BakedCsgOperandExecutionKind {
       ...
       TransformedQuadric,
       TransformedSphere,   // ← new
       TransformedPrimitive,
       ...
   };
   ```

4. **`src/environment/scene/Scene.cpp`** — in `classifyBakedCsgOperand`, before
   returning `TransformedPrimitive`:
   ```cpp
   if (baked.hasTransform) {
       if (baked.quadricGeometry != nullptr)
           return Scene::BakedCsgOperandExecutionKind::TransformedQuadric;
       if (dynamic_cast<Sphere *>(baked.geometry) != nullptr)
           return Scene::BakedCsgOperandExecutionKind::TransformedSphere;
       return Scene::BakedCsgOperandExecutionKind::TransformedPrimitive;
   }
   ```
   In the `executionPlanTransformedPrimitiveOperandIndices` accumulation (around line 284-286 of
   Scene.cpp), add `TransformedSphere` to the same case:
   ```cpp
   case Scene::BakedCsgOperandExecutionKind::TransformedQuadric:
   case Scene::BakedCsgOperandExecutionKind::TransformedSphere:   // ← new
   case Scene::BakedCsgOperandExecutionKind::TransformedPrimitive:
       baked.executionPlanTransformedPrimitiveOperandIndices.add((int)i);
   ```
   If there are other `switch` statements over `BakedCsgOperandExecutionKind` in `Scene.cpp`, also
   add the case there.

5. **`src/render/bakedScene/BakedCsgTracing.cpp`** — three locations:

   a) **`containmentTestOperand`** (around line 800): add `TransformedSphere` to the same case:
   ```cpp
   case Scene::BakedCsgOperandExecutionKind::TransformedQuadric:
   case Scene::BakedCsgOperandExecutionKind::TransformedSphere:   // ← new
   case Scene::BakedCsgOperandExecutionKind::TransformedPrimitive:
       return operand.geometry->doContainmentTest(
           operand.localToObject.transformPoint(point), distanceTolerance);
   ```

   b) **`traceOperandAllCrossings`** (around line 626): add a new block between the
   `TransformedQuadric` case (line 626) and the `LocalIntersectionClone` block (line 669).
   The new case is analogous to `TransformedQuadric` but using `intersectSphereLocalSpace`:
   ```cpp
   if (operand.executionKind ==
       Scene::BakedCsgOperandExecutionKind::TransformedSphere) {
       const Vector3Dd localOrigin =
           operand.localToObject.transformPoint(ray->getOrigin());
       const Vector3Dd localDirection =
           operand.localToObject.transformDirection(ray->getDirection());
       double depth1;
       double depth2;
       if (!Sphere::intersectSphereLocalSpace(
               localOrigin, localDirection,
               ray->getStatistics(), &depth1, &depth2)) {
           return false;
       }
       offerTransformedPrimitiveCandidate(
           operand, ray, effectiveMaterial, localOrigin, localDirection, depth1, depthQueue);
       if (depth2 != depth1) {
           offerTransformedPrimitiveCandidate(
               operand, ray, effectiveMaterial, localOrigin, localDirection, depth2, depthQueue);
       }
       return true;
   }
   ```
   Also, in the condition of the `if (operand.executionKind == TransformedPlane || ...)`
   block that surrounds the clone (lines 581-588), add `TransformedSphere` to the list so
   it reaches the new case BEFORE the `LocalIntersectionClone` is created.

   c) **`traceCompiledCoreOperandAllCrossings`** (around line 1312): add a `TransformedSphere`
   case analogous to the existing `TransformedQuadric` (uses `intersectSphereLocalSpace` +
   `offerTransformedPrimitiveCandidate`).

6. **`src/render/bakedScene/BakedCsgTracing.h`** — add `#include "Sphere.h"` if not present.
   The static methods of `BakedCsgTracing` do not need new signatures; they are called locally.

**Expected impact:** Eliminates the 20M `TransformedPrimitive` Sphere clones in
`traceOperandAllCrossings`. According to gprof Phase F, `LocalIntersectionClone` ctor = 2.5% of
total time (~0.15 s in a 6 s profile). 20M of 50.2M = 40% → estimated savings ~0.06 s
(minimum). The actual benefit may be greater due to reduced cache pressure.

**Risk:** Medium — correctness is verifiable via gate because the math is identical to the
clone path. The main source of error would be failing to add `TransformedSphere` to some
switch that iterates execution kinds; the compiler will warn if the enum is not handled in
an exhaustive switch.

---

### Path I — Reduce `BakedSimpleBodyTracing::traceAllCrossings` 17M clones

**Diagnosis (Phase F):** `BakedSimpleBodyTracing::traceAllCrossings` (28.8M calls, 6.5%)
creates 17M `LocalIntersectionClone` calls — one per simple body with `hasObjectTransform`. In
drums, these correspond to the outer "skins" of the cylinders (direct SimpleBody over the
cylinder quadric, without CSG).

**Current structure (`BakedSimpleBodyTracing.cpp`):**
```
traceAllCrossings:
  if hasObjectTransform:
      objectRay = LocalIntersectionClone(*ray)     ← CLONE 1 (the 17M)
      objectRay.origin = worldToObject.transformPoint(ray->getOrigin())
      objectRay.direction = worldToObject.transformDirection(ray->getDirection())
  traceInObjectSpace(objectRayPtr):
      passesBoundingShapes(baked, ..., objectRayPtr)   ← requires RayWithSegments
      if hasGeometryTransform:
          geometryRay = LocalIntersectionClone(*objectRayPtr)  ← CLONE 2 (conditional)
          ...
      traceInGeometrySpace(geometryRayPtr):
          localDepthQueue = pool->pop(128)
          if bakedCsgIndex >= 0:
              BakedCsgTracing::traceAllCrossings(...)
          else:
              geometry->doIntersectionForAllRayCrossings(geometryRayPtr, ...)
          for each candidate: finalizeCandidate(...)
              → transforms point, computes t, checks clipping shapes
```

**Why the clone is necessary in the general case:**
`passesBoundingShapes` calls `BakedTracingCommon::traceObjectFirstHit` which requires a
complete `RayWithSegments *` (with statistics pool). If the simple body has bounding shapes,
the clone is unavoidable with the current API.

**Optimizable case (restricted scope):**
```
!baked.hasBoundingShapes      → no traceObjectFirstHit → clone not needed for the bounding test
!baked.hasClippingShapes      → finalizeCandidate does not do containment test on clipping shapes
baked.bakedCsgIndex < 0       → geometry is a direct primitive, not a nested CSG
baked.geometry is a Quadric   → we can use intersectBakedQuadric with explicit origin/direction
```

For this case, the code would be:
```cpp
// New path in traceAllCrossings, before the clone:
if (!baked.hasBoundingShapes && !baked.hasClippingShapes && baked.bakedCsgIndex < 0) {
    Quadric *q = dynamic_cast<Quadric *>(baked.geometry);  // avoid at runtime → see baking
    if (q != nullptr) {
        const Vector3Dd worldOrigin = ray->getOrigin();
        const Vector3Dd worldDir    = ray->getDirection();
        const Vector3Dd localOrigin =
            baked.hasObjectTransform ?
            baked.worldToObject.transformPoint(worldOrigin) : worldOrigin;
        const Vector3Dd localDir =
            baked.hasObjectTransform ?
            baked.worldToObject.transformDirection(worldDir) : worldDir;
        const Vector3Dd geomOrigin =
            baked.hasGeometryTransform ?
            baked.objectToGeometry.transformPoint(localOrigin) : localOrigin;
        const Vector3Dd geomDir =
            baked.hasGeometryTransform ?
            baked.objectToGeometry.transformDirection(localDir) : localDir;
        double d1, d2;
        if (!intersectBakedQuadric(*q, ray, geomOrigin, geomDir, &d1, &d2))
            return false;
        // Build and emit candidate directly (similar to offerTransformedPrimitiveCandidate)
        // ... (geometry space → object space → world space transforms)
        return true;
    }
}
// fall through to existing clone path
```

**To avoid the runtime `dynamic_cast`:**
Add a `Quadric *quadricGeometry = nullptr` field to `Scene::BakedSimpleBody` and assign it
during baking (in `bakeBakedSimpleBody`, immediately after `baked.geometry = ...`):
```cpp
baked.quadricGeometry = dynamic_cast<Quadric *>(baked.geometry);
```
Then in `traceAllCrossings`, use `baked.quadricGeometry != nullptr` instead of the
runtime `dynamic_cast`.

**Complexity of emitting candidates without `finalizeCandidate`:**
`finalizeCandidate` applies (if `hasGeometryTransform`) `geometryToObject.transformPoint`, then
(if `hasObjectTransform`) `objectToWorld.transformPoint`, and computes `t`. These can be
replicated inline in the fast path. The candidate point in geometry space is
`geomOrigin + geomDir * depth`; transformed to world space:
```cpp
Vector3Dd geomPoint = geomOrigin.add(geomDir.multiply(depth));
Vector3Dd objectPoint = baked.hasGeometryTransform ?
    baked.geometryToObject.transformPoint(geomPoint) : geomPoint;
Vector3Dd worldPoint = baked.hasObjectTransform ?
    baked.objectToWorld.transformPoint(objectPoint) : objectPoint;
candidate.getIntersection().point = worldPoint;
candidate.getIntersection().t =
    worldPoint.subtract(worldOrigin).dotProduct(worldDir) / worldDir.dotProduct(worldDir);
```

**Attributes** of the candidate: the same ones that `doIntersectionForAllRayCrossings` sets
(material = `baked.geometryMaterial`), plus `setObjectTexture`, `setObjectColor`,
`setNoShadowFlag`, `setHitBody` that `finalizeCandidate` applies. Since we skip
`finalizeCandidate`, they must be set directly.

**Risk:** Medium-high. The optimized path bypasses `passesBoundingShapes`, `finalizeCandidate`,
and the intermediate queue pool. Any combination of flags (`hasGeometryTransform` but not
`hasObjectTransform`, etc.) requires careful testing. The correctness gate is the safest
verification method. Start only with the `!hasGeometryTransform && !hasObjectTransform` case
(the simplest) and measure before extending.

**Estimated ROI:** 17M clones = 34% of the 50.2M, but `LocalIntersectionClone` is only 2.5%
of total time → maximum savings ~0.05 s. Likely low ROI. Execute J first.

---

### Path J — TrueMiss in `traceSingleCorePlaneIntersection` to skip the plane loop

**Diagnosis (Phase F):** `tracePlaneOperandCandidate` receives 34.5M calls (6.0% of time).
These come from the plane loop in `traceSingleCorePlaneIntersection`
(`BakedCsgTracing.cpp` lines 1842-1861), which ALWAYS runs for each
`SingleCorePlaneIntersection` CSG, regardless of whether the core intersects the ray.

**Why it is safe to skip:** The CSG is an INTERSECTION. The condition for a plane candidate
to be valid is: the impact point on the plane must be INSIDE the core. If the ray does not
cross the core at any positive point (core trueMiss), no plane point can be inside the core
→ all plane candidates would fail the containment test → the entire loop is wasted work.

**Structure of `traceSingleCorePlaneIntersection` (lines 1791-1863):**
```
if canUseCompiledSingleCorePlanePlan:
    if coreOperand.executionKind == TransformedQuadric:
        anyIntersectionFound = traceTransformedQuadricCorePlaneIntersection(...)  // ← core
    else:
        localDepthQueue = scratch.borrowQueue()
        traceCompiledCoreOperandAllCrossings(core, ..., localDepthQueue)  // ← core
        for each core candidate: validate against planes → offer
        scratch.returnQueue(localDepthQueue)

    // SHARED plane loop (line 1842) — ALWAYS runs:
    for p in executionPlanPlaneOperandIndices:
        if tracePlaneOperandCandidate(operand, ...) &&
           candidateInsideCompiledSingleCorePlaneOperands(...):
               offer
    return anyIntersectionFound
```

**Fix:** Obtain trueMiss from the core before the plane loop and exit if trueMiss.

For the `TransformedQuadric` case (line 1808):
`traceTransformedQuadricCorePlaneIntersection` does not currently return trueMiss. Add a
`bool &coreTrueMiss` output parameter or compute trueMiss BEFORE calling it:
```cpp
// Option A: compute trueMiss separately (simpler, does not touch traceTransformedQuadricCorePlaneIntersection)
bool coreTrueMiss = false;
if (coreOperand.executionKind == Scene::BakedCsgOperandExecutionKind::TransformedQuadric &&
    coreOperand.quadricGeometry != nullptr) {
    const Vector3Dd localOrigin =
        coreOperand.localToObject.transformPoint(ray->getOrigin());
    const Vector3Dd localDirection =
        coreOperand.localToObject.transformDirection(ray->getDirection());
    double d1, d2;
    intersectBakedQuadricWithTrueMiss(
        *coreOperand.quadricGeometry, ray, localOrigin, localDirection,
        &d1, &d2, coreTrueMiss);
}
if (!coreTrueMiss) {
    anyIntersectionFound = traceTransformedQuadricCorePlaneIntersection(...);
}
```

For the non-`TransformedQuadric` case (line 1819, `DirectAnnotatedPrimitive` with
`quadricGeometry != nullptr`):
```cpp
// After traceCompiledCoreOperandAllCrossings:
if (localDepthQueue->size() == 0 &&
    coreOperand.quadricGeometry != nullptr) {
    // Verify trueMiss to decide whether to skip the plane loop
    const Vector3Dd localOrigin = ray->getOrigin();  // DirectAnnotatedPrimitive: no transform
    const Vector3Dd localDirection = ray->getDirection();
    double d1, d2;
    bool trueMiss = false;
    intersectBakedQuadricWithTrueMiss(
        *coreOperand.quadricGeometry, ray, localOrigin, localDirection,
        &d1, &d2, trueMiss);
    if (trueMiss) coreTrueMiss = true;
}
```

Then, before the plane loop (line 1842):
```cpp
if (coreTrueMiss) {
    scratch.returnQueue(localDepthQueue);  // if applicable
    return anyIntersectionFound;
}
// usual plane loop...
```

**Note on `DirectAnnotatedPrimitive`:** These operands have `executionKind ==
DirectAnnotatedPrimitive` and `quadricGeometry != nullptr` (the 124 cores in drums). The ray
is passed untransformed (they have no `localToObject`, or it is identity). If the core is
`DirectAnnotatedPrimitive`, `localOrigin = ray->getOrigin()` and `localDirection =
ray->getDirection()`.

**For the `DirectAnnotatedPrimitive` case without quadricGeometry:** No trueMiss is available.
The plane loop must run. Using `localDepthQueue->size() == 0` as a heuristic only if the ray
is clearly outside the core's bounding box — not precise enough to use as trueMiss.
Therefore: ONLY activate the skip when `quadricGeometry != nullptr`.

**Expected impact:** The 34.5M `tracePlaneOperandCandidate` calls — gprof Phase F shows
`rayIntersectsAabbForward` at 29.9M calls (4.7%) just below. If in drums 89% of shadow
rays miss the cylinder core (behavior observed in Phase 9), the skip would eliminate ~30M
of the 34.5M calls. Gain estimate: 6.0% × 89% ≈ **5.3% of total profile**, or ~0.3 s in
drums 320×200. Potentially the largest gain of the pending paths.

**Risk:** Medium. The skip is semantically correct for INTERSECTION CSG (does not apply to
UNION or DIFFERENCE). `SingleCorePlaneIntersection` is always an INTERSECTION by construction
(`BakedCsgSpecialization::SingleCorePlaneIntersection` implies the operation is
`BooleanSetOperations::INTERSECTION`). The `coreTrueMiss` condition can only activate when
the ray clearly does NOT cross the core (very negative discriminant), which is much more
conservative than simply "the core returned no hits".

**Note: `intersectBakedQuadricWithTrueMiss` in Option A calls the intersection twice
(once for trueMiss, once inside `traceTransformedQuadricCorePlaneIntersection`).** To
avoid double computation: in Option B, add `bool &coreTrueMiss` to
`traceTransformedQuadricCorePlaneIntersection` and propagate it from its internal
intersection. Option A is simpler and correct despite doing duplicate work; start with A
to verify correctness and measure before optimizing to B.

---

### Phase H Implementation Status (2026-07-03) — REJECTED

Phase H was implemented and exhaustively investigated; it is rejected due to numerical
incompatibility.

**Root cause:** `intersectBakedQuadric` (synthetic sphere quadric) produces depth results
numerically DIFFERENT from `Sphere::intersectSphere` at the same points. The quadric
algorithm uses the direct discriminant formula (`4(d·p)² - 4|d|²(|p|²-1)`) while the
sphere uses the chord formula (`tHalfChordSquared = 1 - |p|² + tc²`). Mathematically
equivalent; numerically differ by ~1 ULP at boundary cases. CSG operates with ray segments
bounded by these t values — ULP differences change which value exceeds `SMALL_TOLERANCE`,
shifting the segment order and producing incorrect boolean results. In the suite: 35 gate
failures with AEs up to 600K.

**Attempted and discarded variants:**

1. `classifyBakedCsgOperand` only gives `TransformedQuadric` to Sphere when the geometry is
   actually a Quadric → gate passes, but the sphere path remains `TransformedPrimitive` with
   clone. The synthetic Quadric is created but not used in intersection → no-op.

2. Inlining `intersectBakedQuadric` in the `TransformedPrimitive` path with
   `quadricGeometry != nullptr`: same numerical problem — gives different t values, 35 gate
   failures.

3. The idea of using `Sphere::intersectSphere` directly without a clone is not feasible
   because `intersectSphere` is `private` in Sphere and requires a `RayWithSegments *` as
   a carrier for statistics and `isPrimaryRay`.

**To implement Phase H correctly, one of the following is required:**
- Make `Sphere::intersectSphere` public and call it with local origin/direction directly,
  or add a `static bool intersectSphereLocalSpace(const Vector3Dd &origin, const Vector3Dd &direction, Statistics *, bool isPrimary, double *d1, double *d2)` method to Sphere.
- Numerically guarantee that `intersectBakedQuadric` of a unit sphere produces identically
  the same values as `intersectSphere` (requires unifying the formulas at the root).

**Refactored blocks that ARE delivered (gate green, 0 performance impact):**
- Package `src/render/bakedScene/` with all `Baked*.cpp/h` files
- namespace→static class conversion for `BakedCsgTracing`, `BakedCompositeTracing`, `BakedSimpleBodyTracing`, `BakedTracingCommon`
- `CsgScratchContext` extracted to its own `.h` with constructor, inline getters, `releaseAll()`
- Baseline drums -parallel: 12.78s (HEAD `9a2d86e`) → 12.84s with refactoring (+0.06s, noise)

## Path G — Revert Phase C (2026-07-03)

The `!ray->isPrimaryRayEnabled()` gate restored in `tracePlanOperandAllCrossings` (one line added to the existing condition).

drums 320×200 measurements after the change:

| Run | Time (s) |
| ---: | ---: |
| 1 | 8.69 |
| 2 | 8.61 |
| 3 | 8.56 |
| 4 | 8.56 |
| 5 | 8.53 |
| 6 | 8.56 |
| Average (last 3) | **8.55** |

| Metric | Before (Phase C active) | After (Phase C reverted) | Δ |
| --- | ---: | ---: | ---: |
| `drums` 320×200 | 8.92 s | **8.55 s** | −0.37 s (**+4.2%**) |
| `renderAll.sh` | 151 s | **141 s** | −10 s (**+6.6%**) |
| Image | — | AE=0 vs `66c2fe3` | `Test passed.` |

Note: the original Phase 9 measured 8.40 s. The residual 0.15 s difference is due to Phases B, D, and E that remain active (the Phase E viewpoint cache adds one check per call to `intersectBakedQuadric` on secondary rays).

### Prioritized Summary

| # | Path | Status | Estimated effect | Effort | Risk |
| --- | --- | --- | --- | --- | --- |
| G | Revert Phase C | **Accepted** (2026-07-03) — drums 8.92→8.55 s (+4.2%); renderAll 151→141 s | — | — | — |
| H | Sphere CSG operand → TransformedQuadric in baking | **REJECTED** (2026-07-03) — `intersectBakedQuadric` ≠ `intersectSphere` numerically; 35 gate failures. Reverted. See Phase H section above. Retry only after exposing `Sphere::intersectSphere` as a static API without clone. | — | — | — |
| H-bis | Sphere without clone via `Sphere::intersectSphereLocalSpace` (new API) | Pending — requires API change in `Sphere.h` | ~10% if 20M clones eliminated | Medium | Medium |
| I | Specialize `BakedSimpleBodyTracing` without clone (17M clones, 34%) | Pending | ~5% if 17M clones compensate | High | Medium-high |
| J | trueMiss in `tracePlaneOperandCandidate` (53.1M calls, 5%) | Pending | ~3% if 30% of 34.5M calls are skipped | Medium | Medium |

**Path G executed (2026-07-03).** Next options in order of risk/effort: H-bis (expose `Sphere::intersectSphereLocalSpace`), J (trueMiss in `tracePlaneOperandCandidate`), I (`BakedSimpleBodyTracing` without clone).

### Global Progress Table (updated 2026-07-03)

| # | Cut | Status | `drums` 320×200 | `renderAll` | Δ time |
| --- | --- | --- | ---: | ---: | ---: |
| 1 | Compile CSG execution plans | Accepted | 9.63 s | — | +1.1% |
| 2 | Direct core-plus-plane all-crossings | Structural, regressive | 10.07 s | — | −4.6% |
| 3 | Direct first-hit compiled path | Structural, regressive | 10.20 s | 184 s | −0.1% |
| 4a | Nested/simple-body direct emitter | **Rejected** (`takeoff` AE=9765) | — | — | — |
| 4b | Descriptor compilation | Accepted (neutral) | 10.91 s† | 187 s | ~0% |
| 5 | Non-primary emitter | Accepted | 10.91 s† | 179 s | Clone 75M→51.5M |
| 6 | Parent-plan dispatch | Accepted | 10.45 s† | 171 s | +4.2% |
| 7 | Nested plane-role descriptor | Accepted | 10.33 s† | 169 s | +1.1% |
| 8 | Compiled containment sequence | Accepted (neutral) | 10.51 s† | 175 s | −1.7% (noise) |
| 9 | TrueMiss early-exit | **Accepted — largest gain** | **8.40 s** | **137 s** | **+20.1%** |
| A | TrueMiss in `traceFirstHitCompiledSingleCorePlane` | **Rejected** — not a hot path | — | — | −4.5% |
| B | Remove redundant dispatcher copy | Accepted (neutral) | ~8.40 s | ~137 s | ~0% |
| C | Activate emitter for primary rays | Accepted (**regression, reverted by G**) | 8.92 s | 151 s | **−6.2%** |
| D | Pre-scan trueMiss `traceMorganIntersectionGeneric` | Accepted (neutral) | ~8.92 s | ~151 s | ~0% |
| E | Viewpoint cache `intersectBakedQuadric*` | Accepted, partial improvement | 8.92 s | 151 s | +1.7% vs E-v1 |
| F | Reprofile `gprof` | Completed (analysis) | — | — | — |
| H | Sphere→TransformedQuadric | **Rejected** (numerical, 35 failures) | — | — | — |
| Refact. | `src/render/bakedScene/` package + `CsgScratchContext` | **Committed** (`d23b75f`) | no impact | no impact | ~0% |
| G | Revert Phase C (`!isPrimaryRayEnabled()` restored) | **Accepted** — pending commit | **8.55 s** | **141 s** | **+4.2%** |

† single sample, not an average of 3 runs.

**Current net state (post-G):** drums `8.55 s` (−1.19 s = **−12.2%** vs baseline `66c2fe3` 9.74 s). Residual vs Phase 9 (8.40 s): +0.15 s from active Phases B/D/E.
