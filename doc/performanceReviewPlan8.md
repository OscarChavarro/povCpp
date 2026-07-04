# Performance Review Plan 8: Fused CSG Execution Kernels — Collapsing the Interpreter

## Position in the Plan Sequence

Fourth of six (Plans 5–10; sequence table in `doc/performanceReviewPlan5.md`).
Requires Plans 5–7: world-space baked operands, the rebuilt
`render/bakedScene` model with per-plan `BakedCsgProgram` tables, and the
per-ray shared cache. This plan is Plan 4 §6.3 ("collapse the interpreter
layers") executed on the new foundation.

## Motivation (from Plan 4, Conclusions §3)

Baseline call path per (ray × CSG object): scene loop → 1 virtual call →
`allCsgIntersectIntersections` (a 50-line two-nested-loop kernel) → 1 virtual
call per operand → primitive math. Three functions held 62% of all time.

The pre-rebuild path was: `RenderEngine::trace` →
`traceObjectAllCrossings` → `traceAllCrossings` → `traceAllCrossingsWithScratch`
→ plan dispatch → `traceOperandAllCrossings` (per-operand kind switch + AABB
+ material resolution) → `intersectBaked*` — the same work smeared over 93
functions, none above 7.4%, with 5–7 arguments shuffled per level. Plan 6
removed the per-ray metadata reads and material resolution but deliberately
ported the traversal shape. This plan removes the traversal shape itself:
**one fused loop per plan kind, chosen once at build time.**

After Plans 5–7 the operands a kernel iterates are world-space geometries in
flat arrays with a shared per-ray cache — exactly the conditions under which
the baseline kernel was fast. The fused kernels are therefore a
reconstruction of the baseline's Morgan kernel shape over the new data
model, per plan kind, with zero numeric change.

## Architectural Constraints

- All work happens inside `render/bakedScene` (kernels) and consumes
  `render/raySharedCache`. `environment/geometry` and `environment/scene`
  are untouched.
- Kernels receive `(const BakedCsgProgram&, const RayWithSegments&,
  RaySharedCache&, CsgScratchContext&, results)` — nothing else. If a kernel
  needs more, the builder failed to bake something; fix the builder, not the
  signature.
- Candidate ordering and queue semantics stay exactly as ported in Plan 6
  (byte-exactness). Queue *replacement* is Plan 9, not here.

## Design

One kernel function per `BakedCsgPlanKind` × {allCrossings, firstHit,
shadow}. The build-time plan choice becomes a stored function pointer (or a
single top-level switch in `BakedTrace` — measure both; the switch is
usually faster than an indirect call when the kind population is small, and
it inlines):

```
traceCsgAllCrossings(program, ray, cache, scratch, out)
    → kernelUnionAllCrossings(...)            // fused: loop operands inline,
    → kernelIntersectionAllCrossings(...)     // intersect primitive inline,
    → kernelSingleCorePlaneAllCrossings(...)  // membership merge inline
    → kernelTopLevelPlaneUnionAllCrossings(...)
    → kernelDisjointBoundedUnionAllCrossings(...)
    → kernelRaySegments(...)                  // -csgRoth algorithm scenes
    → kernelFallback(...)                     // anything not yet fused
```

Inside a fused kernel:

- the operand loop iterates the flat SoA arrays directly
  (`operandGeometry[]`, plane arrays, bounds) — no per-operand function call
  for classification, no AABB helper call (inline the slab test), no
  per-operand kind switch: the builder already partitioned operand index
  ranges by kind (planes first, direct quadrics next, residual transformed
  last — the partition order is a builder guarantee documented in
  `BakedCsgProgram.h`);
- primitive intersections call the geometry routine directly
  (`Quadric::doIntersectionForAllRayCrossings`-equivalent via the cache-aware
  baked helper for quadrics; direct calls for plane/sphere) — one call level
  above the math, as in the baseline;
- containment/membership merging (`mergeByMembership`,
  `containmentTestOperand` logic) is inlined into the kernel body where the
  profile justifies it, keeping the exact test order;
- nested CSG children recurse through the same top-level kernel dispatch
  (depth is bounded by scene nesting; the baseline recursed too).

`static` + internal linkage (or members of a kernel class) so the compiler
can inline aggressively within the translation unit; keep each kernel in one
`.cpp` to give the optimizer the whole loop.

## Phases

Each phase fuses one plan kind, gates, and measures before the next starts.
Order by measured heat (Plan 4 table):

### Phase 1 — Union all-crossings kernel

`traceGenericMorganUnion` (3.21%) + its share of `traceOperandAllCrossings`
(7.42%) + `traceMorganCsg` routing (1.35%). Fuse the generic Morgan union
into one loop. Scenes to watch beyond the gate: `drums`, `skyvase`
(mirror-corner bypass must keep working), `chess` (nested case), `tomb`,
`snack`.

### Phase 2 — Intersection kernels

`traceMorganIntersectionGeneric` (2.90%) and
`kernelSingleCorePlaneAllCrossings` replacing the compiled core-plus-plane
paths (`traceCompiledCoreOperandAllCrossings` 1.31%,
`traceTransformedNestedSingleCorePlaneOperandAllCrossings` 1.83%,
`tracePlaneOperandCandidate` 2.24%). Watch `iortest`, `wg5`, `desk`,
`fishbowl`.

### Phase 3 — First-hit and shadow kernels

`traceFirstHit*` family and `traceShadowObject` path. The primary-ray gate
(Path G) is a build-time plan property; first-hit kernels for non-primary
rays must preserve the exact early-exit order (`trueMiss`,
`traceFirstHitByIntersectionMembership`). Watch `takeoff` (historic
byte-exactness canary for first-hit ordering), `pacman`.

### Phase 4 — Remaining plan kinds + fallback audit

`TopLevelPlaneUnion`, `DisjointBoundedUnion`, ray-segments. Then audit the
builder statistics across all 108 scenes: every CSG object must map to a
fused kernel or be explicitly counted as fallback with a scene list recorded
here (target: fallback population ≈ 0 across the suite; document any
irreducible stragglers).

### Phase 5 — Simple-body and composite loop tightening

Apply the same fusion discipline to the non-CSG hot traversals identified in
Plan 4: `BakedSimpleBodyTracing::traceAllCrossings` (5.18%) / `traceFirstHit`
(2.69%) successors and `BakedCompositeTracing::traceAllCrossingsInCompositeSpace`
(2.31%) successor: inline `passesBoundingShapes` (2.35%) and
`finalizeCandidate` (1.86%) where the compiler doesn't already, and confirm
the Plan 6 rule that no lambda trampoline sits on the first-hit path.

### Phase 6 — Closing measurement

drums (3 runs), panel, gprof capture; record the new hot-function table and
compare its concentration against both the Plan 4 table (93 functions) and
the baseline (3 functions at 62%).

## Measurement Gate

Every phase:

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

Byte-identical output vs the Plan 7 exit state at every phase — fusion
re-arranges control flow, never arithmetic or ordering. Plus, per phase:
drums timing (3 runs) and a gprof run confirming the targeted functions
disappeared from the profile (their time must reappear inside the kernel,
not vanish into new helpers).

`-parallel` determinism check (two renders byte-compared) at Phases 3 and 6.

## Acceptance Criteria

- drums wall-clock: this is the plan expected to close most of the remaining
  gap. Target: ≤ 1.2× the baseline 5.10 s (≤ ~6.1 s) at Phase 6, measured on
  the same machine as the Plan 4 numbers.
- gprof concentration: ≥ 50% of drums self-time in ≤ 10 functions, with the
  top entries being kernels and primitive math, not routing.
- `traceOperandAllCrossings` and its successors: 0 calls (function deleted).
- `LocalIntersectionClone` on drums: only residual transformed operands may
  still clone; count reported and justified against the Plan 6 builder
  statistics.
- Fallback-kernel population across the suite documented (≈ 0).

## Status: Phase 1 attempted, paused pending strategy decision

Before starting, re-profiled drums with gprof on the current (post-Plan-7)
tree rather than trusting the Plan 4 heat table, which predates Plans 5-7
and no longer reflects reality. Current top drums self-time (320x200,
non-parallel):

| self% | calls | function |
| ---: | ---: | --- |
| 12.46% | 50.1M | `intersectBakedQuadricWithTrueMiss` |
| 9.76% | 53.4M | `traceOperandAllCrossings` |
| 8.59% | 56.8M | `traceMorganCsg` |
| 6.06% | 25.9M | `traceSimpleBodyAllCrossings` lambda |
| 6.06% | 23.5M | `traceTransformedNestedSingleCorePlaneOperandAllCrossings` |
| 5.56% | 3.0M | `traceMorganIntersectionGeneric` |
| 5.22% | 28.6M | `traceCompiledCoreOperandAllCrossings` |

Two findings changed the plan's Phase 1 target:
1. `traceGenericMorganUnion` does not appear in the profile at all - the
   compiler already fully inlines it into `traceMorganCsg` (both are
   `inline` header functions), so "fuse the union kernel" in the literal
   sense the plan describes is already done by the optimizer. There was no
   separate union-dispatch cost left to remove.
2. The `SingleCorePlaneIntersection`/core-plane family
   (`traceCompiledCoreOperandAllCrossings` +
   `traceTransformedNestedSingleCorePlaneOperandAllCrossings` + their
   first-hit counterparts) is now the dominant cluster in drums (250 of 450
   CSG programs use this specialization vs. 200 generic-Morgan), matching
   what Plan 7 already found. This is Phase 2's target, not Phase 1's.

Implemented anyway, scoped to what Phase 1 could still plausibly reach:
in `traceGenericMorganUnion`'s transformed-operand loop,
`TransformedQuadric` operands (the dominant residual-transformed kind) now
get a direct inlined fast path instead of calling
`traceOperandAllCrossings`'s generic kind-switch dispatch. Iteration order
was kept identical (same array, same direction) specifically to avoid
disturbing equal-depth candidate tie-breaking across operand kinds - a
historically real regression class in this codebase.

Gate: byte-identical (full suite), `-parallel4` determinism re-checked on
drums (AE=0), panel scenes unaffected.

**drums timing: no measurable change** (~8.7s before -> ~8.9s after,
noise-level). This is the **fourth consecutive fusion/caching attempt**
(Plan 7 Phases 2-4, and now this one) that is individually correct,
gate-green, and reasoned from a real profile finding, yet produces no
measurable drums improvement. The consistent pattern across all four
points to the same conclusion each time re-confirmed: drums's cost is
concentrated in `intersectBakedQuadricWithTrueMiss`'s raw arithmetic
(12-13% self-time, ~40 FLOPs x 50-68M calls) and in the sheer call volume
through the core-plane specialization, neither of which a dispatch-level
fusion touches - fusion removes call/switch overhead *around* the math, and
that overhead is evidently not where drums's remaining ~1.7x-vs-baseline
gap lives.

**Recommendation given to the user**: pause Plan 8's remaining phases
(2-6) rather than continue speculatively. Phase 2 (the core-plane kernel,
the plan's own next-highest-heat target and the one most likely by current
profile data to matter) is the reasonable next attempt *if* the user wants
to keep pushing on drums specifically, but the last four attempts targeting
correctly-identified hot functions all failed to move the number, so there
is no strong basis for confidence it will differ. The two directions with
actual remaining leverage per Plan 4's own Conclusions are (a) revisit
scene-owned coefficient baking with baseline-identical operation order
(Plan 4 Conclusions #1 / Plan 5's original, only partially realized idea)
or (b) accept the current ~1.7x-of-baseline drums figure as the practical
floor of this architecture and close the Plan 5-10 sequence's performance
chase, banking the real wins already delivered (correctness fixes, the
-parallel race fix, zero regressions across 108 scenes throughout).

## Risks and Dead-End Reminders

- **Byte-exactness under reordering.** The recorded dead ends are explicit:
  append-only queues broke byte-exact; the Plan 3 clone-reduction emitter
  broke `takeoff.tga` via first-hit ordering. Fuse control flow, keep
  candidate order; when a kernel wants a better order, that is a Plan 9+
  experiment with its own gate, never a silent part of fusion.
- **Icache blowup.** 15+ fused kernels × aggressive inlining can exceed the
  icache the fusion was meant to relieve. Watch the panel (not just drums)
  at every phase; if a phase wins drums but loses the panel, split or
  de-inline that kernel.
- **Compiler-dependent wins.** Function-pointer vs switch dispatch and
  inline decisions must be measured on the Release toolchain used for the
  official numbers, not assumed.
- The mirror-corner bypass (`skyvase`) and nested-composite virtual dispatch
  (`pacman` eyes) are historic regression sites; both have golden coverage,
  but check them explicitly in the phase notes.
