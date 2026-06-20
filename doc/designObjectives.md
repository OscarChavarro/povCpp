# Design Objectives — Assessment Report

A faithful C++ adaptation of POV-Ray 1.0 (1991/1992), a classic recursive ray
tracer of the kind popularized in the 1990s. This document reviews the current
state of the code under `src/` and `base/` against the project's five stated
objectives.

## Project objectives

1. **Academic study base** — serve as a clear, well-organized reference for learning.
2. **C++11 alignment** — deliberately stay on C++11 (no newer standards) to keep
   the implementation easy to migrate.
3. **JDK alignment** — mirror the JDK so a Java port can be built for C++ vs Java
   performance comparisons on this specific problem domain.
4. **Parallel/concurrent version (pthreads)** — keep the pure-CPU serial ray tracer
   core pure-CPU, but exploit multi-core systems.
5. **Vulkan 1.3 (GPU) version** — re-implement the algorithms on the GPU aiming for
   an AE-metric close-to-similarity match against the CPU reference (not a bit-exact,
   pixel-perfect match), and organize the CPU version in a clear architecture that
   aids the Vulkan implementation.

## Observed structure

- **`src/`** (~23.5k LOC): the ray tracer proper, in strict getLayers
  `app → io → environment → render → base`.
- **`base/`** (~8.8k LOC): a foundational `vsdk` toolkit plus a **C++ mirror of the
  JDK** (`java::String`, `java::Math`, `java::PriorityQueue`, `java::ArrayList`,
  `java::io::*`) — already aligned with objective #3.
- Key separation already achieved: `Geometry` (pure intersection math) vs
  `SimpleBody` (material + transform in the scene layer). The shader pipeline is
  decoupled from the trace loop through `TraceService` (a function-pointer pair).
- The render driver is now split into collaborating classes: `RenderEngine`
  (pure sampling driver), `RenderWorker` (per-ray scratch + line buffers),
  `AdaptiveAntiAliasing` (super-sampling/jitter), `RayShaderPipeline` (surface
  shading) and `RenderImageWriter` (scanline persistence + interrupted-render
  resume), instead of one monolithic engine.

## Strengths

| # | Aspect | Evidence | Why it matters (objective) |
|---|--------|----------|----------------------------|
| P1 | **Strict layered architecture** with a verified dependency graph | `README` architecture; `app→io→environment→render→base` | #1 didactic, #5 clean base for Vulkan |
| P2 | **Geometry = pure math, no material** | `Geometry.h`, `SimpleBody.h` | #5: the GPU kernel only needs the geometric half |
| P3 | **One shader = one class** (Lambert, Phong, Shadow, Mirror, Refraction, Fog…) | `src/render/shaders/*` | #1 readable; #5 maps almost 1:1 to GPU shading stages |
| P4 | **Recursion decoupled via `TraceService`** (function-pointer pair) | `TraceService.h` | #1 shaders independent of the loop; extension point |
| P5 | **Golden image testing** — bit-exact vs the original POV-Ray reference (108 scenes, ImageMagick AE) | `README`, `scripts/testAgainstGoldenImages.sh` | #5 provides the oracle to validate the GPU port by AE similarity |
| P6 | **Disciplined C++11**: `constexpr`, `nullptr`, `override`, `= default`, no extensions | `CMakeLists` (`STANDARD 11`, `EXTENSIONS OFF`), `RenderEngine.h` | #2 clean migration |
| P7 | **JDK mirror already built** (`java::*`) | `base/src/main/java/**` | #3: the Java port will be nearly mechanical |
| P8 | **No mutable global state** (singletons/globals removed, bridge pattern) | globals grep = 0; `globals_removal` notes | #4 prerequisite for parallelism |
| P9 | **Per-engine intersection pooling** | `IntersectionPriorityQueuePool` in `RenderEngine` | #4 avoids per-ray malloc; base for per-thread |
| P10 | **Modern, strict build** (`-Wall -pedantic`, clang-tidy/format present) | `CMakeLists`, `.clang-tidy` | #1/#2 sustained quality |
| P11 | **Iterative reflection/refraction traversal** with weighted events and an explicit frame stack | `TraceService`, `RenderEngine::trace` | #5 removes CPU call recursion and prototypes the GPU execution model |
| P12 | **Per-ray scratch isolated in `RenderWorker`** (ray, primary ray, trace level, line buffers); `trace`/AA/shading parameterized over a worker | `RenderWorker.h`, `RenderEngine::trace(RenderWorker&, …)` | #4 prerequisite for one-worker-per-thread parallelism |
| P13 | **Image writing separated from the sampling driver** — `RenderImageWriter` (private state) owns scanline disk persistence, interrupted-render resume and the per-line buffer flush; `RenderEngine` delegates through a private member | `RenderImageWriter.h/.cpp`, `RenderEngine::readRenderedPart`/`startTracing` | #1 single responsibility; #4/#5 the driver can be swapped (parallel/GPU) without dragging file I/O |

## Areas to improve

| # | Aspect | Evidence / risk | Objective | Suggestion |
|---|--------|-----------------|-----------|------------|
| M1 | **Antialiasing couples neighboring scanlines** (`previousLine`/`currentLine`) | `AdaptiveAntiAliasing::doAntiAliasing`, `RenderWorker` line buffers/`swapLines` | **#4** | Now isolated in its own class, but it still reads the adjacent scanline. Blocks trivial tiling; redesign into independent tiles or resolve AA in two passes |
| M2 | **Statistics incremented per pixel without synchronization** | `incrementNumberOfPixels()` in `RenderEngine::startTracing` loop | **#4** | Per-thread counters with final reduction, or `std::atomic` |
| M3 | **187 `new` vs 68 `delete`, zero smart pointers** | memory grep | #1, #2, #3 | Leak and double-management risk; the imbalance complicates the mapping to Java's GC. Consider value semantics or `unique_ptr` (still C++11) |
| M4 | **Virtual dispatch + AoS of pointers in geometry** (`virtual allIntersections`, `PriorityQueue<Intersection>`) | `Geometry.h`, primitive headers | **#5** | GPU/SPIR-V has no virtuals or recursion; it will need a tagged-union/SoA. Plan a flat POD scene representation early |
| M5 | **`double` throughout the pipeline** | `Vector3Dd`, `Matrix4x4d`, shaders | **#5** | FP64 is slow/scarce on the GPU. Since the GPU goal is AE-metric similarity (not bit-exact), decide the precision strategy and the acceptable AE tolerance early |
| M6 | **No unit-test scaffolding** (only whole-image golden tests) | no unit-test directory | #1, #4, #5 | The golden test does not localize per-module regressions; missing `Vector3Dd`/`Matrix4x4d`/solver tests that would also pin the contract for the ports |

## Tile-based parallelism (`-parallel`) — readiness assessment

Target: a `-parallel` flag that detects `nproc`, spins up `nproc` threads via
`java::Executors::newFixedThreadPool` (the pthread-backed wrapper in
`base/.../util/concurrent`), and renders the frame as independent **tiles**.
The render layer is already *closer* than the rest of the codebase thanks to P12
(per-ray scratch lives in `RenderWorker`, and `trace`/AA/shading are parameterized
over a worker) and P8/P9 (no globals, per-engine pooling). What remains are the
following design-blocking items, in priority order.

| # | Blocker | Evidence | Why it blocks tiling | Direction |
|---|---------|----------|----------------------|-----------|
| B1 | **One `RenderEngine` owns exactly one `RenderWorker`, one `IntersectionPriorityQueuePool` and one `AdaptiveAntiAliasing`** | `RenderEngine.h:16-18`; `getWorker()` returns the single member | All threads would share one worker (ray scratch, line buffers) and one intersection pool → data races. Need **one worker + one pool per thread** | Make the per-thread state a `RenderTask`/context object (worker + pool), created `nproc` times; keep `RenderEngine` as the shared, read-only sampling logic |
| B2 | **`createRay` injects the engine-owned pool into every ray** | `RenderEngine.cpp:53` `localRay->setIntersectionQueuePool(&intersectionQueuePool)` | Couples ray scratch to the single shared pool even though `trace` already takes a `RenderWorker&` | Pass the pool (or the owning worker) as a parameter to `createRay`/`trace`; move the pool into the per-thread context (B1) |
| B3 | **Tile/scanline window is carried in the shared `RenderingConfiguration`** | `startTracing` reads `getFirstLine()/getLastLine()`; `RenderImageWriter::readRenderedPart` calls `config.setFirstLine()` | Tiles must each render a different sub-window; mutating a shared config per tile is a race and breaks the resume logic | Pass tile bounds (`y0,y1[,x0,x1]`) as explicit parameters to a new per-tile entry point; keep `config` immutable during tracing |
| B4 | **AA reads the adjacent scanline (M1)** | `AdaptiveAntiAliasing::doAntiAliasing`, `RenderWorker::swapLines` | Horizontal tile/band seams need the neighbor row that another thread owns | Per-band: render a one-scanline **halo** overlap; or resolve AA in a **second pass** over a full-frame buffer once all base samples exist |
| B5 | **`Statistics` mutated per pixel/ray without synchronization (M2)** | `incrementNumberOfPixels/Rays` in `startTracing`/`trace`; primitive test counters | Concurrent `long++` is a race; counters undercount | **Per-thread `Statistics`**, reduced (summed) once at join. Cleaner than atomics and maps to the Java port |
| B6 | **`SolidTextureStatistics::callsToNoise/DNoise` incremented inside `noise()`** | `ProceduralNoise.cpp:200,269` (`solidTextureStatistics->callsToNoise++`) | Same race as B5 but hidden in the shared `TextureUtils` reached via `RenderContext` | Give each thread its own `SolidTextureStatistics` sink (per-thread noise stats), reduce at join. Noise **tables** are `const`-read — safe to share |
| B7 | **Output is streamed one scanline at a time, in order** | `RenderImageWriter::outputLine/writeScanline`; `RenderOutput::writeLine(line, lineNumber)` | Tiles finish out of order; the current writer assumes monotonically increasing lines and writes the *previous* line to avoid partial AA | Render into a **full-frame in-memory buffer**, then persist sequentially after join; or add a completion-ordered writer that buffers until the next expected line is ready |
| B8 | **Single `fatalErrorFound` / `stopFlag` shared, non-atomic** | `RenderEngine.h:27`; `RenderRuntimeState::getStopFlag()` | Abort signalling across threads is racy (benign today, undefined under threads) | Make the abort flag atomic (`java::util::concurrent::atomic`) and report-once via a guarded flag |
| B9 | **No CPU-count detection and no `-parallel` option plumbing** | `CommandLineOptions.*`, `RenderingConfiguration` (no thread flag) | Nothing reads `nproc` or selects the parallel driver | Add `Runtime.availableProcessors()`-style helper (`sysconf(_SC_NPROCESSORS_ONLN)`), a `PARALLEL` flag + thread count in `RenderingConfiguration`, parsed in `CommandLineOptions` |

**Assets already in place** (reduce the work): `trace(RenderWorker&, …)` is
worker-parameterized (P12); `RenderContext` is read-mostly (`config` and `scene`
are `const` references) so it is safe to share across threads once B3/B5/B6 remove
the writes; the `Executors`/`ExecutorService`/`Future`/`Callable` wrapper already
exists and is pthread-backed; no global state to untangle (P8).

**Recommended tiling shape:** start with **horizontal bands** (full-width strips of
`height / nproc` rows). They keep `createRay`'s `x` math untouched, make B7 a simple
contiguous-range write, and reduce B4 to a single shared boundary row per seam
(solved by a 1-row halo). Rectangular tiles can come later for better load balance.

## Reading by objective

- **#1 Academic base** — Well on track (P1–P4, P7, P13); the sampling driver is now
  cleanly separated from image writing. The main item left reducing clarity: M6
  (no unit tests documenting contracts).
- **#2 C++11 migratable** — Solid (P6, P10). The only real pending item: M3 (manual
  memory management; optional `unique_ptr` use is still C++11 and reduces risk
  without breaking the version discipline).
- **#3 JDK alignment** — Ahead thanks to P7. The obstacle is M3: raw `new`/`delete`
  and pointer aliasing do not map cleanly to GC; the more value semantics, the more
  mechanical the Java port.
- **#4 pthreads parallelism** — The largest pending work. P12 already moved per-ray
  scratch out of `RenderEngine` into `RenderWorker`. The concrete `-parallel`
  (tile-based) blockers are now enumerated in **Tile-based parallelism — readiness
  assessment** (B1–B9): per-thread worker+pool (B1/B2), tile bounds out of the shared
  config (B3), AA seam handling (B4, the old M1), per-thread stats reduction (B5/B6,
  the old M2 plus the hidden noise counters), out-of-order tile output (B7), atomic
  abort (B8) and CPU-count/option plumbing (B9). The good news: `trace` is already
  worker-parameterized (P12), `RenderContext` is read-mostly, the pthread `Executors`
  wrapper exists, and P8/P9 already removed the global obstacles.
- **#5 Vulkan 1.3** — The goal here is an AE-metric *close-to-similarity* match with
  the CPU reference, not a bit-exact one — that relaxation is what makes the GPU port
  tractable. The architecture helps (P2, P3, P5 as oracle), but M4 (virtuals/AoS),
  M5 (double precision) remains a deep decision to resolve **before** writing SPIR-V.
  P11 resolves CPU call recursion and provides the execution model to port; the next
  prototype should flatten the scene into POD data, then measure GPU output against
  the CPU reference with the existing AE tooling under an agreed tolerance.
