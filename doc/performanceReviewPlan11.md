# Performance Review Plan 11: Re-Diagnosis at Commit 472fa48 — the 85 s vs 120 s Gate Gap

This document is the successor of `doc/performanceReviewPlan5.md`'s diagnosis
sections, re-executed from scratch against the current tree. It is a
*diagnostic* plan (like Plan 4 was): it changes no code. Its deliverable is
the measurement record below and the derived Plans 12–16.

## Position in the Plan Sequence

The Plans 5–10 cycle is closed (`doc/performanceReviewPlan10.md`, Ending B:
structural floor declared). This plan re-opens the original question with
fresh measurements at the current commit, because the tree has moved past
the Plan 10 exit state (commits "Performance review, part 21–25") and the
Plan 10 numbers no longer describe it.

| Plan | Subject | Depends on |
| --- | --- | --- |
| **11 (this one)** | Re-diagnosis at `472fa48`; evidence base for the new cycle | — |
| 12 | Call-overhead removal: LTO + inlining of hot leaf functions + per-ray AABB reciprocal cache | 11 |
| 13 | Hierarchical operand/object culling (build-time BVH in `render/bakedScene`) | 11 |
| 14 | Candidate-traffic reduction: indirect heap + operand record hot/cold split | 11, 13 |
| 15 | Coefficient collapse through `Composite` transforms (extends Plan 5's replay) | 11, 12–14 measured |
| 16 | Consolidation: re-measure, compare, close or seed the next cycle | 12–15 |

## Reference Commits and Benchmarks

- Older baked baseline: `4af1a75e5b7356600bec34e12a4882560994a058`
  (worktree `/tmp/povCpp-baseline-4af1a75`; its `build/CMakeCache.txt` was
  re-verified clean of the `-pg` contamination Plan 10 found and fixed —
  `CMAKE_CXX_FLAGS` empty, optimization from `target_compile_options -O3`).
- Current branch: `472fa486e3a5380745e01d45fcda8a67116a962c`
  ("Performance review, part 25"). **This measurement replaces the
  "Current scene-owned baked branch" reference point of Plan 5.**

All numbers below were measured fresh on 2026-07-04, same machine, same
session; drums runs interleaved current/baseline to cancel load drift.

### Primary benchmark — the full gate, which is what the user experiences

```bash
/usr/bin/time -f "%e" ./scripts/renderAll.sh     # 108 scenes, 1280x800, backgrounded
```

| Tree | Wall time | Ratio |
| --- | ---: | ---: |
| Baseline `4af1a75` | 85.4 s | 1.00× |
| Current `472fa48` | 120.1 s | **1.41×** |

Plan 10 measured 146 s vs 86 s (1.70×) at its exit commit; parts 21–25
recovered ~26 s of gate time between Plan 10's close and this measurement.

### Where the 120 s actually is: the longest-pole analysis

`renderAll.sh` backgrounds all 108 scenes at once; its wall time is the
*maximum* scene time, not the sum. Per-scene finish offsets (mtime of each
`.tga` minus the first one):

| Finish offset | Current `472fa48` | Baseline `4af1a75` |
| ---: | --- | --- |
| last (= wall) | `drums` 118.9 s | `drums` 84.6 s |
| 2nd | `iortest` 58.6 s | `piece3` 40.1 s |
| 3rd | `ionic5` 46.3 s | `iortest` 37.4 s |

**The whole 85 s → 120 s gate gap is literally drums.pov's own finish time**
(118.9 vs 84.6 s at 1280×800 under 108-way process contention), with
`iortest` as the second pole that would cap any improvement at ~59 s. Every
other scene finishes within ~46 s in both trees. Plan 10's suite-wide 1.70×
claim was true of its exit commit, but at `472fa48` the median scene is back
at parity (see the panel below) and the gate gap is drums-shaped again —
with iortest next in line.

### Secondary benchmarks

drums 320×200, 5 interleaved rounds each:

| Tree | Runs (s) | Mean | Ratio |
| --- | --- | ---: | ---: |
| Baseline | 5.15 5.14 5.12 5.15 5.14 | 5.14 | 1.00× |
| Current | 7.71 7.77 8.21 7.78 7.72 | 7.84 | **1.52×** |

`./scripts/benchmarkPanel.sh` (baseline column already corrected by Plan 10):

| Scene | Baseline (s) | Current (s) | Factor |
| --- | ---: | ---: | ---: |
| `level2/spline` | 1.045 | 0.38 | 0.36× |
| `level3/ntreal` | 0.64 | 0.66 | 1.03× |
| `level3/piece3` | 2.015 | 2.15 | 1.07× |
| `level2/iortest` | 2.115 | 3.41 | **1.61×** |
| `level1/shapes2` | 0.145 | 0.14 | 0.97× |

Plan 10 measured 1.39×/1.47×/2.23×/1.21× for ntreal/piece3/iortest/shapes2;
three of the four are now at parity. iortest is the one panel scene still
materially slow, consistent with its role as the gate's second pole.

Gate reference state at `472fa48`: `./scripts/renderAll.sh` +
`./scripts/testAgainstGoldenImages.sh` → `Test passed.`, zero diffs.
(Measurement pitfall recorded while producing this document: leftover
`output/panel/*.tga` from `benchmarkPanel.sh` makes the golden gate report
missing references — delete `output/panel` before gating.)

## Motivation (unchanged from Plan 5, restated)

Understand why the baseline `4af1a75` executes `./scripts/renderAll.sh` in
~85 s while the current tree needs ~120 s, and turn that understanding into
actionable plans. Plans 6–10 improved the time (146 s → 120 s across the
cycle's aftermath) but the current `render/bakedScene` implementation still
does not support baking as efficient as the baseline commit's.

## Diagnosis §1 — It is not work volume; it is cost per unit of work

drums 320×200 intersection-test statistics, printed by both binaries:

| Test type | Baseline | Current | Current/Baseline |
| --- | ---: | ---: | ---: |
| Sphere | 947,382 | 1,294,663 | 1.37× |
| Plane | 91,153,576 | 12,432,486 | **0.136×** |
| Quadric | 68,775,936 | 67,699,854 | 0.98× |
| Total primitive tests | 160.9 M | 81.4 M | **0.51×** |

The current tree does **half** the primitive-test work of the baseline
(7.3× fewer plane tests at equal quadric tests — the baked CSG plans and
AABB culling actually work), yet takes 1.52× the time. Tests per second:
~31 M/s baseline vs ~10 M/s current. **The per-test orchestration cost is
roughly 3× the baseline's.** This sharpens Plan 10's Ending B: the floor is
not arithmetic volume (the arithmetic shrank); it is the machinery wrapped
around each test — dispatch depth, ray clones, candidate copies, queue
traffic, and linear operand scans.

Current drums bake inventory (from the statistics lines): 450 CSG programs
(200 generic-morgan, 250 core-plane), residual transformed operands 0
(collapsed: quadric 274, plane 368), 13 composites, 19 direct bodies.

## Diagnosis §2 — gprof, fresh sweep at `472fa48`

Recipe (unchanged), plus two traps worth recording:

```bash
cmake -S . -B build-gprof -DCMAKE_CXX_FLAGS="-pg" -DCMAKE_EXE_LINKER_FLAGS="-pg"
cmake --build build-gprof --target povray -j"$(nproc)"
# TRAP 1: CMakeLists.txt line 7 forces CMAKE_RUNTIME_OUTPUT_DIRECTORY to
# <repo>/build — the instrumented binary OVERWRITES build/povray (this is
# exactly how Plan 10's contaminated-baseline incident happened). Move it
# aside (mv build/povray build/povray-gprof), rerun ./scripts/compile.sh,
# and verify: nm -D build/povray | grep -c mcount   -> must print 0.
# TRAP 2: the 1992 parser truncates long +o output paths silently — render
# output lands nowhere. Use short output paths when byte-comparing.
cd etc/level3/drums2
../../../build/povray-gprof +l../../include +idrums.pov +o/tmp/d.tga +w320 +h200 -d +x +ft
gprof ../../../build/povray-gprof gmon.out > drums_gprof.txt
```

### drums flat profile, top self-time (total attributed ≈ 5.45 s)

| Self % | Calls | Function |
| ---: | ---: | --- |
| 11.2 | 42.5 M | `BakedCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings` |
| 8.4 | 50.3 M | `BakedCsgTrace::intersectBakedQuadricWithTrueMiss` |
| 7.9 | 32.4 M | `BakedCsgTrace::traceGenericMorganUnion` |
| 7.9 | 12.5 M | `BakedTrace::traceCompositeAllCrossingsInCompositeSpace` |
| 5.7 | 34.4 M | `BakedCsgTrace::traceOperandAllCrossings` |
| 5.1 | 25.9 M | `traceSimpleBodyAllCrossings` lambda#2 |
| 5.0 | 14.6 M | `Quadric::doIntersectionForAllRayCrossingsAnnotated` |
| 4.6 | 3.0 M | `BakedCsgTrace::traceMorganIntersectionGeneric` |
| 2.8 | 28.8 M | `BakedTrace::traceSimpleBodyAllCrossings` |
| 2.8 | 14.9 M | `BakedTrace::traceCompositeAllCrossings` |
| 2.6 | 27.6 M ×2 | `java::ArrayList<Material*>::ArrayList(long)` + `init()` |
| 1.7 | 27.3 M | `RayWithSegments::RayWithSegments(LocalIntersectionClone, …)` |
| 1.3 | 24.7 M | `java::PriorityQueue<IntersectionCandidate>::siftUp` |

Structure read off the call graph:

- **Call chain depth.** Every (ray × core-plane operand) test runs through
  four nested out-of-line frames: `traceMorganCsg` →
  `traceGenericMorganUnion` → `traceTransformedNestedSingleCorePlane…` →
  `intersectBakedQuadricWithTrueMiss`. The Morgan interpreter cycle
  (`traceMorganCsg` 37.8 M / `traceGenericMorganUnion` 32.4 M /
  `traceOperandAllCrossings` 34.4 M) accounts for **40.9% inclusive**. The
  baseline's equivalent was a flat linked-list walk calling primitive math
  directly (3 functions held 62% of its profile).
- **Clone volume** (Plan 10's suspect (d), now itemized): 27.3 M
  `LocalIntersectionClone` constructions — 13.0 M from
  `traceSimpleBodyAllCrossings` (transformed bodies), 12.5 M from
  `traceCompositeAllCrossings` (composite-space rays; drums has 13
  composites), remainder from shading unwind. Each clone also constructs
  two capacity-0 `java::ArrayList`s **out of line** (`ArrayList<Material*>`
  is explicitly instantiated only in `RayWithSegments.cpp`, so no other TU
  can inline its trivial ctor/init). Clone construction alone is ~4.2%
  inclusive.
- **Queue traffic**: 24.7 M `siftUp` calls moving a **168-byte**
  `IntersectionCandidate` (56 B `Intersection` + ~112 B
  `IntersectionAttributes`, 64 B of which is the `detailOwners[8]` array,
  almost always nearly empty).
- The scaffolding categories together (CSG trace + body/composite dispatch
  + clones + queue) hold ~60% of drums self-time; primitive math ~16%. The
  baseline profile was the inverse shape (~40% primitive math, ~13%
  scaffolding).

### Panel scenes: the linear operand scan

| Scene | `rayIntersectsAabbForward` | `traceOperandAllCrossings` | Note |
| --- | ---: | ---: | --- |
| spline | 29.5 M calls, **39% self** | 29.5 M calls, 36% self | 68,643 union traversals × 430 operands, scanned linearly per ray; **75% of the scene is the scan itself** |
| ntreal | 23.4 M calls, 33% | 9.1 M | same shape |
| piece3 | 37.7 M calls, 10.5% | 37.7 M, 9.2% | same shape |
| iortest | 33.3 M calls, 5.7% | 33.6 M, **14.9%** | plus `traceGenericMorganUnion` 24.4 M calls, 12.7% |

`traceGenericMorganUnion` iterates a union program's operand array linearly
for every ray; each operand pays an out-of-line AABB slab test
(`rayIntersectsAabbForward`, defined in `BakedCsgTrace.cpp`, so never
inlined — and it recomputes `1.0/direction` per axis per call, though the
ray's direction is fixed for its whole life). spline is *still* 0.36× —
faster than baseline — but three quarters of its remaining time is a scan a
hierarchy would collapse; iortest, the gate's second pole, has the same
shape on a scene 5× heavier.

## Diagnosis §3 — Does inlining matter? (measured, not guessed)

Experiment: same source, `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` (LTO),
versus the shipping `-O3` build:

| Scene | -O3 (s) | LTO (s) | Delta | Output |
| --- | ---: | ---: | ---: | --- |
| drums 320×200 | 7.70 | 7.50 | **−2.6%** | byte-identical |
| iortest 320×200 | 3.38 | 3.26 | **−3.5%** | byte-identical |

So: **yes, out-of-line call overhead is real, and yes, it is small.** LTO —
the upper bound of what pure inlining can deliver without changing any
structure — recovers ~3%, byte-exact. Inlining alone cannot close a 1.41×
gap; it is worth taking because it is nearly free (Plan 12), but the
structural items of §2 (scan, clones, candidate copies, interpreter depth)
are where the other ~35% lives.

The specific out-of-line leaves worth hand-inlining if LTO is not adopted
wholesale (all measured ≥ 8 M calls/scene): `rayIntersectsAabbForward`,
the `LocalIntersectionClone` constructor plus `ArrayList`'s trivial
ctor/init pair, `RayWithSegments::makeRay`, and
`PriorityQueue<IntersectionCandidate>::siftUp`.

## Diagnosis §4 — Conclusions

1. **The gate gap is two scenes.** drums (118.9 s) *is* the wall time;
   iortest (58.6 s) is the next cap. Optimizing median scenes does not move
   `renderAll.sh` at all. Plans 12–15 must be judged primarily on drums and
   iortest.
2. **The work volume already beats baseline; the orchestration doesn't.**
   Half the primitive tests, three times the cost per test. The remaining
   gap decomposes, in drums, into: Morgan interpreter depth (~41%
   inclusive), clone construction (~4% + the cache pressure of 27 M × 168 B
   candidate and clone traffic), queue copies, and — in union-heavy scenes
   — the linear operand scan.
3. **Inlining: measured at ~3% (LTO, byte-identical).** Take it, don't
   expect it to be the story.
4. **The architecture is not the blocker for the next ~20%.** Every item
   above is fixable inside `render/bakedScene` (build-time structures,
   flatter kernels, slimmer queue payloads) without touching the
   `environment/geometry` / `environment/scene` layering: geometry keeps
   zero transform knowledge, scene keeps parsed data + `TransformStep`
   lists only. Plan 10's Ending B ("structural floor") was declared against
   the *ownership* constraint — none of Plans 12–15 violate it.
5. One genuinely architectural residue remains: the per-ray composite/body
   local-space clone (Plan 10 suspect (d)). Plan 15 attacks the *bakeable
   subset* of it (quadric/plane children under composites) with the same
   elementary-step replay Plan 5 built, extended through the composite's
   own recorded steps — the rest stays declared floor.

## Appendix — Measurement log

- `renderAll.sh`: single run each tree, same session, `/usr/bin/time`.
  Current 120.14 s; baseline 85.37 s. Pole offsets from output `.tga`
  mtimes (first-finished = t0).
- drums: 5 interleaved rounds, table above, binaries verified `mcount`-free.
- Panel: `./scripts/benchmarkPanel.sh` as shipped (its baseline column is
  Plan 10's corrected clean-baseline measurement).
- gprof: `-pg` builds of `472fa48` for drums, iortest, piece3, ntreal,
  spline, shapes2 at 320×200, each scene run from its own directory.
- LTO: `build-lto` with `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`, 3
  interleaved rounds vs `-O3`, outputs `cmp`-identical on drums and
  iortest. Experimental build dirs and binaries deleted afterwards;
  `build/povray` re-verified clean (`nm -D | grep -c mcount` → 0) and the
  golden gate re-run green before closing this plan.
