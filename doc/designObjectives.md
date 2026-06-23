# Design Objectives

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
4. **Parallel/concurrent version (pthreads)** — keep the pure (not using external dependencies such as STL, glm, OpenGL or others) CPU ray tracer
   compatible with serial and concurrent multi-thread implementations.
5. **Vulkan 1.3 (GPU) version** — re-implement the algorithms on the GPU aiming for
   an AE-metric close-to-similarity match against the CPU reference (not a bit-exact,
   pixel-perfect match), and organize the CPU version in a clear architecture that
   aids the understanding and development of the Vulkan API implementation.

## Observed structure

- **`src/`** (~25.8k LOC): the ray tracer proper, in strict layers
  `app → io → environment → render → base`.
- **`base/`** (~8.6k LOC): a foundational `vsdk` toolkit plus a **C++ mirror of the
  JDK** (`java::String`, `java::Math`, `java::PriorityQueue`, `java::ArrayList`,
  `java::io::*`) — already aligned with objective #3.
- Key separation already achieved: `Geometry` (pure intersection math) vs
  `SimpleBody` (material + transform in the scene layer, behind the `Material`
  interface). The shader pipeline is decoupled from the trace loop through
  `TraceService` (a function-pointer pair).
- The procedural-texture engine is now two polymorphic hierarchies under
  `environment/material/` — `SolidTexturePigment` (colour patterns) and
  `SolidTextureNormal` (bump patterns) — one concrete class per POV-Ray pattern,
  composed into the immutable `PovRayMaterial` by `PovRayMaterialBuilder`.
- The render driver is now split into collaborating classes: `RenderEngine`
  (pure sampling driver), `RenderWorker` (per-ray scratch + line buffers),
  `AdaptiveAntiAliasing` (super-sampling/jitter), `RayShaderPipeline` (surface
  shading) and `RenderImageWriter` (scanline persistence + interrupted-render
  resume), instead of one monolithic engine.
- The renderer runs **serial or tile-parallel from the same sampling code**:
  `-parallel[N]` detects `nproc`, splits the frame into horizontal bands via
  `RasterTileGenerator`, and renders each band on its own `RenderTask` (per-thread
  `RenderWorker`, intersection pool, `Statistics` and `TextureUtils`) through the
  pthread-backed `java::Executors` pool, reducing the per-thread statistics at
  join — **objective #4 fulfilled**, not a future step.

## Strengths

| # | Aspect | Evidence | Why it matters (objective) |
|---|--------|----------|----------------------------|
| P1 | **Strict layered architecture** with a verified dependency graph | `README` architecture; `app→io→environment→render→base` | #1 didactic, #5 clean base for Vulkan |
| P2 | **Geometry = pure math, no material** | `Geometry.h`, `SimpleBody.h` | #5: the GPU kernel only needs the geometric half |
| P3 | **One unit = one class across three families** — shaders (Lambert, Phong, Shadow, Mirror, Refraction, Fog…), pigments (Wood, Marble, Bozo, Checker…) and normals (Waves, Ripples, BumpMap…) | `src/render/shaders/*`, `src/environment/material/pigment/*`, `src/environment/material/normal/*` | #1 readable; #5 maps almost 1:1 to GPU shading stages |
| P4 | **Recursion decoupled via `TraceService`** (function-pointer pair) | `TraceService.h` | #1 shaders independent of the loop; extension point |
| P5 | **Golden image testing** — bit-exact vs the original POV-Ray reference (108 scenes, ImageMagick AE) | `README`, `scripts/testAgainstGoldenImages.sh` | #5 provides the oracle to validate the GPU port by AE similarity |
| P6 | **Disciplined C++11**: `constexpr`, `nullptr`, `override`, `= default`, no extensions | `CMakeLists` (`STANDARD 11`, `EXTENSIONS OFF`), `RenderEngine.h` | #2 clean migration |
| P7 | **JDK mirror already built** (`java::*`) | `base/src/main/java/**` | #3: the Java port will be nearly mechanical |
| P8 | **No mutable global state** (singletons/globals removed, bridge pattern) | globals grep = 0; `globals_removal` notes | #4 prerequisite for parallelism |
| P9 | **Per-engine intersection pooling** | `PriorityQueuePool` in `RenderEngine` | #4 avoids per-ray malloc; base for per-thread |
| P10 | **Modern, strict build** (`-Wall -pedantic`, clang-tidy/format present) | `CMakeLists`, `.clang-tidy` | #1/#2 sustained quality |
| P11 | **Iterative reflection/refraction traversal** with weighted events and an explicit frame stack | `TraceService`, `RenderEngine::trace` | #5 removes CPU call recursion and prototypes the GPU execution model |
| P12 | **Per-ray scratch isolated in `RenderWorker`** (ray, primary ray, trace level, line buffers); `trace`/AA/shading parameterized over a worker | `RenderWorker.h`, `RenderEngine::trace(RenderWorker&, …)` | #4 prerequisite for one-worker-per-thread parallelism |
| P13 | **Image writing separated from the sampling driver** — `RenderImageWriter` (private state) owns scanline disk persistence, interrupted-render resume and the per-line buffer flush; `RenderEngine` delegates through a private member | `RenderImageWriter.h/.cpp`, `RenderEngine::readRenderedPart`/`startTracing` | #1 single responsibility; #4/#5 the driver can be swapped (parallel/GPU) without dragging file I/O |
| P14 | **Procedural texturing is a polymorphic class hierarchy** — `SolidTexturePigment` (15 concrete patterns) and `SolidTextureNormal` (7 concrete patterns), each over an abstract base | `src/environment/material/pigment/*`, `src/environment/material/normal/*` | #1 readable; #3 maps cleanly to a Java class hierarchy; #5 each pattern is an isolated GPU shading variant |
| P15 | **`PovRayMaterial` immutable + constants centralized** — const getters and constructor-only state, assembled by `PovRayMaterialBuilder`; tolerances/epsilons consolidated in `Config` | `PovRayMaterial.h`, `Config.h`, `PovRayMaterialBuilder.*` | #4 a read-only material is safe to share across threads; #1/#2 no scattered magic numbers |
| P16 | **Manual but robust memory management, verified by valgrind** — every class has an explicit destructor with a correct owner and lifetime; every shared resource is owned by exactly one place. **No smart pointers, by design** — explicit raw ownership throughout, for backwards-portability to pre-C++11 toolchains and forward-portability to a future Rust port (explicit, traceable ownership maps directly to Rust's move/borrow model). The full 108-scene corpus runs leak-free and error-free under valgrind memcheck | valgrind memcheck, full 108-scene corpus | #2 stays on raw `new`/`delete` only, no C++11+ ownership types to migrate away from; #1 ownership is explicit and traceable, not hidden behind a smart-pointer type |
| P17 | **Tile-based `-parallel` renderer, gate-green** — `-parallel[N]` detects `nproc`, splits the frame into horizontal bands (`RasterTileGenerator`) and renders each on its own `RenderTask` (own `RenderWorker`, intersection pool, `Statistics`, `TextureUtils`) via the pthread-backed `java::Executors` pool; tile bounds are explicit parameters, per-thread statistics/noise counters are reduced at join through their parts-summing constructors, the frame is assembled in a full-frame buffer and persisted after join, AA seams use a one-row halo, and abort is an atomic report-once. **Bit-exact against the serial path** on the golden corpus | `RenderEngine::startTracingParallel`, `RenderTask.h`, `RasterTileGenerator`, `renderParallel.sh` | **#4 fulfilled**: the concurrent multi-thread driver matches the serial reference image-for-image |

## Areas to improve

| # | Aspect | Evidence / risk | Objective | Suggestion |
|---|--------|-----------------|-----------|------------|
| M1 | **Virtual dispatch across geometry and materials** (`virtual allIntersectionsForOwner` + `PriorityQueue<Intersection>`; plus the `SolidTexturePigment`/`SolidTextureNormal` virtuals) | `Geometry.h`, primitive headers, `pigment/*`, `normal/*` | **#5** | GPU/SPIR-V has no virtuals or recursion; it will need tagged-unions/SoA. The pigment/normal hierarchies aid CPU/Java clarity (P14) but are three families to flatten — plan a flat POD scene + pattern-id dispatch early |
| M2 | **`double` throughout the pipeline** | `Vector3Dd`, `Matrix4x4d`, shaders | **#5** | FP64 is slow/scarce on the GPU. Since the GPU goal is AE-metric similarity (not bit-exact), decide the precision strategy and the acceptable AE tolerance early |
| M3 | **No unit-test scaffolding** (only whole-image golden tests) | no unit-test directory | #1, #4, #5 | The golden test does not localize per-module regressions; missing `Vector3Dd`/`Matrix4x4d`/solver tests that would also pin the contract for the ports |
| M4 | **Antialiasing still reads the adjacent scanline within a band** (optional refinement) | `AdaptiveAntiAliasing::doAntiAliasing`, `RenderWorker::swapLines` | #4 | Already safe across tiles (1-row halo + a guard that forbids supersampling a neighbour-owned row), so it does **not** block `-parallel`. A full two-pass AA over the frame buffer — plus rectangular tiles for better load balance — would remove even the remaining intra-band coupling |

## Reading by objective

- **#1 Academic base** — Well on track (P1–P4, P7, P13, P14, P15, P16); the sampling
  driver is separated from image writing, procedural texturing is now a readable
  one-class-per-pattern hierarchy instead of a switch/facade, and ownership is now
  explicit and fully traced (P16) rather than implicit. The main item left reducing
  clarity: M3 (no unit tests documenting contracts).
- **#2 C++11 migratable** — Solid (P6, P10, P16). `unique_ptr` was deliberately not the
  fix for memory ownership — real destructors plus deep-cloning the pigment/normal
  `copy()` hierarchy, staying on raw `new`/`delete` per the backwards/forwards
  portability goals (P16).
- **#3 JDK alignment** — Ahead thanks to P7, P14, P15 and P16: the pigment/normal class
  hierarchies and the immutable, builder-built `PovRayMaterial` map almost mechanically
  to Java, and every class has a real, traceable destructor with verified ownership
  (P16), the closest C++ analogue to relying on GC-managed lifetimes.
- **#4 pthreads parallelism** — Done (P17). `-parallel[N]` renders the frame as
  independent horizontal bands on the pthread-backed `java::Executors` pool, one
  `RenderTask` per thread carrying all mutable state (worker, intersection pool,
  `Statistics`, `TextureUtils`); tile bounds are explicit parameters, the frame is
  assembled in a full-frame buffer and persisted after join, AA seams use a one-row
  halo, abort is atomic, and per-thread `Statistics`/`SolidTextureStatistics` are
  reduced via their parts-summing constructors. The parallel path is bit-exact
  against the serial one on the golden corpus (`renderParallel.sh`). Foundations that
  made it cheap: `trace` worker-parameterized (P12), read-mostly `RenderContext`, the
  pre-existing pthread `Executors` wrapper, and the global-free base (P8/P9). The one
  remaining refinement is M4 (intra-band AA scanline coupling / rectangular tiles).
- **#5 Vulkan 1.3** — The goal here is an AE-metric *close-to-similarity* match with
  the CPU reference, not a bit-exact one — that relaxation is what makes the GPU port
  tractable. The architecture helps (P2, P3, P5 as oracle, and P14: each pigment/normal
  is an isolated shading variant). But M1 (virtuals/AoS spanning geometry and materials)
  and M2 (double precision) remain deep decisions to resolve **before** writing SPIR-V.
  P11 resolves CPU call recursion and provides the execution model to port; the next
  prototype should flatten the scene into POD data, then measure GPU output against
  the CPU reference with the existing AE tooling under an agreed tolerance.
