# Parallel Ray Tracer Implementation Plan (`-parallel`)

This document is the **step-by-step work plan** for adding a `-parallel` option that
renders a frame concurrently using **tiles** (horizontal bands) across `nproc`
threads, on top of the pthread-backed `java::util::concurrent` wrapper.

It is the actionable companion to the readiness assessment in
[`designObjectives.md` → "Tile-based parallelism"](designObjectives.md) (blockers
B1–B9). Read that section first for the *why*; this document is the *how*.

## Ground rules (apply to every task)

- **C++11 only.** No newer standard. No `friend`, no operator overloading, no
  features that do not map cleanly to a future Java port (see the project
  portability note). Prefer the `java::*` wrappers (`ArrayList`, `Executors`,
  `Future`, `Callable`) over raw STL/pthreads in `src/`.
- **The serial path must stay bit-identical.** When `-parallel` is *off*, the code
  must run exactly as today. Every change is additive or guarded by the new flag
  until the very last integration task.
- **The gate (definition of done).** A change is only complete when, from the repo
  root, the full pipeline prints **`Test passed.`**:
  ```
  ./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh
  ```
  Run this exact sequence — not a partial build or a single script — after every
  phase. Without antialiasing the parallel output must be **bit-exact** vs. the
  serial reference (the golden-image AE oracle); the antialiasing case is decided in
  Phase 4 (T8).
- **Touch `base/` only where explicitly noted.** Two base items already exist and
  are part of the design: `java::Executors::newFixedThreadPool` and
  `Statistics(java::ArrayList<Statistics*> *partsPerThread)` (the reduction
  constructor that sums all per-thread counters, including
  `solidTextureStatistics.callsToNoise/callsToDNoise`).

## Difficulty / impact legend

Each task is tagged so work can be handed to the right model:

- 🟢 **Low impact, mechanical** — self-contained, exact edits, no design judgement.
  Suitable for Haiku. Every step is spelled out literally.
- 🟡 **Medium** — local refactor with a clear contract, a few files.
- 🔴 **High** — structural change or a design decision (AA seams, the driver).

## Phase ordering at a glance

```
Phase 1 (independent, start now):   T1🟢  T2🟢  T3🟢
Phase 2 (structural refactor):      T4🟡  T5🟡  T6🟡
Phase 3 (per-thread stats):         T7🟡
Phase 4 (antialiasing seams):       T8🔴
Phase 5 (parallel driver + wire):   T9🔴
Phase 6 (validation):               T10🟡
```

Critical path: **T4 → T5 → T6 → (T7, T8) → T9 → T10**. T1, T2, T3 are independent,
low-risk, and unblock the option/threading plumbing, so do them first.

---

## Phase 1 — Independent low-impact groundwork

These three tasks touch nothing in the hot render path and can land in any order.
They are fully specified so a low-impact agent can execute them.

### T1 🟢 — Add `Runtime.getRuntime().availableProcessors()`

**Goal:** mirror `java.lang.Runtime`'s singleton accessor and instance method, so
the C++ call site reads exactly like the JDK one. `availableProcessors()` is a
real JDK `Runtime` instance method — it must live on a `Runtime` class, not as a
free function, to keep 1:1 correspondence with the JDK.

**Files:**
- New: `base/src/main/java/lang/Runtime.h`
- New: `base/src/main/java/lang/Runtime.cpp`
- Edit: the `base` `CMakeLists.txt` source list (add the new `.cpp`).

**Steps:**
1. Create `Runtime.h` with this exact content:
   ```cpp
   #ifndef JAVA_LANG_RUNTIME__
   #define JAVA_LANG_RUNTIME__
   namespace java {

   class Runtime {
     public:
       static Runtime &getRuntime();
       int availableProcessors();

     private:
       Runtime() = default;
   };

   }
   #endif
   ```
2. Create `Runtime.cpp`:
   ```cpp
   #include "java/lang/Runtime.h"
   #include <unistd.h>
   namespace java {

   Runtime &
   Runtime::getRuntime() {
       static Runtime instance;
       return instance;
   }

   int
   Runtime::availableProcessors() {
       long n = sysconf(_SC_NPROCESSORS_ONLN);
       if (n < 1) {
           return 1;
       }
       return (int)n;
   }

   }
   ```
3. Add `Runtime.cpp` to the base library's source list in its
   `CMakeLists.txt` (find the existing `java/lang/*.cpp` entries and add it next to
   them; if sources are globbed, no edit is needed).

**Acceptance:** the project builds; `java::Runtime::getRuntime().availableProcessors()`
is callable and returns a value ≥ 1. (No behavior change yet — nothing calls it.)

### T2 🟢 — Add a `PARALLEL` flag and thread count to `RenderingConfiguration`

**Goal:** carry the user's choice of parallel mode and thread count in the config,
defaulting to serial.

**Files:**
- Edit: `src/environment/material/RendererConfiguration.h`
- Edit: `src/environment/material/RendererConfiguration.cpp`

**Steps:**
1. In `RendererConfiguration.h`, find the block of `static constexpr` option-flag
   bits (e.g. `ANTIALIAS`, `DISPLAY`, `DISK_WRITE`, `VERBOSE`…). Add a new bit using
   the next unused power-of-two value. **Verify the value is not already used** by
   grepping the existing flags. Example (adjust the literal if `0x...` is taken):
   ```cpp
   static constexpr int PARALLEL = /* next free bit, e.g. */ 0x00040000;
   ```
2. Add a private member and accessors for the thread count. Near the other
   `int` members add:
   ```cpp
   int numberOfThreads;   // 0 = auto (use availableProcessors())
   ```
   and in the public accessor area:
   ```cpp
   int getNumberOfThreads() const { return numberOfThreads; }
   void setNumberOfThreads(int n) { numberOfThreads = n; }
   ```
3. In `RendererConfiguration.cpp`, in `reset()`, initialize the new member:
   ```cpp
   numberOfThreads = 0;
   ```
   (The `PARALLEL` flag bit is cleared by the existing flag reset; confirm `reset()`
   zeroes the option-flags word.)

**Acceptance:** builds; `getNumberOfThreads()` returns 0 after `reset()`;
`hasOptionFlags(RenderingConfiguration::PARALLEL)` is `false` by default.

### T3 🟢 — Parse `-parallel[=N]` in the command line

**Goal:** `-parallel` enables the flag and auto-detects threads; `-parallel=N` sets
an explicit count.

**Files:**
- Edit: `src/app/options/CommandLineOptions.cpp`
- Possibly: `src/app/options/CommandLineOptions.h` (only if a usage string lives
  there).

**Steps:**
1. Open `CommandLineOptions.cpp` and locate the argument-parsing switch/if-chain
   that handles existing flags (search for where `ANTIALIAS` or `setOptionFlags` is
   set). Mirror that style.
2. Add a branch that matches the token `-parallel`. Pseudostructure (match the
   surrounding parsing idiom exactly — string compare helper, `argv` cursor, etc.):
   ```cpp
   // -parallel            -> auto threads (availableProcessors)
   // -parallel=N          -> N threads
   if (/* token starts with "-parallel" */) {
       configuration.setOptionFlags(RenderingConfiguration::PARALLEL);
       const char *eq = /* find '=' in the token, or nullptr */;
       if (eq != nullptr) {
           int n = atoi(eq + 1);
           if (n < 1) { n = 1; }
           configuration.setNumberOfThreads(n);
       } else {
           configuration.setNumberOfThreads(0); // 0 = auto
       }
       continue; // or break out of this arg, per local style
   }
   ```
   Use the same flag-setting method the file already uses (`setOptionFlags` vs a
   bitwise-or helper) — do not invent a new one.
3. If there is a `usage()` text listing options, add a line:
   `  -parallel[=N]   Render using N threads (default: number of CPUs)`.

**Acceptance:** `./povray -parallel scene.pov` sets the `PARALLEL` flag and leaves
thread count at 0; `-parallel=4` sets it to 4. Nothing reads these yet, so
rendering output is unchanged.

---

## Phase 2 — Structural refactor: one render context per thread

This is the core of the work (blockers B1, B2, B3). The goal is to make the
per-thread mutable state (ray scratch, intersection pool, line buffers, stats sink)
into an object that can be instantiated `nproc` times, while `RenderEngine` becomes
the **shared, read-only** sampling logic.

### T4 🟡 — Introduce a `RenderTask` per-thread context

**Goal:** group everything a single thread mutates into one object: a
`RenderWorker`, an `IntersectionPriorityQueuePool`, and a per-thread `Statistics`.

**Files:**
- New: `src/render/RenderTask.h` (+ `.cpp` if needed)
- Edit: `src/render/RenderEngine.h` / `.cpp`
- Edit: `src/render/RenderWorker.*`

**Steps:**
1. Today `RenderEngine` owns the single `RenderWorker worker`,
   `IntersectionPriorityQueuePool intersectionQueuePool`, and `AdaptiveAntiAliasing`
   (`RenderEngine.h:16-18`). Create a `RenderTask` that owns:
   - `RenderWorker worker;`
   - `IntersectionPriorityQueuePool intersectionQueuePool;`
   - `Statistics statistics;`  (this thread's private counters — B5/B6)
   - the tile bounds it must render (`int firstLine, lastLine;`) — B3
2. Give `RenderTask` a reference to the shared `RenderEngine` (read-only sampling
   logic) and the shared `RenderContext`.
3. `RenderWorker` already isolates per-ray scratch (P12); keep its constructor
   taking the owning engine, but ensure its pool reference comes from the
   `RenderTask`, not from an engine member (next task wires this).

**Acceptance:** builds; `RenderEngine` still has its single members (we remove them
in T9's integration). No behavior change yet — `RenderTask` is not on the hot path.

### T5 🟡 — Pass the intersection pool through the call chain (kill B2)

**Goal:** `createRay`/`trace` must use the **task's** pool, not the engine member.

**Files:** `src/render/RenderEngine.cpp` (`createRay`, `trace`), `RenderEngine.h`.

**Steps:**
1. In `RenderEngine::createRay` (`RenderEngine.cpp:24-56`), line 53 currently does:
   ```cpp
   localRay->setIntersectionQueuePool(&intersectionQueuePool);
   ```
   Change `createRay` to receive the pool (or the owning `RenderWorker`/`RenderTask`)
   as a parameter and use that instead of the engine member.
2. Update both callers:
   - `RenderEngine::startTracing` (`RenderEngine.cpp:100`)
   - `AdaptiveAntiAliasing::superSample` (`AdaptiveAntiAliasing.cpp:54`)
   They already hold a `RenderWorker&`; route the worker's task pool through.
3. `trace(RenderWorker&, …)` already takes the worker (P12); confirm it no longer
   reads any engine-owned mutable state except read-only `scene`/`config`/`context`.

**Acceptance:** builds; serial render output is **bit-exact** vs. before
(`testAgainstGoldenImages.sh` green). The pool is now plumbed, not shared by hidden
reference.

### T6 🟡 — Move tile bounds out of the shared config (kill B3)

**Goal:** a thread renders a sub-window without mutating shared
`RenderingConfiguration`.

**Files:** `src/render/RenderEngine.cpp` (`startTracing`), `RenderEngine.h`.

**Steps:**
1. `startTracing` (`RenderEngine.cpp:65-125`) reads `getConfig().getFirstLine()` and
   `getLastLine()` for the loop bounds. Extract the loop body into a new method:
   ```cpp
   void renderBand(RenderTask &task, int firstLine, int lastLine);
   ```
   that takes the bounds **as parameters** and uses `task`'s worker/pool/stats.
2. Keep `startTracing()` as a thin serial wrapper that calls
   `renderBand(serialTask, config.getFirstLine(), config.getLastLine())` so the
   serial path is preserved.
3. Do **not** call `config.setFirstLine()` from inside a band. The continue-trace
   resume logic (`RenderImageWriter::readRenderedPart`) stays on the serial path
   only for now (document that `-parallel` + `CONTINUE_TRACE` is unsupported in this
   first cut; reject the combination in T9).

**Acceptance:** builds; serial output bit-exact. `renderBand` can be called with
arbitrary `[firstLine, lastLine)` and produces the same pixels as the serial loop
for that range.

---

## Phase 3 — Per-thread statistics (kill B5, B6)

### T7 🟡 — Wire per-thread `Statistics` and reduce at join

**Goal:** each `RenderTask` accumulates into its own `Statistics`; the master sums
them with the existing reduction constructor.

**Files:** `src/render/RenderEngine.*`, `src/app/PovRayApplication.cpp`,
and the noise-stats wiring.

**Steps:**
1. The reduction is **already built**:
   `Statistics(java::ArrayList<Statistics*> *partsPerThread)` sums every counter,
   including `solidTextureStatistics.callsToNoise/callsToDNoise`
   (`Statistics.cpp:6-56`). Use it — do not write a new reducer.
2. Make `RenderEngine::trace`/`startTracing` increment **`task.statistics`** instead
   of `context->getStatistics()`. The `RenderContext`'s `Statistics&` becomes the
   *master* sink, written only by the serial path / final reduction.
3. **Noise counters (B6):** `ProceduralNoise` holds a
   `SolidTextureStatistics * const` set at construction, and `TextureUtils` is shared
   via `RenderContext`. Two options — pick per appetite:
   - **(Recommended, exact):** give each `RenderTask` its own `TextureUtils`
     initialized with that task's `statistics.getSolidTextureStatistics()`. The
     large noise *tables* are read-only — share them; only the stats pointer differs.
     This requires a `TextureUtils` that can share tables but take a per-thread stats
     sink (small `base/` change — note it explicitly in the commit).
   - **(Pragmatic first cut):** keep one shared `TextureUtils`; accept that
     `callsToNoise/DNoise` are approximate under threads (benign undercount). Add a
     `// TODO(B6)` and exclude those two counters from the bit-exact assertion.
4. After the threads join (T9), build the master:
   ```cpp
   java::ArrayList<Statistics*> parts;
   for (each task) parts.add(&task.statistics);
   Statistics total(&parts);
   // copy `total` into the master statistics used by printStatistics
   ```

**Acceptance:** serial path unchanged. In a 1-thread parallel run, the reduced
`Statistics` equals the serial `Statistics` (excluding the two noise counters if the
pragmatic option was chosen).

---

## Phase 4 — Antialiasing across tile seams (kill B4)

### T8 🔴 — Make AA independent across horizontal bands

**Goal:** the adaptive AA (which reads the adjacent scanline via
`previousLine`/`currentLine`, `AdaptiveAntiAliasing.cpp`) must produce correct pixels
at band boundaries, where the neighbor row belongs to another thread.

**Design decision — pick one and record it in `designObjectives.md`:**
- **Option A — 1-row halo:** each band renders one extra scanline of overlap at its
  top edge so `doAntiAliasing` always has its `previousLine`. The halo row is
  computed but not written. Simple; minor recompute on seams. Keeps AA single-pass.
- **Option B — two-pass AA:** Pass 1 fills a full-frame base-sample buffer (trivially
  parallel, no neighbor dependency). Pass 2 runs the adaptive super-sampling over the
  full buffer (also tileable, now all neighbors exist). Cleaner, more memory, larger
  refactor of `AdaptiveAntiAliasing`.

**Recommended:** Option A for the first cut (least change), then revisit B for the
GPU port (it matches the two-pass GPU model).

**Steps (Option A):**
1. Extend `renderBand` so a band `[y0, y1)` with AA enabled also traces row `y0-1`
   into the worker's `previousLine` before entering the main loop (unless `y0` is the
   image top).
2. Ensure the band's AA only **writes** pixels in `[y0, y1)`; the halo row is scratch.
3. Confirm the AA flag buffers (`previous/currentLineAntiAliasedFlags`) are per-worker
   (they are — `RenderWorker`), so no sharing across bands.

**Acceptance:** with AA on, parallel output matches the agreed tolerance vs. the
serial reference. State the tolerance explicitly: target is **bit-exact**; if Option A
leaves tiny seam differences, document the accepted AE delta.

---

## Phase 5 — Parallel driver and final wiring

### T9 🔴 — Tile partition + thread pool + join

**Goal:** when `PARALLEL` is set, split the frame into bands, submit one `Callable`
per band to a `newFixedThreadPool`, and join.

**Files:** `src/render/RenderEngine.*`, `src/app/PovRayApplication.cpp`.

**Steps:**
1. Decide the thread count: `int n = config.getNumberOfThreads(); if (n == 0) n =
   java::Runtime::getRuntime().availableProcessors();` (T1).
2. Partition `[config.getFirstLine(), config.getLastLine())` into `n` contiguous
   bands (`height/n` rows each, remainder distributed to the first few bands).
3. Create `n` `RenderTask` objects (T4), each with its own worker/pool/stats and its
   band bounds.
4. For each task, create a `java::Callable<...>` whose `call()` runs
   `engine.renderBand(task, task.firstLine, task.lastLine)`. Submit all to
   `java::Executors::newFixedThreadPool(n)`; collect the `Future`s.
5. `get()` every `Future` to join (propagates exceptions per the wrapper). Then
   `shutdownNow()` the pool.
6. **Output (B7):** for the first cut, render each band into a **full-frame
   in-memory color buffer** (allocate `width * height` once, bands write disjoint
   row ranges → no locking). After join, persist rows sequentially through the
   existing `RenderImageWriter` ordering. This replaces the per-scanline streaming on
   the parallel path only; the serial path keeps streaming as today.
7. Reduce statistics (T7) and the abort flag.
8. **Abort flag (B8):** make the shared stop / `fatalErrorFound` signalling atomic
   using `java::util::concurrent::atomic`. Report-once stays guarded.
9. Reject unsupported combinations early with a clear message: `-parallel` together
   with `CONTINUE_TRACE` (and, if Option A halo isn't done yet, with `ANTIALIAS`).
10. Gate the whole thing: `if (config.hasOptionFlags(PARALLEL)) parallelTrace();
    else startTracing();` in `PovRayApplication::runRenderLoop`.

**Acceptance:** `-parallel` renders correctly and faster on a multi-core box; without
AA the image is **bit-exact** vs. serial. `-parallel=1` equals serial exactly.

---

## Phase 6 — Validation

### T10 🟡 — Golden-image and scaling validation

**Files:** none new; the gate is the full pipeline.

**The gate must print `Test passed.`:**
```
./scripts/clean.sh;./scripts/compile.sh;./scripts/renderAll.sh;./scripts/testAgainstGoldenImages.sh
```

**Steps:**
1. Run the full gate **serial** → baseline (must already print `Test passed.`).
2. Run it with `-parallel` (auto) → compare. Non-AA scenes: AE must be 0
   (bit-exact). AA scenes: AE within the tolerance recorded in T8.
3. Run with `-parallel=1`, `=2`, `=nproc` → confirm identical images across thread
   counts (determinism: tiling must not change pixels).
4. Spot-check the printed `Statistics`: reduced totals must match the serial run
   (modulo the B6 noise-counter caveat if the pragmatic path was taken).
5. Record wall-clock speedup vs. thread count in the PR description.

**Acceptance:** suite green serial and parallel within tolerance; images identical
across thread counts; near-linear speedup up to `nproc`.

---

## Summary table

| ID | Title | Tag | Blocker(s) | Depends on |
|----|-------|-----|-----------|------------|
| T1 | `Runtime.getRuntime().availableProcessors()` | 🟢 | B9 | — |
| T2 | `PARALLEL` flag + thread count in config | 🟢 | B9 | — |
| T3 | Parse `-parallel[=N]` | 🟢 | B9 | T2 |
| T4 | `RenderTask` per-thread context | 🟡 | B1 | — |
| T5 | Pass intersection pool through call chain | 🟡 | B2 | T4 |
| T6 | Tile bounds out of shared config | 🟡 | B3 | T4, T5 |
| T7 | Per-thread `Statistics` + reduce | 🟡 | B5, B6 | T4 |
| T8 | AA independent across band seams | 🔴 | B4 | T6 |
| T9 | Tile partition + pool + join + output | 🔴 | B1,B7,B8,B9 | T1–T8 |
| T10 | Golden-image + scaling validation | 🟡 | — | T9 |

**Start here:** T1, T2, T3 (independent, mechanical), then the T4→T5→T6 refactor.
