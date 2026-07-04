# Performance Notes

Reference points (nothing in between matters here; git has the history):

- **Baseline**: commit `4af1a75e5b7356600bec34e12a4882560994a058` — the last
  tree with the original, parse-time-destructive baking model.
- **Current**: commit `2dfe55683688dfa87f9f4445d6e57f7beaf7b270`.

Measured on the same machine, interleaved runs, LTO release builds:

| Benchmark | Baseline | Current | Ratio |
| --- | ---: | ---: | ---: |
| `scripts/renderAll.sh` (108 scenes, 1280×800) | 85 s | 112 s | 1.32× |
| `drums` 320×200 | 5.14 s | 7.22 s | 1.40× |
| `iortest` 320×200 | 2.10 s | 3.28 s | 1.56× |

`renderAll.sh` backgrounds all scenes, so its wall time is the *slowest
scene*, not the sum. The whole suite gap is two pole scenes: `drums` (the
wall time itself) and `iortest` (the second cap). The median scene is at
parity or faster than the baseline.

## Bake strategy: baseline

The baseline baked **destructively at parse time**:

- Transforms were folded directly into quadric coefficients
  (`Quadric::transformQuadric` rewrote the ten coefficients in place);
  shapes owned their transforms and rays were mutated in place when
  entering local spaces.
- Per-viewpoint constants were cached in **mutable fields on the shared
  shape objects** — free to read, but a data race under any concurrency.
  The baseline binary cannot run multithreaded correctly at all.
- Intersection results were raw depth doubles pushed into flat arrays,
  walked from a flat linked list of objects. No per-crossing annotations.

This is why the baseline is fast: almost no machinery around each
primitive test (~31 M primitive tests/s on drums), at the cost of being
single-threaded by construction and unable to carry the per-operand
material/annotation data that several texture-correctness features need.

## Bake strategy: current

Baking is a **separate compilation step** at the pre-render boundary,
producing a read-only model in `render/bakedScene`:

- `environment/geometry` stays transform-free (pure intersection math) and
  `environment/scene` holds only parsed data plus `TransformStep` lists.
  `BakedSceneBuilder` compiles those into flat records
  (`BakedTraceableObject`, `BakedCsgProgram`) with value-stored baked
  geometry copies — no per-ray pointer chases into the parsed model.
- Quadric/plane/sphere operands are collapsed to world space by
  **replaying the elementary transform steps with the baseline's exact
  formulas and operation order**, so the baked math is bit-identical to
  the baseline's parse-time rewriting. (Composing steps into one matrix is
  a known dead end: not bit-identical.)
- CSG objects compile to plans: a fused kernel for the dominant
  `SingleCorePlaneIntersection` pattern, and a generic Morgan interpreter
  for everything else (correct nested-CSG semantics the baseline lacked).
- Transform pushdown folds nested operand transforms at build time; the
  hot core-plane kernel skips the (provably identity) matrix work when
  `pushdownFolded` is set.
- Per-ray culling: AABB slab tests with cached direction reciprocals, plus
  build-time cull bins over large union operand lists.
- Per-viewpoint constants live in an indexed `RaySharedCache` instead of
  mutable shape fields; `effectiveMaterial` is resolved at build time.
- Remaining render-time fallbacks to parsed objects are limited and
  counted (see `bakedSceneFallbacks.md`).

Where baking cannot reach — composites and unbakeable primitives — rays
are cloned into local space per test (`LocalIntersectionClone`), and every
crossing becomes an `IntersectionCandidate` (~176 B, with detail-owner and
per-operand material annotations) ordered in a binary heap.

## Thread safety

`-parallel` (tile-based, deterministic) is a **non-negotiable product
requirement**. Consequences:

- No mutable state on shared scene/baked objects, ever. All per-ray and
  per-viewpoint scratch is task-owned or indexed (`RaySharedCache`,
  per-task `TextureUtils`).
- The one legitimately shared RNG (texture randomness in `LambertShader`)
  is mutex-guarded.
- Parallel and serial renders are byte-identical; the golden-image gate
  (`scripts/renderAll.sh` + `scripts/testAgainstGoldenImages.sh`) is the
  correctness bar for any performance change.

## Product trade-offs behind the residual gap

On drums the current tree executes **half** the baseline's primitive tests
(81 M vs 161 M — the compiled CSG plans and culling genuinely work) but at
roughly **3× the cost per test**. The gap is orchestration, and each layer
of it was bought deliberately:

1. **Parallel determinism** — indexed cache lookups instead of free reads
   of mutable shape fields.
2. **Layering** — transform-free geometry means per-ray clones on
   composite/unbakeable paths (~27 M clones per drums render).
3. **Deterministic annotated candidates** — the heap of 176-byte
   candidates carries the machinery several texture fixes depend on; the
   baseline pushed bare doubles.
4. **CSG generality** — the Morgan interpreter is correct for nestings the
   baseline got wrong.

Reaching 85 s parity would require un-buying at least one of these; that
is a product decision, not an optimization. Within the constraints, the
remaining hot spots are the composite clone volume (shadow rays dominate
it: ~87% of drums' composite-space traces are shadow rays) and the
interpreter depth on generic-morgan programs.

## Known defects and hazards

- **CSG compiled first-hit is non-minimal**: `BakedCsgTrace`'s first-hit
  path for `SingleCorePlaneIntersection` can return a qualifying hit that
  is not the nearest crossing. All current consumers only need a boolean
  (bounding pass/fail, shadow-window membership), so it is latent — but
  any new consumer that compares its `t` numerically against another
  object's (e.g. a per-child first-hit merge for composite shadow rays)
  will produce wrong images. Fix this before building such a consumer.
- Composite children's `worldBounds` are composite-local, not world; a
  child's `noShadowFlag` is not consulted on nested paths. Both matter if
  composites are ever flattened.
- Measurement discipline: interleave runs within one session (absolute
  timings drift across sessions); `CMakeLists.txt` forces the runtime
  output dir to `build/`, so an instrumented (`-pg`) build silently
  overwrites `build/povray` — verify with `nm -D build/povray | grep -c
  mcount` → 0; stray `output/panel/*.tga` files break the golden gate.
