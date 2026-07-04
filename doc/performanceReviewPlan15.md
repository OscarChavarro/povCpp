# Performance Review Plan 15: Coefficient Collapse Through Composite Transforms

## Position in the Plan Sequence

Fourth executable plan of the Plan 11 cycle, deliberately last of the
optimization plans: it is the only one with real numeric risk (it extends
Plan 5's replay machinery to a new owner class and therefore re-enters the
Golden-Image Evaluation Protocol world). Requires Plans 12–14 measured
first so its own effect is attributable.

## Evidence Being Attacked (from Plan 11 §2, drums)

- 14.9 M `traceCompositeAllCrossings` calls constructing **12.5 M**
  composite-space `LocalIntersectionClone` rays per render — about half of
  drums' 27.3 M total clone volume. `traceCompositeAllCrossingsInComposite
  Space` alone is 7.9% of drums self-time.
- 13.0 M of the remaining clones come from `traceSimpleBodyAllCrossings`
  on transformed bodies whose geometry kinds Plan 5 could not bake
  (spheres, boxes) or whose owners are composite children Plan 5 never
  reached.
- This is precisely Plan 10's residual suspect (d) — "structural clone
  volume independent of CSG population" — scoped down to its *bakeable
  subset*: composite children whose geometry is a quadric or plane with a
  recorded elementary-step list. The unbakeable remainder (spheres — see
  Plan 5 Phase 0: the baseline itself never coefficient-baked spheres —
  boxes, patches) stays declared structural floor.

## Architectural Constraints

- Identical to Plan 5's, verbatim: `environment/geometry` gains no
  transform knowledge, no mutable state, no baking hooks;
  `environment/scene` gains only passive parse-time data (one new
  `TransformStep` list per `Composite` level); all replay and all baked
  copies live in `render/bakedScene`.
- The Plan 5 numeric law stands: **replay elementary steps one at a time,
  in recorded order, with the exact baseline formulas** — never the
  composed matrix (recorded dead end).
- Every collapsed owner gets an explicit `bakedTransformFolded`-style flag
  set at bake time. Never infer "was baked" from pointer comparisons in
  `doExtraInformation` (twice-recorded dead end, Plan 5 Phases 3–4).

## Phase 0 — Composite Transform Semantics (evidence, no code)

Plan 5 Phase 1 deliberately did **not** record `TransformStep`s on
`Composite` (its ownership appendix: composites only ever apply
object-level transforms and were not a bake target). That decision is the
first thing to re-verify against today's code:

1. Trace how a `Composite`'s transform pair is built at parse time
   (`parseComposite` path) and how `traceCompositeAllCrossings` /
   `…InCompositeSpace` apply it per ray, including **nested** composites
   (the level-by-level model confirmed in Plan 5's ownership appendix and
   the [[nested_composite_virtual_dispatch_fix]] history — outer composite
   transforms must reach nested children).
2. Write down, in this document, the exact per-level replay order needed
   for a child quadric/plane: child geometry-layer steps, then child
   object-layer steps, then each enclosing composite's steps innermost-to-
   outermost — *derived from the matrix code, not assumed* (Plan 5's rule).
3. Enumerate what else composite-space evaluation does besides transform
   (bounding tests in composite space? texture/detail owners entangled with
   the composite level?) — each is a correctness hook the collapse must
   preserve or explicitly flag.

## Phase 1 — Record Composite Steps at Parse Time

Mirror Plan 5 Phase 1 at the `Composite` chokepoints: append a
`TransformStep` to the composite's own list inside the same functions that
build its matrices (recording cannot drift from the matrices if it lives
in the same function — the Plan 5 pattern). Gate byte-identical (recording
is passive).

## Phase 2 — Bake Composite Children Where the Rules Exist

For each composite child that is a `SimpleBody` whose geometry is a
Quadric/InfinitePlane with complete step lists (its own + all enclosing
composites'), produce a world-space baked copy via `BakedGeometryBaker`
replaying the Phase 0 order, store it in the composite's baked record, set
the explicit folded flag on the live objects, and route the trace path to
the direct (non-clone) kind. Children that fail any precondition
(unbakeable kind, bounding/clipping shapes tested in local space, missing
steps) stay on the clone path untouched — partial collapse per composite
is expected and fine.

Bake-time statistics line: composite children collapsed vs residual, per
scene. drums expectation from Plan 11: a large share of the 12.5 M
composite-space clones disappears.

## Phase 3 — Shading Unwind Audit

The two Plan 5 bugs live here in new clothing; audit before enabling:

- `doExtraInformation` unwind through composite levels must skip the
  folded levels' inverse-transform application (the explicit flag, not a
  heuristic).
- The detail-owner ordering fixed in [[texture_frame_fixes_part7]] (§17,
  `pushDetailOwner` not prepend) must survive the collapsed path — chess
  and dfwood are the canaries.

## Phase 4 — Golden Protocol and Close

Baked coefficient math takes a different FP path than per-ray transforms;
scenes with composite-heavy content may shift at noise level exactly as
Plan 5's did. Re-run Plan 5's Golden-Image Evaluation Protocol verbatim:
archive a fresh baseline render set, classify every flagged scene as
recovery / FP-noise / regression, re-baseline only with explicit user
confirmation, and record the table here. Then the standard close: gate,
`renderParallel.sh`, valgrind, drums/iortest/panel/renderAll timing table,
re-profile.

## Acceptance Criteria

- Phase 0 semantics appendix written before any code.
- Composite-clone count in drums measurably down (gprof
  `LocalIntersectionClone` callers) with gate clean under the protocol.
- No unexplained diffs; every re-baseline user-confirmed.
- Residual (unbakeable) clone population documented as the cycle's final
  declared floor, handed to Plan 16.

## Known Dead Ends (inherited from Plan 5; all still binding)

- Composed-matrix coefficient baking (replay elementary steps only).
- Sphere → baked-quadric promotion (`intersectBakedQuadric` ≠
  `intersectSphere` numerically; Plan 3 Phase H, 35 failures).
- Inferring "baked" from `hit->hitGeometry != geometry` in
  `doExtraInformation` (broke the gate twice; explicit flags only).
- Double-applying parse-time-destructive `Invert` steps in replay (Plan 5
  Phase 3's root-cause bug — `invertGeometry()` already mutated the parsed
  coefficients at parse time).
