# POV-Ray 1.0 — Modern C++ Rewrite

A faithful rewrite of [POV-Ray 1.0 (1992)](https://www.povray.org/) in modern C++.
The renderer is **100% compatible** with the original POV-Ray 1 scene description language and
produces **pixel-identical output** to the reference implementation across the full original
scene test suite (108 scenes).

| `iortest.pov` — refraction / IOR | `car.pov` — sample scene |
|:---:|:---:|
| ![iortest](doc/iortest.png) | ![car](doc/car.png) |

## What it is

POV-Ray 1 is a classic recursive ray tracer from 1992. This project reimplements the same
rendering algorithm and scene parser from scratch in idiomatic modern C++, replacing the
original monolithic C translation unit with a layered, object-oriented architecture while
preserving every visible behaviour: same primitives, same solid-texture pipeline, same
lighting model, same numerical output.

**Compatibility guarantees:**
- Scene files written for POV-Ray 1 render without modification. Both `color` and `colour`
  spellings are accepted everywhere in `.pov` files, as in the original.
- Output images are bit-for-bit identical to those produced by the reference build, verified
  with ImageMagick's `AE` (absolute error) metric against a golden image corpus.

## Features

**Geometry primitives**
- Sphere, box, infinite plane
- Quadric surfaces (general second-degree equation)
- Poly / quartic surfaces (higher-degree polynomial shapes)
- Blob (implicit metaball-style surfaces with configurable threshold)
- Height field (GIF, POT, TGA elevation maps)
- Triangle and smooth triangle (Phong normal interpolation)
- Bicubic Bézier patch
- CSG: union, intersection, difference, composite

**Solid textures**
- Procedural patterns: checker, marble, wood, granite, agate, bozo, onion, leopard,
  spotted, gradient
- Procedural bumps: ripples, waves, bumps, dents, wrinkles, bumpy1/2/3
- Image-based: `image_map` (planar, spherical, cylindrical, toroidal mapping),
  `bump_map`, `material_map`
- Color maps with linear interpolation

**Lighting & shading**
- Phong and Blinn-Phong specular highlights
- Lambertian diffuse
- Ambient light
- Point lights and spotlights with `falloff`, `tightness`, and `radius`
- Hard shadows
- Mirror reflection (recursive ray tracing)
- Refraction / transmission with index of refraction and Snell's law
- Exponential atmospheric fog

**Image output**
- Targa (TGA) — primary output format
- Raw dump (`.dis`) — scanline-based intermediate format
- IFF (Amiga) and raw RGB

## Architecture

The codebase is organised around five root packages inside `src`: `app`, `common`,
`environment`, `io`, and `render`. Dependencies mostly flow downward from orchestration
(`app`) to parsing / domain modelling (`io`, `environment`) and finally to the rendering
kernel (`render`) plus shared support code (`common` and the external `base` toolkit).

The diagram below was generated directly from the source code using
[dependencyGraphAnalyzer](https://github.com/OscarChavarro/dependencyGraphAnalyzer).

![Architecture diagram](doc/architectureDiagram.png)

<table>
  <thead>
    <tr>
      <th>Meta-layer</th>
      <th>Layer</th>
      <th>Package</th>
      <th>Responsibility</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="background-color:#f28682;color:#000;padding:8px">app</td>
      <td><strong>Application</strong></td>
      <td><code>src/app</code></td>
      <td><code>main.cpp</code> and <code>PovRayApplication</code> coordinate CLI parsing, file lookup, scene loading, renderer setup, and final image output; <code>src/app/options</code> contains the command-line parser</td>
    </tr>
    <tr>
      <td style="background-color:#ffff8f;color:#000;padding:8px">io</td>
      <td><strong>IO — POV-Ray scene input</strong></td>
      <td><code>src/io/pov</code></td>
      <td>Tokenizer, parser context, and specialised recursive-descent parsers for camera, geometry, lights, materials, declarations, and scene-level directives; these parsers build render-time objects directly rather than producing a separate AST</td>
    </tr>
    <tr>
      <td style="background-color:#ffff8f;color:#000;padding:8px">io</td>
      <td><strong>IO — Runtime support and image formats</strong></td>
      <td><code>src/io</code></td>
      <td><code>src/io/binaryIo</code> resolves include / asset paths, <code>src/io/context</code> stores mutable runtime state such as stop flags and max trace depth, and <code>src/io/image</code> implements <code>ImageOutput</code> backends for TGA, GIF, IFF, raw dump, and raw RGB data</td>
    </tr>
    <tr>
      <td style="background-color:#9bfc8c;color:#000;padding:8px">environment</td>
      <td><strong>Environment</strong></td>
      <td><code>src/environment</code></td>
      <td>Render-time scene model: <code>Scene</code>, object wrappers in <code>scene/</code>, geometry and intersection code in <code>geometry/</code>, light sources in <code>light/</code>, and POV-Ray materials, pigments, normals, and renderer configuration in <code>material/</code>. Camera state is stored as Vitral <code>CameraSnapshot</code> data rather than a local camera implementation</td>
    </tr>
    <tr>
      <td style="background-color:#7d7ef8;color:#000;padding:8px">render</td>
      <td><strong>Render</strong></td>
      <td><code>src/render</code></td>
      <td><code>RenderEngine</code> owns the trace loop, tile scheduling, adaptive antialiasing, per-thread workers, and output staging; <code>RenderContext</code> exposes immutable scene/config access plus shared statistics and texture services</td>
    </tr>
    <tr>
      <td style="background-color:#7d7ef8;color:#000;padding:8px">render</td>
      <td><strong>Shaders</strong></td>
      <td><code>src/render/shaders</code></td>
      <td>One class per shading step (ambient, direct light, Lambert, Phong, Blinn-Phong, shadowing, reflection, refraction, fog, bump handling) plus <code>TraceService</code>, the callback interface shaders use to request secondary rays without depending on <code>RenderEngine</code> directly</td>
    </tr>
    <tr>
      <td style="background-color:#99fcfe;color:#000;padding:8px">base</td>
      <td><strong>Common</strong></td>
      <td><code>src/common</code></td>
      <td>Small cross-cutting support code; today this is mostly configuration constants and the statistics subsystem used by both parsing and rendering</td>
    </tr>
    <tr>
      <td style="background-color:#99fcfe;color:#000;padding:8px">base</td>
      <td><strong>Base library</strong></td>
      <td><code>base/src/main/vsdk/toolkit</code></td>
      <td>Vitral toolkit: linear algebra (<code>Vector3Dd</code>, <code>Matrix4x4d</code>), image buffers, I/O primitives</td>
    </tr>
    <tr>
      <td style="background-color:#99fcfe;color:#000;padding:8px">base</td>
      <td><strong>Solid textures</strong></td>
      <td><code>base/src/main/vsdk/toolkit/media/solidTexture</code></td>
      <td>Solid texture evaluation (procedural noise, image-to-solid-texture projection), indexed palette images, image-map sampling</td>
    </tr>
    <tr>
      <td style="background-color:#99fcfe;color:#000;padding:8px">base</td>
      <td><strong>Processing</strong></td>
      <td><code>base/src/main/vsdk/toolkit/numericalAnalysis/polynomial</code></td>
      <td>Sturm-sequence polynomial root solver used by quartics and polys</td>
    </tr>
  </tbody>
</table>

At runtime, `PovRayApplication` parses the command line into
`PovRayRendererConfiguration`, `FileLocator`, and `Scene`, then invokes the POV-Ray
parsers in `src/io/pov` to populate geometry, lights, materials, and camera data. The
renderer then traces the resulting `Scene` either serially or by dividing the image into
tiles. For each hit, `RayShaderPipeline` dispatches the shading stages, while reflection,
refraction, and shadow queries go back through `TraceService` so the shader code stays
decoupled from the top-level iterative trace loop in `RenderEngine`.

## Building

### Prerequisites

| Tool | Minimum version | Notes |
|---|---|---|
| C++ compiler | GCC 11 or Clang 14 | C++11 required |
| CMake | 3.16 | |
| ImageMagick | 6 or 7 | Required for `testAgainstGoldenImages.sh` and `viewImages.sh` |
| clang-tidy + clang-format | any recent | Optional, required for `lint.sh` |
| gcovr or lcov + genhtml | any | Optional, required for coverage reports |

### Linux / macOS

```bash
# 1. Clone
git clone https://github.com/OscarChavarro/povCpp.git
cd povCpp

# 2. Build
./scripts/compile.sh

# The binary is placed at build/povray
```

### Running a scene

```bash
./build/povray \
    +l/path/to/include \
    +i scene.pov \
    +o output.tga \
    +w1280 +h800
```

Common options (inherited from the original POV-Ray 1 command-line interface):

| Option | Meaning |
|---|---|
| `+i<file>` | Input scene file |
| `+o<file>` | Output image file |
| `+l<path>` | Include search path |
| `+w<n>` | Image width in pixels |
| `+h<n>` | Image height in pixels |
| `-d` | Disable display output |
| `-v` | Verbose mode |
| `+ft` | Output format: Targa |
| `+x` | Enable early exit on error |

## Scripts

All scripts live in `scripts/` and are run from the repository root.

### `clean.sh`
Removes the `build/` and `output/` directories, generated `.tga` files, and editor
temporary files. Run this before a fresh build.

```bash
./scripts/clean.sh
```

### `compile.sh`
Configures and builds with CMake using all available CPU cores. Accepts any extra CMake
flags as arguments (forwarded verbatim to `cmake -S . -B build`). After a successful build,
copies `compile_commands.json` to the root for IDE and linter use.

```bash
./scripts/compile.sh
# With extra flags:
./scripts/compile.sh -DCMAKE_BUILD_TYPE=Debug
```

### `renderAll.sh`
Renders the full test suite of 108 `.pov` scenes drawn from the original POV-Ray 1
distribution, organised in four groups:

| Directory | Description |
|---|---|
| `etc/level1/` | Basic primitives, simple lighting, introductory textures |
| `etc/level2/` | CSG, height fields, spotlights, atmospheric effects |
| `etc/level3/` | Complex multi-object scenes (car, chess set, teapot, ionic column…) |
| `etc/math/` | Mathematical surfaces — quartics, Bézier patches, parametric shapes |

All scenes run in parallel and output TGA files to `output/`.

```bash
./scripts/renderAll.sh
```

### `testAgainstGoldenImages.sh`
Compares every generated `.tga` in `output/` against the reference corpus in
`../referenceTestImages/` using ImageMagick's `AE` (absolute pixel error) metric.
Exits with a non-zero status if any image differs or is missing.

```bash
./scripts/testAgainstGoldenImages.sh
# Expected output: Test passed.
```

### `lint.sh`
Runs `clang-tidy` and `clang-format` against source files matched by a path or regex.
Two modes:

```bash
# Static analysis only (does not modify files):
./scripts/lint.sh --check src/render/.*

# Apply automatic fixes and reformat:
./scripts/lint.sh --fix src/render/.*
```

Override the check set or format style via environment variables:
```bash
LINT_CHECKS='modernize-*' ./scripts/lint.sh --fix src/.*
```

### `viewImages.sh`
Builds a side-by-side comparison (reference | generated | diff) for every scene and
displays the result in the terminal using [Sixel](https://en.wikipedia.org/wiki/Sixel)
graphics. Requires a Sixel-capable terminal (e.g. iTerm2, mlterm, xterm -ti vt340).

```bash
./scripts/viewImages.sh
```

### `renderAllWithCoverage.sh`
Recompiles the project with `gcov` instrumentation, runs the full render suite, and
generates an HTML coverage report under `build/coverage/index.html`. Requires `gcovr`
(preferred) or `lcov` + `genhtml`.

```bash
./scripts/renderAllWithCoverage.sh
open build/coverage/index.html
```

### `parseCoverage.sh`
Parses each `.pov` scene file through the tokenizer/parser only (no rendering) and
reports which scenes parse successfully. Useful for quickly iterating on parser changes.

```bash
./scripts/parseCoverage.sh
```

## Full verification workflow

```bash
./scripts/clean.sh
./scripts/compile.sh
./scripts/renderAll.sh
./scripts/testAgainstGoldenImages.sh
```

A passing run prints `Test passed.` with zero failures across all 108 scenes.

## Project objectives

The [design objectives assessment](doc/designObjectives.md) tracks the current strengths,
risks, and planned architectural improvements against these goals.

- **Education.** The primary goal is to serve as a learning resource for computer graphics
  courses. The codebase is intentionally structured so that each algorithm — BVH traversal,
  CSG evaluation, texture projection, the Phong shading model, Snell's law refraction — can
  be studied in isolation. A curated set of reference getMaterials is included in `doc/references/`,
  and specific sections of the source code are annotated with citations that link directly to
  the relevant pages of those texts.

- **Vulkan rendering variant.** A longer-term objective is to port the rendering pipeline to
  Vulkan, using this codebase as the functional specification. The clean separation between
  scene representation and shading makes it a suitable starting point for a GPU-accelerated
  implementation.

- **Case study for the Vitral library.** This project also acts as a real-world integration
  test for [Vitral](https://github.com/OscarChavarro/vitral), a companion C++ toolkit that
  provides the linear algebra, image buffer, and I/O primitives used here. Both projects share
  the same design principles and evolve together.

- **Fun.** POV-Ray is one of the most entertaining programs ever written. Watching a
  mathematically correct image of a glass teapot or a Steiner surface materialise one
  scanline at a time never gets old.

- **Tribute.** This project is dedicated to the original POV-Ray Team — David Buck,
  Aaron Collins, and all contributors who, in 1992, shipped a free, portable, cross-platform
  ray tracer of remarkable quality and made it available to the world. Their work has inspired
  countless developers and remains a landmark in the history of computer graphics. The
  [original POV-Ray 1 author list and contribution notes](doc/references/originalPovray1Authors.md)
  preserve the credits recovered from the original source comments.
