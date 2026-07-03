# Performance Review Plan 6: Full Rebuild of `render/bakedScene` Around a Truly Baked Model

## Position in the Plan Sequence

Second of six (Plans 5–10; see the sequence table in
`doc/performanceReviewPlan5.md`). Requires Plan 5 completed: transformed
quadric/sphere/plane operands already collapse to world-space baked copies,
and `BakedGeometryBaker` exists.

## Decision Being Executed

Per the Plan 4 conclusions and the accepted decisions for this cycle:

- The current `render/bakedScene` layer (`BakedCsgTracing`,
  `BakedSimpleBodyTracing`, `BakedCompositeTracing`, `BakedTracingCommon`,
  `CsgScratchContext`) **may be destroyed completely and re-implemented from
  scratch**. It grew by accretion across Plans 1–3 into a trace-time
  interpreter of routing metadata: 93 hot functions, per-ray `executionKind`
  re-reads, per-ray effective-material re-resolution, 5–7 arguments per call
  level.
- The replacement must be a layer whose baked model **guarantees the math is
  actually baked**: nothing that is constant across rays (transforms,
  execution kinds, plan selection, effective materials, bounds, operand
  ordering) is ever read, branched on, or resolved per ray. The baked model
  is the *newly created coefficient rewriting* copy from Plan 5 plus
  pre-resolved routing — not "routing metadata only".
- The `environment/geometry` layer stays as it is. The decoupling between
  geometry and baking is preserved: the new layer consumes `Geometry`
  read-only and owns all baked state itself.
- The parsed model in `environment/scene` remains the complete model that
  `io` builds today. This plan **moves the `Baked*` structs out of
  `Scene.h` into the rebuilt layer**, so `environment/scene` returns to
  being the parsed-scene owner and `render/bakedScene` owns the baked
  representation end to end. (`Scene.h:21-174` currently hosts
  `CompiledTracingObject`, `BakedSimpleBody`, `BakedCsgOperand`,
  `BakedConstructiveSolidGeometry`, `BakedComposite`,
  `CompiledTracingScene` — all of that moves.)

## Reference Commits and Benchmarks

Identical to Plan 5 (baseline `4af1a75`, drums 320×200 primary, panel
secondary, same gprof recipe — use `drums_plan6_*` output names). The gate
reference state is the accepted-diff table produced at the end of Plan 5.

## Design of the New Layer

### Package layout

```
src/render/bakedScene/
    BakedScene.h / .cpp          — the owned baked model + builder entry point
    BakedGeometryBaker.h / .cpp  — carried over from Plan 5 unchanged
    BakedSceneBuilder.h / .cpp   — parse-model → baked-model compilation
    BakedTraceableObject.h       — one flat per-object record (see below)
    BakedCsgProgram.h / .cpp     — compiled CSG operand tables + plan record
    BakedTrace.h / .cpp          — the trace entry points RenderEngine calls
    (Plan 8 will add per-plan fused kernels next to BakedTrace)
```

The old five files are deleted at cutover (Phase 5), not incrementally
patched.

### The core rule: one dispatch per (ray × object)

Every constant-across-rays decision is resolved at build time into data:

1. **`BakedTraceableObject`** — a flat record per top-level traceable:

   ```cpp
   struct BakedTraceableObject {
       TraceKind kind;                 // read ONCE per (ray × object) at the
                                       // single top-level switch — the only
                                       // kind-branch on the hot path
       const Geometry *geometry;       // world-space: parsed direct geometry
                                       // or Plan 5 baked copy owned below
       const Material *effectiveMaterial; // fully resolved at build time:
                                       // object texture / geometry material /
                                       // scene default — never re-resolved per ray
       const ColorRgba *effectiveColor;
       AxisAlignedBox worldBounds;     // by value, hot
       bool bounded;
       bool castsShadow;
       int32_t csgProgramIndex;        // -1 unless kind == Csg
       int32_t compositeIndex;         // -1 unless kind == Composite
       // owned baked geometry copies (value storage, no pointer chase):
       Quadric bakedQuadricStorage;    // valid when the geometry pointer
                                       // points at it
       ...
   };
   ```

   `TraceKind` is a *small* enum over genuinely different trace algorithms
   (DirectPrimitive, Csg, Composite, BoundedGeneric, GenericFallback), not
   the current 8-way `BakedSimpleBodyExecutionKind` whose distinctions
   (transformed vs direct) Plan 5 already erased for the hot kinds.

2. **Material resolution is a build-time concern.** The current per-ray
   resolution (object texture vs geometry material vs default) happens in
   the tracing scaffolding today; in the new model
   `BakedSceneBuilder` computes `effectiveMaterial` once. The shading
   pipeline receives the resolved pointer from the hit record. If any scene
   feature makes the effective material genuinely ray-dependent (layered
   textures are per-point, not per-ray — they stay in shading), that feature
   is documented and kept out of this rule rather than silently breaking it.

3. **`BakedCsgProgram`** — per CSG object, flat operand tables in
   struct-of-arrays form where the kernels iterate:

   - `operandGeometry[]` (world-space `Geometry*`, baked copies included)
   - `operandMaterial[]` (resolved)
   - `operandBounds[]`, `operandCullSafe[]`
   - plane fast-path arrays: `planeNormal[]`, `planeDistance[]`
   - the plan kind (`GenericMorganUnion`, `MorganIntersection`,
     `SingleCorePlaneIntersection`, `TopLevelPlaneUnion`,
     `DisjointBoundedUnion`, `RaySegments`, `Fallback`) chosen once at build
   - residual transformed operands (Box/Torus/nested non-bakeable — the ones
     Plan 5 could not collapse) carry their matrix pair here, *grouped by
     transform class* so Plan 7 can key its cache on the class index
   - nested CSG children compile into their own `BakedCsgProgram` entries
     referenced by index (no pointers back into parsed `CsgOperand` on the
     hot path; the parsed pointer is kept in a cold debug field only)

4. **Scratch state** stays per-render-task (thread-compatible with
   `-parallel`, as today's `CsgScratchContext`), owned by the new
   `BakedTrace` entry points.

### What this plan does NOT change

The per-plan *kernels* are ported, not redesigned: Phase 3 re-implements the
existing traversal semantics (same candidate order, same queue usage, same
Morgan logic) on top of the new data model. Kernel fusion and the
interpreter collapse are Plan 8; the shared ray cache is Plan 7; queue
replacement is Plan 9. This separation keeps each gate diff attributable.
The goal of Plan 6 is byte-stable output with a strictly simpler dispatch
structure and pre-resolved constants.

## Phases

### Phase 0 — Inventory and porting map

Enumerate every public entry point `RenderEngine` and the shaders use from
the old layer (`BakedTracingCommon::traceObjectAllCrossings`,
`traceObjectFirstHit`, `traceShadowObject`, containment queries, …) and
every behavior the old layer implements (bounding shapes, clipping shapes,
composite recursion, no-shadow flags, primary-ray gates such as the
`!isPrimaryRayEnabled()` Path G gate, viewpoint plane cache). Produce a
porting table in this document: old function → new home. Anything the old
layer does that the new design has no slot for gets a design decision here,
before code.

**Status: DONE.** Full inventory performed (external entry points, every
routing/execution-kind branch, every `Baked*` struct field with its
populating function, and load-bearing subtleties). Summary below; see
`git log` / this section for the record.

#### External contract surface (only 5 methods are actually called from outside `render/bakedScene/`)

| Old function | Caller(s) | New home |
|---|---|---|
| `BakedTracingCommon::resetFallbackCounters` | `RenderEngine.cpp:398` | `BakedTrace::resetFallbackCounters` (kept only while any fallback path remains; deleted in Phase 5 once fallbacks are gone) |
| `BakedTracingCommon::getFallbackCounters` | `PovRayApplication.cpp:104` | same as above |
| `BakedTracingCommon::traceObjectFirstHit` | `RenderEngine.cpp:459,477`; `DirectLightShader.cpp:84` | `BakedTrace::traceFirstHit(const BakedScene&, int objectIndex, ...)` |
| `BakedTracingCommon::traceObjectAllCrossings` | `DirectLightShader.cpp:102` | `BakedTrace::traceAllCrossings(...)` |
| `BakedTracingCommon::containmentTest` | no external caller (internal only) | `BakedTrace::containmentTest(...)`, kept for internal bounding/clipping recursion |

`BakedCsgTracing`/`BakedSimpleBodyTracing`/`BakedCompositeTracing` have
**zero** external call sites — the entire internal three-class split is
free to be redesigned as long as these 5 signatures keep working.

#### Behaviors that must survive the rewrite, verbatim

1. **8-way `BakedSimpleBodyExecutionKind` collapses to the plan's 5-way
   `TraceKind`.** Only one bit of it is actually read at trace time today
   (the `canUseGeometryFirstHit` gate for DirectPrimitive/TransformedPrimitive,
   `BakedSimpleBodyTracing.cpp:100-102`); everything else re-derives routing
   from raw booleans per ray. `TraceKind` must still expose whatever
   distinction feeds that first-hit-without-a-queue fast path.
2. **10-way `BakedCsgOperandExecutionKind`, `BakedCsgSpecialization`
   (`TopLevelPlaneUnion`/`DisjointBoundedUnion`/`SingleCorePlaneIntersection`)
   and `BakedCsgExecutionPlanKind`** all get folded into `BakedCsgProgram`'s
   per-operand-kind bucket arrays plus a single `planKind` — this is
   already what Plan 6's design section describes; the porting job is
   mechanical (bucket arrays already exist as
   `executionPlan*OperandIndices`, just need to move into the new struct
   as-is).
3. **The `!ray->isPrimaryRayEnabled()` Path G gate** (nested-single-core-
   plane compiled emitter bypass, `BakedCsgTracing.cpp:149-157`) and the
   **primary-ray-only plane/quadric constant caches** (`planeVpCached`,
   `Quadric::isConstantCached`) are ray-class-dependent, not object-
   dependent — `takeoff.tga` broke when this was collapsed unconditionally
   (Plan 3 history). The new design keeps this as an explicit
   `ray->isPrimaryRayEnabled()` check at the same call sites; it is not a
   per-ray "routing metadata read" of the kind Plan 6 eliminates (it's a
   ray-property test, orthogonal to the object-constant data this plan
   bakes) and is EXPLICITLY exempted from the "no per-ray branching" rule.
4. **Composite child traversal order (reverse), the transform-conditional
   AABB culling gate (`!hasObjectTransform`), and the `.length()` vs
   parametric-`t` recomputation formula** (`BakedCompositeTracing.cpp:132-
   134,137-139,179-180`) must be ported byte-for-byte; these are exactly
   the kind of "obviously simplifiable" code a naive rewrite would collapse
   and silently break.
5. **`CsgScratchContext`'s LIFO borrow/return discipline** (max 8 local
   slots, ray-pool overflow) is the accepted design (two prior Plan 3 dead
   ends tried alternatives and regressed wall-clock) — ported as-is.
6. **The reserve-then-fixup pointer pattern** for `bakedQuadric`/
   `bakedPlane` self-referential copies is a workaround for
   `java::ArrayList`'s reallocating `add()`/`set()`. The new design should
   build each `BakedTraceableObject`/`BakedCsgProgram` tree bottom-up into
   *final* storage sized up front, removing the need for the two-phase
   fixup pass entirely — this is a real simplification opportunity, not
   just a port.
7. **Open questions carried into Phase 1-3 design** (flagged by the
   inventory, not yet resolved): (a) whether `traceMorganCsg`'s DIFFERENCE
   fallback branch (`BakedCsgTracing.cpp:1805-1817`, unconditional-offer
   loop, looks semantically wrong for DIFFERENCE) is ever actually
   exercised by any scene in the corpus — port faithfully regardless, but
   verify with a build-time assertion/counter during Phase 3 rather than
   silently "fixing" it; (b) `BakedComposite::boundedChildObjects`/
   `unboundedChildObjects` are populated but never read anywhere — drop
   them in the new struct unless Phase 2 porting finds a reader this
   inventory missed; (c) `GenericFallback` enum values are declared but
   never assigned by either classifier — keep the enum value for safety
   but do not expect to hit it; (d) the mutable primary-ray caches
   (`planeVpCached`/`Quadric`'s constant cache) are not thread-safe under
   `-parallel` today (plain `mutable` fields, no synchronization) — Phase 1
   must decide explicitly whether to move these into per-render-task
   scratch or keep the current (apparently-tolerated) shared-mutable-state
   behavior; default to keeping current behavior unless a race is observed,
   since changing it is out of this plan's scope (structural rebuild, zero
   numeric/behavioral change) — revisit under Plan 7 (raySharedCache).

#### Baked struct → new home

`Scene::CompiledTracingObject`, `BakedSimpleBody`, `BakedCsgOperand`,
`BakedConstructiveSolidGeometry`, `BakedComposite`, `CompiledTracingScene`
(`Scene.h:22-199`) all move out of `environment/scene` into
`render/bakedScene/BakedScene.h` as Phase 4 of this plan, superseded in
shape by `BakedTraceableObject`/`BakedCsgProgram` (Phase 1-3 build the new
shape; Phase 4 only relocates + deletes the old one once nothing depends on
it). Every populating function (`bakeSimpleBody`, `bakeCsgOperand`,
`bakeConstructiveSolidGeometry`, `bakeComposite`, `classifyBaked*`,
`buildBakedCsgExecutionPlan`, `compileTracingObject`,
`buildCompiledTracingScene`'s two fixup passes — all in `Scene.cpp` today)
moves into `BakedSceneBuilder.cpp`.

### Phase 1 — Build the new model alongside the old

`BakedSceneBuilder` compiles the parsed scene into `BakedScene` at the same
point `Scene::finalizeCompiledTracingScene` runs today. The old structures
keep building in parallel. Add builder statistics (object counts per
`TraceKind`, CSG plan inventory, residual-transformed-operand count with
transform-class count) printed under the existing statistics facility.

Gate: unchanged output (the new model is built but unused).

**Status: DONE.** `render/bakedScene/BakedScene.h` (the new flat model:
`TraceableObject`/`CsgProgram`/`CsgOperandRecord`/`CompositeRecord`, unified
index space for simple bodies and composites so bounding/clipping/child
references are plain `int` indices into one array instead of the old
mutually-exclusive-pointer-pair) and `BakedSceneBuilder` (translates
`Scene::CompiledTracingScene` - still the authoritative source at this
phase - into it) are built. `Scene` owns a `BakedScene bakedScene` member,
rebuilt every `buildCompiledTracingScene()` call (same point as the old
model), exposed via `getBakedScene()`; nothing reads it yet. Builder
statistics ("Plan 6 baked model...", "Plan 6 CSG programs...", "Plan 6
residual (un-collapsed) operands...") print alongside the existing Plan
1-5 statistics in `PovRayApplication::printStatistics`.

Cross-checked against the old model's own statistics on `drums.pov`: old
"direct 18 + transformed 1" simple bodies == new "direct 19" (Plan 6
correctly collapses `DirectPrimitive`/`TransformedPrimitive` into one
`TraceKind`, since Plan 5 already made that distinction numerically
irrelevant for the hot path); csg 86 matches exactly both models; CSG
program count 450 and the full plan-kind breakdown
(generic-morgan/core-plane/etc.) match exactly; collapsed-quadric count 150
matches; new "residual transformed operands: 244" matches the old
"transformed-nested 244" (operands whose transform survives Plan 5's fold
are exactly the `TransformedNestedCsg` ones, expected since Plan 5 only
folds leaf quadric/plane operands, never nested-CSG operands).

Full gate: `./scripts/renderAll.sh` (138s) +
`./scripts/testAgainstGoldenImages.sh` → `Test passed.`, byte-identical
across the whole suite (expected: the new model is built but has zero
readers).

Note for Phase 4: `BakedSceneBuilder::build` currently takes
`Scene::CompiledTracingScene` as its only input (i.e. it derives the new
model from the old one, not from the parsed `SimpleBody`/`CsgOperand`/
`Composite` tree directly). This was a deliberate Phase-1 simplification to
avoid re-deriving the classify/bake logic twice while both models still
need to exist and agree. Phase 4 (moving the `Baked*` structs out of
`Scene.h` for good) must change `BakedSceneBuilder` to consume the parsed
tree directly instead, porting `bakeSimpleBody`/`bakeCsgOperand`/
`bakeConstructiveSolidGeometry`/`bakeComposite`/`classifyBaked*`/
`buildBakedCsgExecutionPlan` (currently in `Scene.cpp`) into
`BakedSceneBuilder.cpp` verbatim - at that point the reserve-then-fixup
pointer dance these functions do can also be dropped, since the new
`BakedSceneBuilder` builds bottom-up into the new arrays' *final* value
slots directly (each `java::ArrayList::add` still copies, but the
self-pointers are only taken in a single, final pass after each array has
stopped growing for good - same idea as today's fix-up pass, just against
the new arrays instead of the old ones).

### Phase 2 — Port the non-CSG paths

Implement `BakedTrace` for DirectPrimitive, Composite, bounded/clipped and
generic-fallback objects; switch `RenderEngine` to the new entry points for
those kinds only (CSG still routes to the old layer through a temporary
bridge). Bounding-shape tests are a plain inlined loop over
`boundingObjects` — no lambda trampoline (the rank-10 hotspot from Plan 4 is
a porting bug if it reappears).

Gate: byte-identical to the Plan 5 reference state. This phase must be
byte-exact because no numeric path changed.

**Status: DONE.** `render/bakedScene/BakedTrace.{h,cpp}` implements the new
entry points (`traceFirstHit`/`traceAllCrossings`/`containmentTest`),
dispatching on `BakedScene::TraceableObject::kind`: `DirectPrimitive`/
`BoundedGeneric`/`GenericFallback` route through the newly-ported simple-
body logic (`traceSimpleBodyAllCrossings`/`traceSimpleBodyFirstHit`/
`simpleBodyContainmentTest`/`finalizeSimpleBodyCandidate`/
`passesBoundingShapes` - faithful ports of `BakedSimpleBodyTracing`,
including its ultra-fast direct-quadric bypass), `Composite` routes through
the newly-ported `traceComposite*` functions (faithful port of
`BakedCompositeTracing`, including the transform-conditional AABB-culling
gate and the `.length()` `t`-formula quirk flagged in Phase 0), and `Csg`
bridges transparently to the old `BakedSimpleBodyTracing`/`BakedCsgTracing`
via a `legacyScene` parameter (`Scene::getCompiledTracingSceneForBridge()`)
- object indices are guaranteed to match 1:1 between the two models for
simple-body-derived entries (see `BakedSceneBuilder`), so the bridge is a
direct array lookup, no translation needed. `RenderEngine::trace` and
`DirectLightShader::shade`'s shadow-ray path (`LocalSurfaceShader`→
`DirectLightShader`, threaded through `RayShaderPipeline`) now call
`BakedTrace` unconditionally for every top-level/shadow-casting object -
simpler than the plan's literal phrasing ("switch RenderEngine... for
those kinds only"), since the Csg bridge lives inside `BakedTrace` itself
rather than as a caller-side branch. Note also that
`canUseCsgFirstHitForShadow` (the shadow first-hit fast path gate in
`DirectLightShader.cpp`) had to key off `csgProgramIndex >= 0` directly,
not `kind == Csg` - the old gate applied to CSG-having bodies regardless of
bounded/clipped status, which Plan 6's `BoundedGeneric` kind now also
covers.

**Bug found and fixed during this phase**: `BakedSceneBuilder`'s
`translateOperand`/`translateSimpleBody` re-pointed a folded operand's
`geometry`/`quadricGeometry` at its own (about-to-be-copied) local
`bakedQuadricStorage`/`bakedPlaneStorage` - exactly the self-referential-
pointer hazard Phase 0 flagged (item 1), except this time on the *new*
model instead of the old one. The pointer went stale the moment the local
`CsgOperandRecord`/`TraceableObject` was copied into its final
`java::ArrayList` slot (copy-assignment, not move, per `ArrayList.txx`),
causing segfaults (`cliptst2.pov` crashed inside
`simpleBodyContainmentTest`) across ~29 scenes on the first `renderAll.sh`
run. Fix: don't re-point at all - `out.geometry`/`quadricGeometry` were
already being copied from `operand.geometry`/`quadricGeometry`, which for
a folded operand already alias the OLD model's own `bakedQuadric`/
`bakedPlane` (fixed up once by `Scene::buildCompiledTracingScene`, stable
for the OLD model's lifetime - i.e. the whole `Scene`'s lifetime between
rebuilds). Since `legacyScene` is kept alive alongside `bakedScene` for the
Phase 2/3 bridge anyway, aliasing into it is safe and avoids the
self-pointer problem entirely; the `*Storage` fields are kept for the
`hasBakedQuadric`/`hasBakedPlane` statistics only, not as pointer targets.

**Gate**: `./scripts/renderAll.sh` (134s) + `./scripts/testAgainstGoldenImages.sh`
→ `Test passed.`, byte-identical. `./scripts/renderParallel.sh` (drums.pov +
iortest.pov under `-parallel`) also byte-identical against golden -
confirms the per-task scratch design (`CsgScratchContext`, `RayWithSegments`
pools) still holds with the new dispatch layer added on top.
`./scripts/benchmarkPanel.sh`: spline 0.186x, ntreal 0.519x, piece3 0.559x,
iortest 0.651x, shapes2 0.393x of the (also-measured-in-this-session)
baseline - no regressions.

**Investigated and ruled out as a Phase 6 concern**: an initial drums.pov
absolute-timing check showed ~130s, wildly worse than Plan 5's recorded
~8.4s. Bisected by building and timing three points in a scratch worktree:
Plan 6 Phase 1 (`a9a5751`), the exact Plan 5 end-state commit (`65115a5`,
predates all Plan 6 work), and the current Phase 2 tree - all three take
~118-130s for the identical drums.pov render (same ray count and
intersection-test counts reported in the stats block across all three).
This proves the absolute slowdown is an **environmental property of this
session's machine** (CPU throttling/scheduling/noisy-neighbor - root cause
not further investigated, out of scope for this plan), not something
Phase 2 introduced. `benchmarkPanel.sh`'s baseline-vs-current ratios remain
valid evidence of no regression because both sides of each ratio are
measured in the same session under the same environmental conditions.

### Phase 3 — Port the CSG paths

Re-implement the CSG traversal semantics over `BakedCsgProgram`: generic
Morgan union/intersection/difference, the compiled specializations
(`SingleCorePlaneIntersection`, `TopLevelPlaneUnion`,
`DisjointBoundedUnion`), ray-segments algorithm scenes (`-csgRoth` path if
still routed here), first-hit and shadow variants, containment queries.
Port faithfully — same candidate ordering, same scratch-queue discipline —
so the gate stays byte-identical. The known behavior gates (primary-ray
gate on nested emitters; the mirror-corner top-level bare-planes-union
bypass) are ported as build-time plan choices, not per-ray branches.

Gate: byte-identical to the Plan 5 reference state.

### Phase 4 — Move the structs out of `Scene.h`

Delete `CompiledTracingScene` and all `Baked*` structs from
`environment/scene/Scene.h`; `Scene` keeps only parsed data plus the
`TransformStep` lists from Plan 5. `RenderEngine` (or `PovrayApplication`)
owns the `BakedScene` instance and passes it to render workers.
`environment/scene` no longer includes anything from `render/`.

Gate: byte-identical; also verify include-graph direction
(`environment` must not include `render` headers) with a grep check:

```bash
grep -rn '#include "render/' src/environment/ && echo "LAYERING VIOLATION" || echo "layering ok"
```

### Phase 5 — Cutover and deletion

Remove the old five files and the temporary bridge; remove dead statistics;
run the full gate, drums timing, panel, and a gprof capture as the Plan 6
closing measurement.

## Measurement Gate

Every phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

Byte-identical output vs the Plan 5 accepted state is required for **every**
phase of this plan — this is a structural rebuild with zero numeric changes.
Any image diff at all is a porting bug.

Additionally at Phases 2, 3 and 5: drums timing (3 runs), benchmark panel,
and `-parallel` smoke run (`./scripts/renderParallel.sh` or the documented
parallel invocation) to confirm the per-task scratch design holds under
threads.

## Acceptance Criteria

- Old layer deleted; new layer owns the baked model; `Scene.h` reduced to
  parsed data.
- Zero per-ray reads of execution-kind style metadata below the single
  top-level `TraceKind` switch: verify by code review of the hot path and by
  gprof (no function shaped like `classify*` / `resolve*` / plan-switch
  helpers in the profile).
- Effective materials resolved at build time (code review + one debug assert
  path during development).
- drums wall-clock: no regression vs the Plan 5 exit time; any improvement
  is a bonus (the structural wins are mostly harvested in Plans 7–8, but
  removing per-ray material resolution and argument-heavy call chains
  usually pays something immediately).
- gprof hot-function count over drums: the CSG-D + Body/Comp/BTC routing
  categories (47.2% combined in Plan 4) must show visibly fewer distinct
  functions; record the new table in this document.

## Risks and Dead-End Reminders

- Porting drift: the old layer encodes subtle accepted behaviors
  (primary-ray gates, `takeoff.tga`-sensitive emitter ordering — see Plan 3's
  rejected clone-reduction cut). Byte-exact gating per phase is the defense;
  port semantics first, simplify later (Plan 8).
- Do not "improve" candidate ordering or queue behavior in this plan
  (append-only queues and reordered drains are recorded byte-exactness
  breakers).
- Keep scratch per task; the Plan 3 dead ends "reusable local-ray scratch
  stacks" and "sharing scratch ownership from BakedSimpleBodyTracing" both
  regressed wall-clock — the new design must not recreate them under a new
  name without measuring.
- RAM: the baked model remains a side-car copy of the scene (accepted).
  Avoid gratuitous duplication beyond what the hot path needs; cold debug
  pointers are fine.
