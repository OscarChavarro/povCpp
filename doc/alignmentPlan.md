# povCpp → VITRAL alignment plan

Companion to `doc/vitralPovrayParallelLearningExperience.md` (the statement of
intent) and `doc/vitralNormalizationAnalysis.md` (the detailed model
comparison). Where those documents say *why* and *what differs*, this one says
*which concrete renames and extractions to do in povCpp `src/`*, in what
order, so the implementation drifts toward the shape of the
[VITRAL library](https://github.com/OscarChavarro/vitral) one gate-green step
at a time.

## Ground rules

1. **One step, one commit, gate green.** Every step below must leave the
   108-scene golden gate byte-identical (`scripts/testAgainstGoldenImages.sh`).
   None of these steps changes pixels; a diff means the step was executed
   wrong.
2. **Renames before semantics.** Steps are ordered so purely mechanical
   renames/extractions come first; the few steps that move a decision point
   (e.g. where the detail mask lives) come after, once the vocabulary is
   settled.
3. **Names are the contract.** Whatever name povCpp stabilizes here is the
   name VITRAL is expected to adopt when it grows the same operation
   (see "Upstream items" at the end). Avoid inventing a third name during
   migration.
4. **Statistics changes must also preserve the verbose report.** For steps
   touching `Statistics`, additionally diff the `-v` text output of a
   representative render before/after: counters must not merely stay
   *correct*, the printed report must stay identical.

## Current vocabulary map (verified 2026-07-05)

| Concept | povCpp today | VITRAL today | Direction |
|---|---|---|---|
| Ray | `RayWithSegments : Ray` | `Ray` | shared base already; povCpp derived name is a misnomer (step 7) |
| Nearest hit (direct) | `virtual doIntersectionFirstHitNoQueue(ray, out, override)` | `virtual bool doIntersectionFirstHit(const Ray&, RayHit*) = 0` | povCpp renames to converge (step 4) |
| Nearest hit (derived from queue) | non-virtual `doIntersectionFirstHit(ray, out)` | — | povCpp renames out of the way (step 4) |
| All crossings (CSG classification) | `virtual doIntersectionForAllRayCrossings(ray, depthQueue, override)` + `...Annotated` | — (missing) | name frozen here; VITRAL adopts it (step 6) |
| Hit detail fill | `virtual doExtraInformation(ray, t, PovRayHit*)` | `virtual doExtraInformation(ray, t, RayHit*)` | aligned name; gating differs (step 3) |
| Surface normal | `virtual normal(result, point[, config])` — public | none (computed inside `doExtraInformation`) | povCpp demotes to helper (step 5) |
| Containment | bare `static constexpr int INSIDE/LIMIT/OUTSIDE` | same constants cast from `enum class Containment` | povCpp adopts the enum (step 2) |
| Detail mask | `RayWithSegments::DETAIL_*`, mask owned by the ray | `RayHit::DETAIL_*`, mask owned by the hit | povCpp moves mask reading to the hit (step 3) |
| Bounds | `AxisAlignedBoundingBox getMinMax() const` | `double* getMinMax() = 0` | povCpp version is superior; VITRAL adopts (upstream) |
| Hit record | `IntersectionCandidate` (= `Intersection` + `IntersectionAttributes`), `PovRayHit` projection at the shading edge | `RayHit` | already bridged by `PovRayHit`; no rename needed |
| Counters | `Statistics` monolith, instance-based, parts-summing ctor | `RaytraceStatistics` static recorder + `SolidTextureStatistics` | povCpp extracts `GeometryStatistics` (step 1); taxonomy reconciled upstream |

Note: `doc/vitralNormalizationAnalysis.md` sections 1–14 were corrected
against the current tree on 2026-07-05 (the former `TransformedGeometry`
descriptions were rewritten); its sections 15–17 remain historical records.

---

## Step 1 — Extract `GeometryStatistics` from `Statistics`

The immediate, self-contained first step, and the worked example of the
statistics-unification direction described in
`vitralPovrayParallelLearningExperience.md`.

**What moves.** The ray↔geometry counters — exactly the ones incremented from
geometry, scene and baked-intersector code:

- The nine primitive test/success pairs: `raySphereTests`, `rayBoxTests`,
  `rayBlobTests`, `rayPlaneTests`, `rayTriangleTests`, `rayQuadricTests`,
  `rayPolyTests`, `rayBicubicTests`, `rayHtFieldTests` (each with its
  `...Succeeded` twin).
- `boundingRegionTests`/`...Succeeded` and
  `clippingRegionTests`/`...Succeeded`.

**What stays in `Statistics`.** Render-level counters: pixels, `numberOfRays`,
super-sampled pixels, `shadowRayTests` (incremented by `DirectLightShader`,
i.e. the shading layer, not geometry), reflected/refracted/transmitted rays,
`usedTime`, and the embedded `SolidTextureStatistics`.

**Shape.** Mirror `SolidTextureStatistics` exactly, since it is the pattern
both trees already share:

- New `src/common/statistics/GeometryStatistics.h/.cpp`. It starts in `src/`
  (app layer), not `base/`: its counters are POV-primitive-specific, and
  promotion into `base/`/VITRAL first requires the shared-taxonomy agreement
  (see upstream items).
- `Statistics` embeds a `GeometryStatistics geometryStatistics;` by value and
  exposes `getGeometryStatistics()` (const and non-const), exactly like
  `getSolidTextureStatistics()`.
- `GeometryStatistics` gets a `reset()` and a parts-summing constructor
  `GeometryStatistics(java::ArrayList<GeometryStatistics*>*)` mirroring
  `SolidTextureStatistics`; `Statistics`' own parts-summing constructor
  delegates the moved fields to it.

**Call sites** (verified by grep, 14 files): `Triangle.cpp`,
`InfinitePlane.cpp`, `ParametricBiCubicSolver.cpp`, `Blob.cpp`, `Box.cpp`,
`HeightField.cpp`, `Quadric.cpp`, `Sphere.cpp`, `PolynomialShape.cpp`,
`Composite.cpp`, `SimpleBody.cpp`, `BakedPlaneIntersector.cpp`,
`BakedQuadricIntersector.cpp` (increments), and `PovRayApplication.cpp` (the
report reader). Change them to
`stats->getGeometryStatistics()->incrementRaySphereTests()` etc. — do **not**
leave forwarding shims on `Statistics`, they would freeze the old API.

**Performance note.** The sub-object is embedded by value, so the extra getter
is offset addressing on the same object — no new indirection on the hot path.
If profiling ever disagrees, `RayWithSegments` can cache a
`GeometryStatistics*` next to its existing `Statistics*`; don't do this
preemptively.

**Verification.** Golden gate byte-identical, plus rule 4 (identical `-v`
statistics text on at least one CSG-heavy scene, e.g. `drums.pov`).

## Step 2 — Adopt VITRAL's `Containment` enum

VITRAL defines `enum class Containment { OUTSIDE = -1, LIMIT = 0, INSIDE = 1 }`
in `vsdk/toolkit/processing/Containment.h` and derives `Geometry`'s `int`
constants from it by `static_cast`. povCpp declares the same three bare
`constexpr int`s on `Geometry`.

- Copy `Containment.h` into `base/src/main/vsdk/toolkit/processing/` (it is
  VITRAL code, so it lands in `base/`, keeping the manual-sync convention).
- Redefine povCpp's `Geometry::INSIDE/LIMIT/OUTSIDE` as
  `static_cast<int>(Containment::...)`, byte-for-byte the VITRAL wording.

Every `doContainmentTest` implementation keeps returning `int`; zero call-site
changes. This is the smallest possible alignment step and a good rehearsal of
the base/-sync workflow.

## Step 3 — Move detail-mask reading onto the hit (`doExtraInformation`, VITRAL style)

VITRAL's protocol: the *caller* creates a `RayHit` carrying
`requiredDetailMask` before intersecting; geometry's `doExtraInformation`
reads `outHit->needsPoint()/needsNormal()/...` and fills only what is asked
(see `Sphere::doExtraInformation` in VITRAL). povCpp currently keeps the mask
on `RayWithSegments` — because the decision is made before any hit exists —
and gates `doExtraInformation` from the outside, in `RenderEngine`.

These are compatible: keep the ray as the mask's *transport* (that part is a
genuine povCpp need, pre-hit), but make the hit the mask's *authority* at
detail-fill time:

1. Add `DETAIL_*` constants, a `requiredDetailMask` field and the `needs*()`
   accessors to `PovRayHit`, with names and values identical to VITRAL's
   `RayHit`. Turn `RayWithSegments::DETAIL_*` into aliases of
   `PovRayHit::DETAIL_*` (single definition).
2. Stamp the mask onto the hit when it is built: `PovRayHit::fromCandidate`
   gains the mask (taken from the ray at the call site in
   `RenderEngine::trace` / the shading pipeline).
3. Change `Geometry::doExtraInformation` (and the overrides in `SimpleBody`,
   `CsgOperand`, and each primitive) to gate on `hit->needsNormal()` instead
   of being unconditionally-normal. Keep `RenderEngine`'s outer
   skip-when-`DETAIL_NONE` as an optimization — VITRAL also has both layers
   (`SimpleRaytracer` builds the mask, the geometry checks it).

After this step, a povCpp `doExtraInformation` override reads exactly like a
VITRAL one modulo the hit type, which is the precondition for ever sharing
those bodies. It also opens the (deliberately deferred) option of filling
`p`/`u`/`v`/tangent lazily the way VITRAL does — do **not** attempt deferred
point computation in the same step; the crossing emitters currently store the
point on `Intersection` and changing that is a semantic move with CSG
implications.

## Step 4 — Converge on `doIntersectionFirstHit`

Both trees use the name, for different primitives (the one real naming
collision):

- VITRAL: `virtual bool doIntersectionFirstHit(const Ray&, RayHit*)` — the
  direct nearest-hit primitive.
- povCpp: non-virtual `doIntersectionFirstHit(ray, IntersectionCandidate&)` —
  an adapter that runs all-crossings into a pooled queue and peeks; plus
  `virtual doIntersectionFirstHitNoQueue(ray, out, override)` — the actual
  direct nearest-hit (10 occurrences).

Two mechanical renames, in this order:

1. Queue adapter → `doIntersectionFirstHitViaCrossings`. The name states what
   it is: the POV fallback that derives nearest-hit from the CSG primitive.
2. `doIntersectionFirstHitNoQueue` → `doIntersectionFirstHit`. Now the shared
   name means the same thing in both trees: *direct nearest-hit, no queue*.

Signature stays povCpp-shaped for now (`RayWithSegments*`,
`IntersectionCandidate&`); converging parameter types is part of the eventual
geometry migration, not this plan.

## Step 5 — Demote `normal()` to an implementation detail

VITRAL `Geometry` has no public `normal()`; normal production is inside
`doExtraInformation`. In povCpp, `normal(result, point[, config])` is public
on `Geometry` and every primitive, but (verified) it has **no callers outside
`src/environment/geometry`** — the default `doExtraInformation` is the only
consumer, plus intra-package uses.

- Make the two `normal(...)` overloads `protected`, renamed
  `computeSurfaceNormal(...)` so the public detail contract is
  `doExtraInformation` alone, matching VITRAL's surface.
- Re-grep for `->normal(` before executing; if a new external caller appeared
  since this plan was written, route it through `doExtraInformation` first.

Best done after step 3, when `doExtraInformation` overrides already gate on
the hit mask and the helper's role is obvious.

## Step 6 — Freeze the CSG ray-classification contract (names VITRAL will adopt)

The learning-experience document commits to giving VITRAL POV-style CSG
(both De Morgan-rules and Roth/segments strategies). That requires VITRAL to
gain the all-crossings classification primitive that povCpp already has. The
alignment work on the povCpp side is to **stop treating these names as
internal** and freeze them as the proposed VITRAL contract:

- `doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride)` —
  keep the name as-is (86 occurrences in 39 files; it is long but precise,
  already `do*`-styled, and already the documented term in
  `vitralNormalizationAnalysis.md`). Renaming it buys nothing.
- `doIntersectionForAllRayCrossingsAnnotated(...)` +
  `GeometryIntersectionEmissionContext` — keep; the annotated variant is what
  lets a wrapper (CSG operand, composite) stamp ownership onto emitted
  crossings without re-sorting.
- The supporting types that travel with the contract:
  `IntersectionCandidate` (geometric `Intersection` + attribution),
  `RaySegments`/`RaySegmentCrossing` (Roth strategy), and the two strategy
  classes `ConstructiveSolidGeometryByMorganRules` /
  `ConstructiveSolidGeometryByRaySegment`.

VITRAL adoption sketch (executed in the VITRAL repo, recorded here so both
trees aim at the same target): `Geometry` gains a **non-pure** virtual
`doIntersectionForAllRayCrossings` with an empty default, so existing VITRAL
shapes are unaffected; VITRAL then needs a crossing record — whether that is a
list of `RayHit`s or a port of `IntersectionCandidate` is exactly the
hit-record taxonomy question flagged in `vitralNormalizationAnalysis.md` §3
and must be settled there first.

## Step 7 — Rename `RayWithSegments` → `TracingRay`

The name is a historical misnomer: the class stores no segments (segments live
in `RaySegments`, used only by the Roth CSG strategy). Its actual payload is
the per-trace workspace riding on the ray: quadric-term cache, AABB
slab-reciprocal cache, containing-media stacks, shadow/primary classification,
detail mask, and the `Statistics`/config/queue-pool service pointers.

- Recommended name: `TracingRay` (a `Ray` plus everything one trace needs).
  Alternatives considered: `RenderRay`, `RayTraversalState`.
- Surface: 352 occurrences in 83 files — large but purely mechanical
  (sed-able), which is why it is sequenced late: by then the method renames
  will have proven the rename-then-gate workflow on smaller surfaces.
- Include the file rename (`element/RayWithSegments.*` → `element/TracingRay.*`)
  and guard rename in the same commit.

## Step 8 (optional, deferred) — Mirror VITRAL's geometry taxonomy

VITRAL inserts intermediate bases: `Surface : Geometry`,
`Volume : Geometry`, `Solid : Volume` (where `Volume` carries
`exportToPolyhedralBoundedSolid()`). povCpp's *directories* already mirror
this (`geometry/element`, `geometry/surface`, `geometry/volume`) but its
classes all extend `Geometry` directly.

Do **not** introduce the intermediate classes yet: povCpp cannot honour
`Volume`'s polyhedral-export contract, so the bases would be empty name-props.
Introduce them only when the actual geometry migration to VITRAL begins and
the export capability arrives with it. Recorded here so nobody "fixes" the
hierarchy prematurely.

## Step 9 — Restore the original console-message surface

Independent of steps 1–8 (no shared files beyond incidental overlap; can run
in parallel with them). The oracle is commit `3665f8f` ("Modernized C++ build
of original Povray 1.0 code from 1992"): under normal conditions the program
may only print what that build printed, the same or reworded — everything
else, in particular instrumentation output, must be deleted.

**The original message surface** (verified by grepping the `3665f8f` tree;
255 emission sites, all falling into six categories):

1. **Credits banner** (stderr): "Persistence of Vision Raytracer Ver 1.0",
   copyright, DKBTrace attribution, contributing-authors table.
2. **Options echo** (stdout): the "POV-Ray  Options in effect: ..." line
   reprinting the effective `+/-` flags.
3. **Usage/help text** (stdout) on invalid options.
4. **Flow and progress**: "Parsing...", "Rendering...", "Displaying...",
   "POV-Ray rendering %s to %s :", the per-line
   "Res %4d X %4d. Calc line %4d of %4d" with "." dots and
   " supersampled %d times.", tokenizer include-file dots, "Done Tracing".
5. **Statistics block** (`-v`/stat file): resolution, rays/pixels counts, the
   "Ray->Shape Intersection Tests" table (Sphere, Plane, Triangle, Quadric,
   Blob, Box, Quartic\Poly, Bezier Patch, Height Fld, Hght Fld Box, Bounds,
   Clips), Calls to Noise/DNoise, Shadow Ray Tests, Reflected/Refracted/
   Transmitted Rays, Time For Trace.
6. **Errors and warnings** (stderr): cannot open input/include/output file,
   out of memory, "Error in %s line %d" parse errors, illegal character,
   unrecognized output format, invalid option, degenerate-triangle warning.

**The task.** Inventory every output site in the current tree — not only
`printf`/`fprintf` but every channel: `std::cout`/`std::cerr`,
`PovRayApplication::printProgress`, `base/`'s `Logger`,
`java::System::err.printf` (`PersistenceElement.cpp`), and any `PrintStream`
use — and classify each site:

- **Keep**: matches or rewords an original category-1–6 message.
- **Keep, conditionally**: messages for features that did not exist in 1992
  (e.g. `-parallel`), only if they follow the original tone and format and
  appear only when the feature is engaged.
- **Delete**: everything else — instrumentation, per-phase diagnostics,
  debug traces, progress chatter not in the original. If a deleted message
  was still diagnostically useful, its place is a debugger or a scratch
  branch, not the shipped binary.

Current state (verified 2026-07-05): `src/` is already close — 12 direct
`fprintf` sites, all category-4/5/6 derived (`ParseErrorReporter`,
`Tokenizer`, `SceneParser`'s degenerate-triangle warning, `RenderEngine`'s
progress line). The audit's real work is the helper channels (`base/` logger
and persistence errors) and confirming nothing instrumentation-like is
reachable in a default or `-v` render.

**Verification.** Render one scene (e.g. `drums.pov`) with default flags and
with `-v`, capturing stdout+stderr, against the same runs of a `3665f8f`
build; every emitted line must map to an original category. Golden gate
unaffected (messages never touch pixels); statistics text falls under ground
rule 4.

## Step 10 — Remove process-narration comments (let the code speak)

Independent of steps 1–8; prioritize `render/bakedScene` first.

**Baseline.** The `3665f8f` tree comments sparsely: a boxed file header
(module purpose, copyright, POV-Team attribution) plus occasional short
inline notes (~10–15% of lines, e.g. `spheres.cpp` 34/222). The current tree
accumulated a different kind of comment during the performance-review cycles:
agent-authored *process narration* — which plan/phase introduced a change,
why the change is correct, measured call counts and timings, review history.
Verified hot spots: `render/bakedScene` (`BakedSceneBuilder.cpp` 133 comment
lines of 1068, `BakedScene.h` 100/310, `CsgMorganUnionTrace.h` 75/514),
`RayWithSegments.h`, `CsgOperand.h`, `SimpleBody.h`, `TransformStep.h`,
`RenderContext.h`. 77 comment blocks across 17 files cite plans or analysis
docs; **22 of them reference `doc/performanceReviewPlan*.md` files that were
deleted on 2026-07-04** — already-dangling pointers.

**Deletion rule.** A comment is deleted when it narrates process or history:
what plan/phase/commit introduced the code, why the edit was correct, what
was measured, what the previous implementation did, what a future phase will
do. That is commit-message and `doc/performanceNotes.md` material, and most
of it already lives there.

**Keep rule.** Two kinds survive:

- **References**: comments citing sources — algorithm papers, the original
  POV-Ray 1.0 code/docs, and cross-references to *existing* project docs when
  they state a live constraint (a citation to a deleted plan doc is not a
  reference, it is a dangling pointer — delete or repoint to
  `performanceNotes.md`).
- **Invariants the code cannot express**: a constraint that would be violated
  by an innocent-looking edit (e.g. "do not renormalize between nested
  non-uniform scales; compose first, normalize once at the top"). Reword
  these to state the constraint only — strip the history of how it was
  discovered.

Everything the comment explains that the code could say itself (what the next
lines do, what a well-named function already names) is deleted without
replacement — rename the symbol instead if the name was the problem.

**Migration rule.** Before deleting a rationale that is genuinely valuable
and not yet recorded, move its essence to `doc/performanceNotes.md`; then
delete the comment.

**Order.** `render/bakedScene` (heaviest, explicit priority), then
`environment/geometry/element` + CSG headers (`RayWithSegments.h`,
`CsgOperand.h`), then `environment/scene`, then the remainder of the 17
flagged files, then a final sweep grep for `Plan [0-9]`, `Phase [0-9]`,
`doc/performanceReviewPlan`, `§1[0-9]` in `src/` and `base/`.

**Verification.** Comments cannot change codegen, but the gate runs anyway
(ground rule 1); the sweep greps above must come back empty except inside
`doc/`.

## Cosmetic backlog (fold into steps that already touch the file)

- Include-guard style: povCpp `src/` uses short guards (`__GEOMETRY__`,
  `__RAY_WITH_SEGMENTS__`); VITRAL and `base/` use path-based guards
  (`__VSDK_TOOLKIT_..._H__`). Adopt path-based guards in `src/` files as they
  get touched by the steps above; not worth standalone commits.

