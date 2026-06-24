# Plan: reinterpret povCpp's `+qN` quality as VITRAL-style lazy selection

**Plan** document (actionable, unlike `doc/vitralNormalizationAnalysis.md`,
which is descriptive). It defines how to evolve povCpp's single quality
integer (`+qN`) into an orthogonal set of feature flags plus a per-ray lazy
detail mask, mirroring VITRAL's `RendererConfiguration` + `RayHit::
requiredDetailMask` model — while keeping every existing render byte-identical
and tracking the performance cost cell by cell.

Read §7 of `doc/vitralNormalizationAnalysis.md` first; this plan executes the
convergence opportunity described there.

---

## 1. Objective and current state

povCpp gates rendering features behind comparisons on a single integer
`RenderingConfiguration::quality` (default 9, set by `+qN`). The thresholds,
verbatim from the current pipeline:

| Feature gated | Condition | Where |
|---|---|---|
| Surface lit at all (normal + ambient + diffuse/specular) | `quality > 1` | `LocalSurfaceShader.cpp:36` |
| Shadow-ray object tests (cast shadows) | `quality > 3` | `DirectLightShader.cpp:63` |
| Full pigment/texture eval (else `quickColor`/`objectColor`/grey) | `quality > 5` | `RayShaderPipeline.cpp:72` |
| Transparent/coloured shadows (shadow ray not short-circuited) | `quality > 5` | `RayShaderPipeline.cpp:59` |
| Refraction / transmission | `quality > 5` | `RayShaderPipeline.cpp:124` |
| Bump-mapped normal (surface and refraction paths) | `quality >= 8` / `> 7` | `LocalSurfaceShader.cpp:46`, `RayShaderPipeline.cpp:131` |
| Mirror reflection | `quality >= 8` | `LocalSurfaceShader.cpp:67` |

VITRAL expresses the same "don't compute what the image doesn't need" idea as
two orthogonal mechanisms: `RendererConfiguration` feature booleans
(`isTextureSet`, `isBumpMapSet`, `shadingType`, …) and the per-ray
`RayHit::requiredDetailMask` (`DETAIL_POINT|NORMAL|UV|TANGENT`), consumed by
`doExtraInformation()` and chosen by `SimpleRaytracer::buildSurfaceDetailMask`.

**Goal:** make `+qN` a *named preset* over a flag set that the shaders query
directly, and add a per-ray lazy mask, so that:

1. the quality knob stops being a pile of magic-number comparisons;
2. povCpp's configuration model moves toward VITRAL's feature-flag shape;
3. arbitrary feature subsets become expressible (today impossible: e.g.
   "textures but no shadows", "reflection without bump");
4. per-ray laziness (shadow vs primary) becomes first-class rather than the
   single ad-hoc `shadowRay && quality <= 5` short-circuit.

---

## 2. Does this make sense? (assessment)

**Yes, with a clear boundary.** The reinterpretation is worth doing and does
bring the two codebases closer, but only the *configuration/laziness* layer
should converge — not the renderers themselves.

What genuinely converges:

- **The feature-flag model.** povCpp's `quality` staircase is, provably, a
  single hand-picked path through the very flag space VITRAL already exposes
  (`withShadows`, `withTexture`, `withBump`, `shadingType`, …). Decomposing it
  into those flags makes povCpp's `RenderingConfiguration` structurally
  resemble VITRAL's `RendererConfiguration` for the first time (today they
  share only a name; see §5 of the analysis doc). This is real convergence.
- **The lazy detail mask.** Adding a per-ray `requiredDetailMask` to
  `RayWithSegments`/`PovRayHit` is a direct port of a VITRAL concept povCpp
  currently lacks (it always computes the winner's normal). It pays for
  itself: shadow rays and `q<=1` previews stop computing normals/UV they
  discard.

What must **not** converge (and why this stays "additive, layered"):

- The **engines are different by necessity**: povCpp keeps `allIntersections`
  + the CSG depth queue + the containing-media refraction stack, none of which
  VITRAL's nearest-hit `SimpleRaytracer` has. We are aligning the *selection
  vocabulary*, not the traversal.
- The **preset semantics of `+qN` must be preserved bit-for-bit** for existing
  users and goldens. So the flag set is introduced *underneath* `+qN`: each
  quality level maps to a fixed flag combination that reproduces today's
  output exactly (guarded by `scripts/testQualities.sh`). The flags are the
  new primitive; `+qN` becomes sugar.

Net: this is one of the higher-value convergence steps available, because it
simultaneously (a) removes magic numbers, (b) unlocks the efficiency lever
povCpp already half-uses (fast preview) and makes it measurable, and (c) ports
a concrete VITRAL mechanism (the detail mask) rather than merely renaming.

The risk it does **not** pay off: if every realistic preset ends up being one
of the existing five bands anyway, the orthogonality buys expressiveness few
scenes use. The performance baseline (§4) is what tells us whether finer
control actually saves time on real scenes, so we collect it *before*
committing to the refactor.

---

## 3. Target model

### 3.1. Feature flags on `RenderingConfiguration`

Introduce predicates the shaders query instead of `getQuality() <> N`:

- `withSurfaceLighting()`  — normal + ambient + diffuse/specular (today `q>1`)
- `withShadows()`          — cast-shadow ray tests (today `q>3`)
- `withTextures()`         — full pigment vs `quickColor` (today `q>5`)
- `withFilteredShadows()`  — coloured/transparent shadows (today `q>5`)
- `withRefraction()`       — transmission/refraction (today `q>5`)
- `withBumpMapping()`      — bump normal perturbation (today `q>=8`/`>7`)
- `withReflection()`       — mirror reflection (today `q>=8`)
- `shadingType`            — eventually align with VITRAL's
  `NOLIGHT/FLAT/GOURAUD/PHONG/COOK_TERRANCE` enum (povCpp uses Phong-style
  direct lighting; this is a later step, not blocking).

`setQuality(N)` becomes a *preset setter* that fills these flags to reproduce
band `N` exactly (see the mapping below). Direct flag setters allow subsets
`+qN` cannot express.

| Band | withSurfaceLighting | withShadows | withTextures | withFilteredShadows | withRefraction | withBumpMapping | withReflection |
|------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| q0–q1 | – | – | – | – | – | – | – |
| q2–q3 | ✓ | – | – | – | – | – | – |
| q4–q5 | ✓ | ✓ | – | – | – | – | – |
| q6–q7 | ✓ | ✓ | ✓ | ✓ | ✓ | – | – |
| q8–q9 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

### 3.2. Per-ray lazy detail mask

Add a `requiredDetailMask` (mirroring VITRAL's `DETAIL_POINT|NORMAL|UV|
TANGENT`) to the hit path, set per ray from the flags above:

- shadow rays → `DETAIL_NONE` (only `hitDistance`);
- `q<=1` preview primary rays → no normal (the surface is flat-shaded anyway);
- textured primary rays → normal (+ UV once povCpp computes UV on the hit).

`TransformableElement::doExtraInformation` already exists as the choke point
(§5 of the analysis doc); it gains a mask check before calling `normal()`,
exactly as VITRAL's `doExtraInformation` honours `needsNormal()`.

---

## 4. Gates

### 4.1. Correctness gate (new): `scripts/testQualities.sh`

`scripts/renderQualities.sh` renders `etc/level2/iortest.pov` at every quality
0..9 (320×200) into `output/qualities/`; `scripts/testQualities.sh` compares
them against the committed baseline in `../referenceTestImagesQualities/`
(established with this plan). `iortest.pov` spans every gated feature, so its
ten images are a tight witness that the preset mapping in §3.1 stays
bit-identical through the refactor.

Run sequence for the new gate:

```
./scripts/clean.sh; ./scripts/compile.sh
./scripts/renderQualities.sh; ./scripts/testQualities.sh        # => "Test passed."
```

The existing full gate must also stay green:

```
./scripts/renderAll.sh; ./scripts/testAgainstGoldenImages.sh    # => "Test passed."
```

> Note: the full golden gate renders at `+q9` (default), so it only guards the
> top band. `testQualities.sh` is what guards the *other* bands; both are
> required for this work.

### 4.2. Performance baseline (Step 0): the 108×10 matrix

`scripts/renderQualityMatrix.sh` renders all 108 gate scenes at q0..q9 and
records wall-clock seconds per cell into `output/qualityMatrix.tsv`
(320×200 by default; `WIDTH=1280 HEIGHT=800` for full-res). `output/` is
gitignored, so the **table pasted below is the canonical, committed baseline**
(the TSV is just the working artifact). Every later step re-runs the matrix
and is judged by its delta against these numbers, cell by cell.

What the baseline must show to justify the refactor: a clear *monotone
decrease* in time as quality drops (the fast-preview lever), and where the big
steps are (expected at the band boundaries q1→q2, q3→q4, q5→q6, q7→q8). The
reinterpreted model should reproduce that curve within noise at the preset
levels, and additionally let a *custom* flag subset land *between* two bands in
both image and time — that interpolation is the new capability we are buying.

<!-- QUALITY_MATRIX_BASELINE -->
Baseline collected at 320×200 (108 scenes × 10 qualities = 1080 renders,
single-threaded, ~253 s total). Seconds per render; lower `+qN` is the fast
preview. The two summary rows give the per-quality geometric mean over the 108
scenes and the column totals — note the monotone steps at the band boundaries
q1→q2 (lighting), q3→q4 (shadows, ~×1.7), q5→q6 (textures + refraction), and
q7→q8 (bump + reflection); the full model (q9) costs ~3.4× the q0 preview.

| # | scene | lvl | q0 | q1 | q2 | q3 | q4 | q5 | q6 | q7 | q8 | q9 |
|--:|-------|-----|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | bumpmap | level1 | 0.042 | 0.027 | 0.038 | 0.035 | 0.055 | 0.053 | 0.057 | 0.055 | 0.103 | 0.103 |
| 2 | alphafun | level1 | 0.034 | 0.035 | 0.056 | 0.042 | 0.066 | 0.051 | 0.153 | 0.164 | 0.183 | 0.171 |
| 3 | ballbox1 | level1 | 0.052 | 0.040 | 0.077 | 0.074 | 0.096 | 0.096 | 0.308 | 0.309 | 0.500 | 0.515 |
| 4 | basicvue | level1 | 0.040 | 0.029 | 0.044 | 0.052 | 0.049 | 0.061 | 0.058 | 0.062 | 0.069 | 0.070 |
| 5 | blob | level1 | 0.049 | 0.051 | 0.054 | 0.055 | 0.062 | 0.072 | 0.058 | 0.072 | 0.073 | 0.070 |
| 6 | box | level1 | 0.039 | 0.026 | 0.043 | 0.033 | 0.036 | 0.042 | 0.042 | 0.031 | 0.046 | 0.049 |
| 7 | cantelop | level1 | 0.027 | 0.042 | 0.044 | 0.043 | 0.045 | 0.031 | 0.036 | 0.045 | 0.033 | 0.049 |
| 8 | checker2 | level1 | 0.030 | 0.043 | 0.040 | 0.053 | 0.059 | 0.045 | 0.123 | 0.124 | 0.196 | 0.184 |
| 9 | cliptst2 | level1 | 0.046 | 0.031 | 0.049 | 0.036 | 0.046 | 0.041 | 0.066 | 0.064 | 0.088 | 0.088 |
| 10 | colors | level1 | 0.269 | 0.240 | 0.258 | 0.261 | 0.358 | 0.343 | 0.360 | 0.357 | 0.361 | 0.362 |
| 11 | dish | level1 | 0.040 | 0.038 | 0.061 | 0.059 | 0.076 | 0.074 | 0.075 | 0.073 | 0.076 | 0.075 |
| 12 | dodec2 | level1 | 0.040 | 0.037 | 0.054 | 0.053 | 0.049 | 0.059 | 0.061 | 0.064 | 0.068 | 0.053 |
| 13 | fogtst | level1 | 0.043 | 0.045 | 0.040 | 0.038 | 0.048 | 0.044 | 0.062 | 0.062 | 0.075 | 0.075 |
| 14 | glasdish | level1 | 0.051 | 0.039 | 0.063 | 0.065 | 0.074 | 0.075 | 0.194 | 0.198 | 0.770 | 0.773 |
| 15 | glass | level1 | 0.033 | 0.032 | 0.042 | 0.053 | 0.060 | 0.047 | 0.388 | 0.389 | 0.863 | 0.859 |
| 16 | imagetst | level1 | 0.053 | 0.051 | 0.061 | 0.059 | 0.069 | 0.062 | 0.113 | 0.112 | 0.129 | 0.132 |
| 17 | intee1 | level1 | 0.053 | 0.053 | 0.060 | 0.061 | 0.078 | 0.075 | 0.363 | 0.365 | 0.476 | 0.478 |
| 18 | laser | level1 | 0.053 | 0.051 | 0.087 | 0.086 | 0.156 | 0.156 | 0.370 | 0.370 | 0.722 | 0.712 |
| 19 | mapper | level1 | 0.042 | 0.042 | 0.031 | 0.029 | 0.044 | 0.043 | 0.039 | 0.054 | 0.054 | 0.044 |
| 20 | mappr2 | level1 | 0.050 | 0.052 | 0.044 | 0.051 | 0.057 | 0.055 | 0.049 | 0.060 | 0.064 | 0.064 |
| 21 | matmap | level1 | 0.039 | 0.051 | 0.057 | 0.060 | 0.053 | 0.065 | 0.071 | 0.084 | 0.101 | 0.103 |
| 22 | pvinterp | level1 | 0.042 | 0.043 | 0.045 | 0.046 | 0.047 | 0.050 | 0.064 | 0.068 | 0.068 | 0.055 |
| 23 | shapes2 | level1 | 0.073 | 0.059 | 0.084 | 0.083 | 0.141 | 0.144 | 0.129 | 0.141 | 0.138 | 0.145 |
| 24 | shapes | level1 | 0.101 | 0.089 | 0.107 | 0.104 | 0.268 | 0.271 | 0.248 | 0.252 | 0.241 | 0.249 |
| 25 | spotlite | level1 | 0.043 | 0.044 | 0.068 | 0.056 | 0.084 | 0.084 | 0.121 | 0.124 | 0.131 | 0.128 |
| 26 | stone1 | level1 | 0.064 | 0.079 | 0.086 | 0.085 | 0.126 | 0.124 | 0.197 | 0.184 | 0.188 | 0.197 |
| 27 | stone2 | level1 | 0.079 | 0.077 | 0.072 | 0.084 | 0.125 | 0.113 | 0.400 | 0.403 | 0.405 | 0.416 |
| 28 | stone3 | level1 | 0.077 | 0.081 | 0.092 | 0.086 | 0.129 | 0.116 | 0.357 | 0.358 | 0.352 | 0.383 |
| 29 | stone4 | level1 | 0.045 | 0.044 | 0.044 | 0.044 | 0.046 | 0.049 | 0.059 | 0.053 | 0.063 | 0.063 |
| 30 | sunset1 | level1 | 0.043 | 0.029 | 0.053 | 0.053 | 0.066 | 0.063 | 0.054 | 0.051 | 0.087 | 0.084 |
| 31 | sunset | level1 | 0.032 | 0.033 | 0.039 | 0.052 | 0.055 | 0.053 | 0.213 | 0.213 | 0.296 | 0.296 |
| 32 | texture1 | level1 | 0.078 | 0.082 | 0.095 | 0.096 | 0.164 | 0.164 | 0.248 | 0.257 | 0.261 | 0.254 |
| 33 | texture2 | level1 | 0.070 | 0.067 | 0.087 | 0.094 | 0.160 | 0.156 | 0.215 | 0.227 | 0.405 | 0.391 |
| 34 | texture3 | level1 | 0.043 | 0.043 | 0.067 | 0.053 | 0.089 | 0.090 | 0.105 | 0.105 | 0.151 | 0.144 |
| 35 | window | level1 | 0.048 | 0.054 | 0.062 | 0.048 | 0.086 | 0.069 | 0.082 | 0.083 | 0.115 | 0.115 |
| 36 | arches | level2 | 0.051 | 0.049 | 0.075 | 0.074 | 0.097 | 0.096 | 0.415 | 0.416 | 0.828 | 0.833 |
| 37 | cluster | level2 | 0.039 | 0.053 | 0.069 | 0.057 | 0.117 | 0.121 | 0.113 | 0.110 | 0.150 | 0.156 |
| 38 | crystal | level2 | 0.042 | 0.055 | 0.074 | 0.074 | 0.118 | 0.115 | 0.164 | 0.178 | 0.406 | 0.423 |
| 39 | eight | level2 | 0.051 | 0.042 | 0.059 | 0.047 | 0.070 | 0.071 | 0.077 | 0.081 | 0.083 | 0.096 |
| 40 | esp01 | level2 | 0.049 | 0.060 | 0.067 | 0.069 | 0.071 | 0.085 | 0.160 | 0.174 | 0.193 | 0.189 |
| 41 | hfclip | level2 | 0.075 | 0.083 | 0.096 | 0.094 | 0.135 | 0.152 | 0.167 | 0.169 | 0.186 | 0.183 |
| 42 | illum1 | level2 | 0.067 | 0.055 | 0.081 | 0.082 | 0.121 | 0.104 | 0.253 | 0.249 | 1.085 | 1.097 |
| 43 | illum2 | level2 | 0.073 | 0.072 | 0.089 | 0.093 | 0.206 | 0.208 | 0.177 | 0.164 | 0.327 | 0.348 |
| 44 | iortest | level2 | 0.089 | 0.073 | 0.113 | 0.112 | 0.370 | 0.386 | 0.699 | 0.691 | 2.052 | 2.070 |
| 45 | lpops1 | level2 | 0.075 | 0.075 | 0.086 | 0.074 | 0.098 | 0.102 | 0.154 | 0.158 | 0.408 | 0.423 |
| 46 | lpops2 | level2 | 0.083 | 0.084 | 0.083 | 0.085 | 0.115 | 0.115 | 0.184 | 0.179 | 0.504 | 0.506 |
| 47 | magglass | level2 | 0.043 | 0.056 | 0.054 | 0.053 | 0.096 | 0.099 | 0.176 | 0.170 | 0.185 | 0.185 |
| 48 | mtmand | level2 | 0.049 | 0.064 | 0.073 | 0.074 | 0.099 | 0.102 | 0.097 | 0.111 | 0.118 | 0.119 |
| 49 | pacman | level2 | 0.040 | 0.039 | 0.060 | 0.048 | 0.072 | 0.073 | 0.090 | 0.092 | 0.130 | 0.121 |
| 50 | pawns | level2 | 0.248 | 0.254 | 0.274 | 0.262 | 0.726 | 0.726 | 1.544 | 1.553 | 1.968 | 1.948 |
| 51 | planet | level2 | 0.032 | 0.044 | 0.038 | 0.043 | 0.056 | 0.052 | 0.234 | 0.232 | 0.240 | 0.238 |
| 52 | poolball | level2 | 0.061 | 0.059 | 0.070 | 0.075 | 0.099 | 0.100 | 0.100 | 0.104 | 0.121 | 0.132 |
| 53 | romo | level2 | 0.057 | 0.042 | 0.057 | 0.056 | 0.071 | 0.071 | 0.090 | 0.092 | 0.191 | 0.191 |
| 54 | room | level2 | 0.049 | 0.052 | 0.076 | 0.075 | 0.132 | 0.132 | 0.240 | 0.236 | 0.244 | 0.243 |
| 55 | skyvase | level2 | 0.060 | 0.058 | 0.080 | 0.080 | 0.096 | 0.107 | 0.147 | 0.147 | 0.527 | 0.542 |
| 56 | smoke | level2 | 0.040 | 0.042 | 0.042 | 0.053 | 0.057 | 0.052 | 0.187 | 0.186 | 0.198 | 0.199 |
| 57 | spline | level2 | 0.221 | 0.208 | 0.218 | 0.217 | 0.309 | 0.306 | 0.310 | 0.325 | 0.429 | 0.414 |
| 58 | stonewal | level2 | 0.067 | 0.081 | 0.089 | 0.081 | 0.134 | 0.135 | 0.163 | 0.164 | 0.255 | 0.255 |
| 59 | sunsethf | level2 | 0.071 | 0.071 | 0.070 | 0.067 | 0.127 | 0.125 | 0.288 | 0.283 | 0.418 | 0.417 |
| 60 | tetra | level2 | 0.061 | 0.071 | 0.086 | 0.073 | 0.141 | 0.127 | 0.164 | 0.164 | 0.374 | 0.394 |
| 61 | waterbow | level2 | 0.056 | 0.060 | 0.066 | 0.069 | 0.087 | 0.073 | 0.138 | 0.155 | 0.361 | 0.364 |
| 62 | wtorus | level2 | 0.055 | 0.057 | 0.070 | 0.073 | 0.088 | 0.099 | 0.182 | 0.193 | 0.196 | 0.198 |
| 63 | car | level3 | 0.124 | 0.110 | 0.124 | 0.125 | 0.250 | 0.251 | 0.269 | 0.247 | 0.824 | 0.836 |
| 64 | chess | level3 | 0.123 | 0.124 | 0.137 | 0.138 | 0.195 | 0.195 | 0.279 | 0.279 | 0.404 | 0.403 |
| 65 | desk | level3 | 0.122 | 0.135 | 0.153 | 0.140 | 0.353 | 0.366 | 0.335 | 0.350 | 0.509 | 0.527 |
| 66 | dfwood | level3 | 0.046 | 0.058 | 0.066 | 0.069 | 0.083 | 0.083 | 0.245 | 0.227 | 0.239 | 0.247 |
| 67 | drums | level3 | 0.361 | 0.348 | 0.404 | 0.405 | 2.326 | 2.314 | 4.043 | 4.048 | 5.751 | 5.739 |
| 68 | fish13 | level3 | 0.235 | 0.244 | 0.263 | 0.250 | 0.474 | 0.490 | 0.569 | 0.576 | 0.911 | 0.912 |
| 69 | fishbowl | level3 | 0.080 | 0.065 | 0.089 | 0.088 | 0.126 | 0.114 | 0.207 | 0.205 | 0.221 | 0.224 |
| 70 | ionic5 | level3 | 0.446 | 0.445 | 0.451 | 0.462 | 1.073 | 1.055 | 1.699 | 1.698 | 1.710 | 1.704 |
| 71 | kscope | level3 | 0.055 | 0.068 | 0.075 | 0.077 | 0.120 | 0.108 | 0.153 | 0.153 | 0.491 | 0.513 |
| 72 | lamp | level3 | 0.085 | 0.085 | 0.103 | 0.103 | 0.212 | 0.215 | 0.164 | 0.175 | 0.278 | 0.258 |
| 73 | ntreal | level3 | 0.236 | 0.252 | 0.262 | 0.261 | 0.400 | 0.391 | 0.534 | 0.516 | 0.526 | 0.539 |
| 74 | oak2 | level3 | 0.346 | 0.343 | 0.355 | 0.339 | 0.867 | 0.860 | 1.278 | 1.272 | 1.544 | 1.565 |
| 75 | palace | level3 | 0.080 | 0.070 | 0.078 | 0.080 | 0.129 | 0.122 | 0.208 | 0.206 | 0.213 | 0.215 |
| 76 | pencil | level3 | 0.072 | 0.074 | 0.085 | 0.085 | 0.169 | 0.175 | 0.180 | 0.185 | 0.299 | 0.297 |
| 77 | piece1 | level3 | 0.159 | 0.152 | 0.168 | 0.165 | 0.514 | 0.514 | 0.480 | 0.481 | 0.889 | 0.873 |
| 78 | piece2 | level3 | 0.314 | 0.299 | 0.326 | 0.326 | 0.755 | 0.750 | 0.811 | 0.809 | 1.509 | 1.519 |
| 79 | piece3 | level3 | 0.437 | 0.435 | 0.478 | 0.461 | 1.096 | 1.159 | 1.094 | 1.092 | 1.544 | 1.547 |
| 80 | pool | level3 | 0.130 | 0.133 | 0.144 | 0.146 | 0.200 | 0.198 | 0.243 | 0.245 | 0.320 | 0.321 |
| 81 | roman | level3 | 0.136 | 0.136 | 0.155 | 0.152 | 0.228 | 0.227 | 0.330 | 0.331 | 0.514 | 0.516 |
| 82 | snack | level3 | 0.208 | 0.210 | 0.213 | 0.212 | 0.527 | 0.528 | 0.676 | 0.674 | 1.663 | 1.664 |
| 83 | snail | level3 | 0.172 | 0.171 | 0.170 | 0.174 | 0.324 | 0.322 | 0.807 | 0.819 | 0.851 | 0.847 |
| 84 | takeoff | level3 | 0.071 | 0.081 | 0.081 | 0.091 | 0.269 | 0.267 | 0.253 | 0.253 | 0.523 | 0.544 |
| 85 | teapot | level3 | 0.419 | 0.405 | 0.428 | 0.423 | 0.727 | 0.718 | 0.730 | 0.736 | 0.729 | 0.728 |
| 86 | tomb | level3 | 0.350 | 0.358 | 0.372 | 0.372 | 0.537 | 0.547 | 0.820 | 0.829 | 0.856 | 0.847 |
| 87 | wealth | level3 | 0.075 | 0.061 | 0.086 | 0.094 | 0.147 | 0.143 | 0.310 | 0.294 | 0.642 | 0.671 |
| 88 | wg5 | level3 | 0.091 | 0.100 | 0.113 | 0.113 | 0.221 | 0.207 | 0.387 | 0.385 | 1.187 | 1.210 |
| 89 | bezier0 | math | 0.052 | 0.041 | 0.041 | 0.052 | 0.055 | 0.045 | 0.049 | 0.056 | 0.060 | 0.056 |
| 90 | bezier | math | 0.066 | 0.064 | 0.071 | 0.084 | 0.107 | 0.106 | 0.106 | 0.097 | 0.105 | 0.101 |
| 91 | bicube | math | 0.054 | 0.055 | 0.064 | 0.052 | 0.059 | 0.074 | 0.074 | 0.061 | 0.060 | 0.060 |
| 92 | folium | math | 0.045 | 0.053 | 0.056 | 0.055 | 0.067 | 0.066 | 0.087 | 0.102 | 0.103 | 0.091 |
| 93 | grafbic | math | 0.128 | 0.133 | 0.133 | 0.133 | 0.170 | 0.172 | 0.165 | 0.172 | 0.167 | 0.170 |
| 94 | helix | math | 0.102 | 0.091 | 0.107 | 0.109 | 0.188 | 0.190 | 0.229 | 0.232 | 0.232 | 0.236 |
| 95 | hyptorus | math | 0.051 | 0.049 | 0.059 | 0.074 | 0.096 | 0.095 | 0.126 | 0.126 | 0.133 | 0.119 |
| 96 | lemnisc2 | math | 0.088 | 0.099 | 0.107 | 0.107 | 0.167 | 0.156 | 0.179 | 0.194 | 0.198 | 0.190 |
| 97 | lemnisca | math | 0.061 | 0.046 | 0.069 | 0.067 | 0.070 | 0.083 | 0.077 | 0.086 | 0.094 | 0.093 |
| 98 | monkey | math | 0.055 | 0.066 | 0.066 | 0.067 | 0.075 | 0.074 | 0.088 | 0.089 | 0.077 | 0.076 |
| 99 | partorus | math | 0.060 | 0.056 | 0.067 | 0.074 | 0.104 | 0.108 | 0.130 | 0.130 | 0.143 | 0.144 |
| 100 | piriform | math | 0.057 | 0.060 | 0.068 | 0.071 | 0.072 | 0.070 | 0.093 | 0.081 | 0.098 | 0.101 |
| 101 | quarcyl | math | 0.049 | 0.051 | 0.046 | 0.046 | 0.065 | 0.064 | 0.058 | 0.072 | 0.078 | 0.073 |
| 102 | quarpara | math | 0.071 | 0.062 | 0.076 | 0.073 | 0.100 | 0.091 | 0.166 | 0.168 | 0.167 | 0.171 |
| 103 | steiner | math | 0.074 | 0.073 | 0.065 | 0.081 | 0.110 | 0.124 | 0.112 | 0.124 | 0.126 | 0.112 |
| 104 | tcubic | math | 0.064 | 0.063 | 0.065 | 0.064 | 0.076 | 0.063 | 0.076 | 0.091 | 0.091 | 0.090 |
| 105 | teardrop | math | 0.079 | 0.063 | 0.064 | 0.080 | 0.080 | 0.072 | 0.083 | 0.082 | 0.081 | 0.083 |
| 106 | torus | math | 0.052 | 0.054 | 0.060 | 0.050 | 0.087 | 0.088 | 0.091 | 0.092 | 0.088 | 0.083 |
| 107 | trough | math | 0.088 | 0.077 | 0.078 | 0.089 | 0.100 | 0.100 | 0.137 | 0.134 | 0.135 | 0.136 |
| 108 | witch | math | 0.053 | 0.053 | 0.070 | 0.068 | 0.082 | 0.083 | 0.129 | 0.126 | 0.129 | 0.133 |
| | **geomean (108)** | | 0.072 | 0.072 | 0.084 | 0.084 | 0.126 | 0.124 | 0.180 | 0.183 | 0.248 | 0.248 |
| | **total (s)** | | 10.2 | 10.1 | 11.4 | 11.4 | 21.4 | 21.3 | 31.6 | 31.8 | 47.6 | 47.8 |
<!-- /QUALITY_MATRIX_BASELINE -->

---

## 5. Implementation steps

Each step keeps both gates green and re-runs the matrix to record its cost.

1. **Step 0 — baseline.** Run `scripts/renderQualityMatrix.sh` and paste its
   `output/qualityMatrix.tsv` into §4.2 (the md table is the committed copy;
   `output/` is gitignored). Done as part of landing this plan.
2. **Introduce flags, derive from quality.** Add the §3.1 predicates to
   `RenderingConfiguration`, implemented purely in terms of the current
   `quality` int (no shader change yet). No behaviour change; gates green.
3. **Switch shaders to flags.** Replace each `getQuality() <> N` in
   `LocalSurfaceShader`, `DirectLightShader`, `RayShaderPipeline` with the
   matching predicate. Behaviour and goldens unchanged; this is the
   magic-number removal. Re-run matrix → expect no perf change.
4. **Make `setQuality` a preset.** `setQuality(N)` now sets flags per the
   §3.1 table instead of storing a bare int; remove the stored `quality`
   field (or keep it only as a label). Gates green.
5. **Expose direct flag setters / CLI.** Allow `+q` presets *and* explicit
   feature toggles (e.g. a `+f<flags>` style option). Add a couple of
   non-band presets (e.g. "textures, no shadows") and measure them against
   the matrix to demonstrate the interpolation capability.
6. **Add the per-ray lazy mask (§3.2).** Introduce `requiredDetailMask` on the
   ray/hit path; gate `doExtraInformation`/`normal()` on `needsNormal()`. Set
   `DETAIL_NONE` for shadow rays and `q<=1` previews. Re-run matrix → this is
   the step expected to *beat* the baseline at low quality and for
   shadow-heavy scenes; quantify by cell.
7. **(Optional, later) `shadingType` enum.** Align povCpp's lighting selector
   with VITRAL's `ShadingType` so the flag set and VITRAL's
   `RendererConfiguration` share the shading vocabulary too.

---

## 6. Performance comparison method

For every step that can change timing (3, 5, 6):

1. `WIDTH=320 HEIGHT=200 ./scripts/renderQualityMatrix.sh` (same resolution as
   the baseline — relative comparison only needs matching pixels).
2. Diff the new `output/qualityMatrix.tsv` against the committed baseline:
   per-cell ratio `new/baseline`, plus per-quality-column geometric mean
   across the 108 scenes (the headline "how much did band qN cost change").
3. Record regressions (ratio > 1.0 at any preset level — the presets must not
   get slower) and the wins (ratio < 1.0, expected at low q after Step 6).
4. A step is acceptable when: both gates green, no preset-level cell is
   meaningfully slower than baseline, and Step 6 shows the intended low-q
   speedups.

The 108×10 shape matters because the quality lever's payoff is scene-dependent
(a shadowless scene like `iortest` at `q4` already shows shadows cost it
nothing — see analysis doc §7.2); only the full matrix reveals which scene
classes the lazy model actually accelerates.

---

## 7. Risks and open questions

- **Preset fidelity.** The §3.1 mapping must reproduce all ten bands
  bit-for-bit. `testQualities.sh` covers `iortest`; consider extending the
  quality baseline to 2–3 more scenes (e.g. a textured + a reflective one) if
  `iortest` proves too narrow a witness.
- **UV on the hit.** VITRAL's mask includes `DETAIL_UV`, but povCpp computes
  UV inside the texture pipeline, not on the hit record. The lazy mask can
  only gate `DETAIL_NORMAL` until UV is lifted onto the hit — a separate piece
  of work, not blocking this plan.
- **Matrix noise.** Wall-clock timing is noisy; use the geometric-mean column
  summary and, for borderline steps, average 2–3 matrix runs.
- **Does orthogonality earn its keep?** If Step 5's non-band presets show no
  useful time/quality point the existing bands don't already hit, the flag
  decomposition still has value (magic-number removal + VITRAL convergence),
  but the per-ray mask (Step 6) becomes the main efficiency justification.
