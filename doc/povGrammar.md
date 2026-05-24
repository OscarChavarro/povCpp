# POV Parser Grammar Findings (Current State)

## Scope
This document captures the current observed grammar behavior implemented by the legacy tokenizer/parser plus the AST-based pipeline in `src/io/pov`.

It is a behavioral inventory, not yet a strict ANTLR grammar.

## Top-Level Scene Structure
Observed top-level statements:

- `#declare <identifier> = <value>`
- `sphere { ... }`
- `light_source { ... }`
- `union { ... }`
- `intersection { ... }`
- `difference { ... }`
- `object { ... }`
- `composite { ... }`
- `fog { ... }`
- `default { ... }`
- `max_trace_level <float/int>`
- `camera { ... }` (`view_point` token in parser)

Top-level parsing currently accepts a mixed model:

- AST-native path for core geometry/CSG constructs.
- Legacy parsers for declarations and several scene directives.

## Declarations
`#declare` supports multiple value categories:

- Geometry and containers: `object`, `sphere`, `plane`, `triangle`, `smooth_triangle`, `quadric`, `poly/cubic/quartic`, `box`, `blob`, `bicubic_patch`, `height_field`, `union/intersection/difference`, `composite`
- Media/config: `texture`, `colour`, `light_source`, `camera/view_point`
- Primitive values: vector (`<x,y,z>`), float

Identifier references are resolved by `identifierNumber` through symbol tables.

## Core AST-Supported Geometry Forms
Current AST object parser natively models:

- `sphere`
- `light_source`
- `union`, `intersection`, `difference`
- `object`
- `composite`

For many advanced primitives nested in `object { ... }` and `composite { ... }`, the runtime still relies on legacy geometry parsers.

## Object and Composite Modifiers
Observed object-level modifiers:

- `bounded_by { <shape-list> }`
- `clipped_by { <shape-list> }`
- `translate <vector>`
- `rotate <vector>`
- `scale <vector>`
- `inverse`
- `no_shadow` (object)

## Light Source Fields
Observed light fields:

- position vector
- `colour`
- `point_at`
- `tightness`
- `radius`
- `falloff`
- `spotlight`
- transforms (`translate/rotate/scale`)

## Camera Fields
Observed camera fields:

- `location`
- `direction`
- `up`
- `right`
- `sky`
- `look_at`
- transforms (`translate/rotate/scale`)

## Fog / Defaults / Render Settings
Observed:

- `fog { colour ... <float distance> }`
- `default { ... }`
- `max_trace_level ...`

These are currently handled by legacy semantic parsers.

## Token/Grammar Hotspots Relevant for ANTLR
Areas where behavior is currently parser-driven (ad-hoc) and should become explicit grammar rules:

- Optional commas and delimiter tolerance in vectors/colors.
- Expression parsing for floats and signs (`+/-/float` token sequences).
- Identifier reference vs inline literal ambiguity.
- Nested shape parsing inside `object/composite` modifier blocks.
- Include-file boundaries and source-location continuity.

## Recommended Next Step Toward ANTLR
Turn this inventory into:

1. Lexer token spec (mapped to existing `Tokenizer` categories).
2. Parser rules for top-level statements and each block.
3. A typed semantic IR independent from runtime render objects.
4. A compatibility mapper from IR to current scene/model builders.
