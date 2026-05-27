# Implementation Plan: Complete ANTLR `.pov` Parser Parity

## Goal

Achieve 1:1 byte-exact parity between the ANTLR-based parser (povrayAntlr) and the legacy golden reference images, as measured by scripts/testAgainstGoldenImages.sh.

Measurable target: bash scripts/testAgainstGoldenImages.sh prints "Test passed." with 0 issues across all 108 reference .tga images (levels 1-3 + math).

Constraint: Only files under src/io/ may be modified. Scripts, grammar regeneration config, and CMakeLists.txt are already in place. All work happens in the IR mapper and lowering layers.

## Assumptions

What works (confirmed):
- The build wiring exists: scripts/compileAntrl.sh builds povrayAntlr with POV_WITH_ANTLR_RUNTIME
- The ANTLR pipeline is fully wired end-to-end (src/io/pov/SceneParser.cpp:93-121)
- Core geometry + basic textures already reach byte parity: level1/blob.tga, level1/box.tga, level1/dish.tga, math/bicube.tga currently match exactly
- Reference images exist at ../referenceTestImages (108 .tga files)

What does NOT work (confirmed):
- The current output/ tree is stale (104/108 images differ). It must be regenerated first.
- Texture handling is the dominant fragility. AntlrSceneLowering::applyRawTextureElement re-parses texture chains from raw flattened text
- buildCompositeFromIr needs to resolve declared planes/boxes/triangles/quadrics/quartics/blobs/lights by name
- Several "Unsupported ANTLR ... shape" throws remain in the mapper (AntlrParseTreeToIrMapper.cpp:842, 1050, 1099, 1375)

Dependencies:
- There is NO runtime legacy fallback when ANTLR is enabled: on any ANTLR failure the render fails
- Parity is byte-exact: even a one-LSB color rounding difference fails
- Legacy reference renderer (povrayLegacy) reproduces the golden images

## Step-by-Step Actions

PHASE 0 -- Establish a clean baseline (no code edits)

Step 0.1 -- Build both binaries
Run: bash scripts/compileLegacy.sh
Run: bash scripts/compileAntrl.sh
Validation: both binaries exist and are newer than the source tree

Step 0.2 -- Regenerate the ANTLR output and run the parity oracle
Run: bash scripts/runAntlr.sh (renders all scenes into output/)
Run: bash scripts/testAgainstGoldenImages.sh
Validation: Capture the authoritative current failure list. Record (a) scenes whose .tga is missing (ANTLR crash) and (b) scenes whose .tga differs (wrong pixels). The 4 known-good scenes (blob, box, dish, bicube) MUST still pass.
Parity impact: baseline; defines the work queue

Step 0.3 -- Build a per-scene triage list
For each failing scene, categorize from its _stderr.log:
- Class A: Crash/abort (missing .tga, stderr shows "Unsupported ANTLR ..." or exception). Highest priority.
- Class B: Wrong pixels (.tga exists but differs). Lowering-fidelity gaps.
Validation: a written scene-to-class-to-suspected-feature mapping. No code yet.

PHASE 1 -- Eliminate Class A crashes (parse/lowering aborts)

Each crash blocks an entire image. Fix the broadest causes first.

Step 1.1 -- Resolve "Unsupported ANTLR object child shape" (AntlrParseTreeToIrMapper.cpp:842)
File: src/io/pov/antlr/AntlrParseTreeToIrMapper.cpp
Inspect the offending scene's object body; extend the mapper's object-child dispatch to cover the missing shapeStatement alternative (grammar rule objectBodyElement, lines 240-257).
Map it into the existing AntlrIrObjectNode child structures used by buildObjectFromIr (AntlrSceneLowering.cpp:1335).
Validation: the previously-missing .tga is now produced; diff against reference

Step 1.2 -- Resolve "Unsupported ANTLR bounded_by/clipped_by shape" (AntlrParseTreeToIrMapper.cpp:1050, 1099)
File: src/io/pov/antlr/AntlrParseTreeToIrMapper.cpp
Extend the boundedShapeElement handling (grammar lines 318-321: shapeStatement | identifierInvocation).
Wire into the bounded/clipped fields consumed by buildObjectFromIr/buildCompositeFromIr.
Validation: scenes using bounded_by/clipped_by render without abort; diff each

Step 1.3 -- Resolve "Unsupported ANTLR CSG child shape" (AntlrParseTreeToIrMapper.cpp:1375)
File: src/io/pov/antlr/AntlrParseTreeToIrMapper.cpp
Extend CSG child dispatch (grammar csgBodyElement, lines 299-308) to cover all shapeStatement alternatives plus identifierInvocation.
Lower into buildCsgFromIr (AntlrSceneLowering.cpp:1834).
Validation: union/intersection/difference scenes produce images; diff each

Step 1.4 -- Replace silent geometry fallbacks with hard failures
File: src/io/pov/antlr/AntlrSceneLowering.cpp
For each fallback substitution (makeFallbackSphereNode at line 86, used at 177; fallback plane 213, box 250, triangle 287, smooth-triangle 329, fallback quadric 389):
- Confirm via POVCPP_ANTLR_DEBUG_RESOLVE=1 env hook whether any test scene triggers them
- For every scene that does, fix the name resolution so the real geometry resolves
- Do NOT leave the fallback active for a tested scene

Validation: run each affected scene with POVCPP_ANTLR_DEBUG_RESOLVE=1; stderr must show NO fallback_sphere/unresolved messages for any tested scene

After Phase 1: rerun bash scripts/runAntlr.sh && bash scripts/testAgainstGoldenImages.sh. Expect zero "Missing generated image" failures.

PHASE 2 -- Close Class B fidelity gaps (texture / color / colour-map)

This is the largest bucket. Work feature-by-feature, smallest-blast-radius scenes first.

Step 2.1 -- Lock down named-color and color-keyword fidelity
File: src/io/pov/antlr/AntlrSceneLowering.cpp (lines 659 parseNamedColourIdentifierForLowering, 760 parseColourSpec, 702 isBuiltinColourName)
Cross-check every named color against legacy color table.
Verify the whitespace-collapse splitter (line 778) handles all colorXcolorY concatenations.
Confirm RGB/RGBA defaults and alpha handling (hasAlpha path at 1230-1248) match legacy exactly.
Validation target: level1/colors.pov, level1/shapes.pov, level1/alphafun.pov reach byte parity
Parity impact: color-only scenes flip to matching

Step 2.2 -- Color-map fidelity
File: src/io/pov/antlr/AntlrSceneLowering.cpp (line 856 parseColourMapRaw)
Use POVCPP_ANTLR_DEBUG_COLOURMAP=1 hook (committed at line 905) to dump parsed spans.
Compare span boundaries/colors/transparency-flag against legacy.
Fix span sorting, boundary inclusivity, and per-stop color parsing.
Validation target: level1/stone1..4.pov, level1/sunset*.pov, level2/skyvase.pov reach parity

Step 2.3 -- Procedural texture parameters
File: src/io/pov/antlr/AntlrSceneLowering.cpp (line 1033 applyRawTextureElement)
Verify each procedural attribute is parsed from the raw element text and applied with legacy-identical defaults.
Key attributes: gradient direction (parseVectorAfterKeyword at 956, applied 1171), turbulence, bump*, phase, frequency, scale/translate/rotate ordering, tiles, checker, marble, etc.
For each, locate the legacy counterpart in src/io/pov/mediaParser/TextureParser.cpp and match field assignment exactly.
Validation target: level1/texture1..3.pov, level1/checker2.pov, level1/cantelop.pov, level1/mapper.pov, level1/mappr2.pov, level1/matmap.pov reach parity
Parity impact: the bulk of level1/level2 diffs

Step 2.4 -- Texture chain layering & default-texture inheritance
File: src/io/pov/antlr/AntlrSceneLowering.cpp (line 2217 applyShapeTexture, 2230 applyObjectTexture)
File: src/io/pov/antlr/AntlrParseTreeToIrMapper.cpp (line 185 parseTextureChain, 218 parseSingleTextureElement)
Confirm default { texture {...} } seeds the same baseline as legacy.
Verify multi-element texture chains and nested texture{ texture{} } compose in the same order as legacy.
Verify simpleReferenceIdentifiers resolution against declared textures.
Validation target: shapes2, glasdish, glass, and other default-texture-dependent scenes reach parity

Step 2.5 -- Composite/CSG named-member resolution end-to-end
File: src/io/pov/antlr/AntlrSceneLowering.cpp (block 1703-1770)
Verify the resolution of declared planes/boxes/triangles/smooth-triangles/quadrics/quartics/blobs/lights inside composites.
Link members in the SAME order legacy does.
Apply inherited transforms/textures identically.
Confirm csgBodyElement identifier members resolve too.
Validation target: level2/level3 assembly scenes (room, palace, chess, teapot, car) render with all members present; diff

Step 2.6 -- Camera / fog / light numeric parity
File: src/io/pov/antlr/AntlrSceneLowering.cpp (line 2248 applyCameraNode, buildLight 579)
Confirm camera vector defaults (location/direction/up/right/sky/look_at), light defaults (spotlight/falloff/radius/tightness/point_at), and fog color/distance match legacy field-for-field.
Validation target: level1/spotlite.pov, level1/fogtst.pov, level1/basicvue.pov, level1/laser.pov reach parity

Step 2.7 -- Math suite (quadric/quartic/blob) parity
File: src/io/pov/antlr/AntlrSceneLowering.cpp (buildQuadric 373, buildQuartic 471, buildBlob 518)
The 19 math/* scenes are pure-geometry.
Verify coefficient ordering and sturm flag match legacy exactly.
math/bicube.tga already passes -- use it as a known-good reference.
Validation target: all math/*.tga reach parity

PHASE 3 -- Full-suite convergence

Step 3.1 -- Iterate to zero
After each Phase-2 sub-step, rerun: bash scripts/runAntlr.sh && bash scripts/testAgainstGoldenImages.sh
Maintain the shrinking failure list.
For any stubborn single-scene diff, render with both povrayAntlr and povrayLegacy.
Use POVCPP_ANTLR_DUMP_EXPANDED env hook plus debug-resolve/colour-map hooks to pinpoint diverging fields.
Validation: monotonically decreasing failure count; no regression of previously-passing scenes (re-check blob/box/dish/bicube every iteration)

Step 3.2 -- Final gate
bash scripts/testAgainstGoldenImages.sh prints "Test passed." (0 issues, all 108 images byte-identical)

## Validation Criteria (summary)

Per code edit: Affected scene's output/.../X.tga exists (no crash) and diff -q vs reference shrinks toward identical.

Per phase: Full runAntlr.sh + testAgainstGoldenImages.sh; failure count strictly decreases; the 4 baseline-good scenes never regress.

Phase 1 done: Zero "Missing generated image" failures (no parse/lowering aborts).

Phase 2 done: All texture/color/colour-map/camera/fog/light/math scenes byte-match.

Final: "Test passed." -- 108/108 byte-identical.

Debug hooks available (all already in code): POVCPP_ANTLR_DEBUG_RESOLVE=1, POVCPP_ANTLR_DEBUG_COLOURMAP=1, POVCPP_ANTLR_DUMP_EXPANDED=<file>.

## Risks and Rollback Notes

Risk 1: Raw-text texture parsing is inherently brittle
applyRawTextureElement reconstructs semantics from flattened parse-tree text (getText()), which collapses whitespace and concatenates tokens (the colorGraycolorGray bug).
Mitigation: prefer reading structured fields from the IR/parse-tree. Reserve raw-text parsing for attributes with no structured representation.
Rollback: each texture feature is an isolated branch in applyRawTextureElement; revert the specific branch if it regresses passing scenes.

Risk 2: No runtime legacy fallback
Any unhandled construct aborts the whole image (Class A).
Mitigation: Phase 1 is mandatory before Phase 2 yields measurable gains.

Risk 3: Silent fallback geometry masks bugs
Fallback sphere/quadric/plane stubs produce non-crashing but wrong images.
Mitigation: gate every fallback behind the POVCPP_ANTLR_DEBUG_RESOLVE assertion check; ensure no tested scene reaches a fallback.

Risk 4: Byte-exact rounding sensitivity
A correct-looking image can still differ by one LSB if color/transform math diverges from legacy ordering.
Mitigation: mirror legacy field assignment order and default constants exactly (cross-reference src/io/pov/mediaParser/TextureParser.cpp, cameraParser/CameraParser.cpp, geometryParser/*).

Risk 5: Grammar gaps would be blocking
If a test scene uses a construct the grammar (grammar/POVParser.g4) cannot parse, fixing it needs a grammar change + regeneration -- outside src/io/.
Mitigation: the grammar appears to cover the suite. If Phase 1 surfaces a genuine grammar gap (parse syntax error, not a mapper throw), STOP and escalate -- do not attempt a workaround in lowering.

Risk 6: Out-of-scope edits
Only src/io/ may change.
Mitigation: all steps target files under src/io/pov/antlr/ exclusively. Primary edit surface: AntlrSceneLowering.cpp, AntlrParseTreeToIrMapper.cpp.
Rollback: the work is uncommitted; git diff src/io/pov/antlr/ shows all changes, and per-file git checkout -- restores any regressed file.

## Summary

The ANTLR migration is structurally complete and wired end-to-end; core geometry already byte-matches on 4 scenes. The remaining work is lowering fidelity, concentrated in src/io/pov/antlr/AntlrSceneLowering.cpp (raw-text texture/color/colour-map parsing) and a handful of "Unsupported ANTLR ..." aborts in AntlrParseTreeToIrMapper.cpp. Parity is byte-exact TGA diff vs ../referenceTestImages via scripts/testAgainstGoldenImages.sh; there is no runtime legacy fallback, so every scene must both parse and pixel-match. The current output/ is stale and must be regenerated as step one. The plan above is ordered (crashes -> fidelity -> convergence), references concrete src/io files with line numbers, and stays within the src/io-only constraint.
