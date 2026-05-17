#!/usr/bin/env bash
set -euo pipefail
set -f

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DB_VISIBLE="$ROOT_DIR/build/compile_commands.json"
DB_FALLBACK="$ROOT_DIR/compile_commands.json"
DB_LOCAL="$ROOT_DIR/compile_commands.rel.json"
MODO="check"
CHECKS_CHECK_DEFAULT="clang-analyzer-*"
CHECKS_FIX_DEFAULT="-*,modernize-loop-convert,modernize-use-nullptr,modernize-use-override,modernize-use-bool-literals,readability-braces-around-statements,readability-container-size-empty,readability-else-after-return,readability-redundant-control-flow"
FORMAT_STYLE_DEFAULT="file"

showUsage() {
  cat <<'USAGE'
Usage:
  ./scripts/lint.sh --check <path|regex> [<path|regex>...]
  ./scripts/lint.sh --fix <path|regex> [<path|regex>...]

Modes:
  --check   Show diagnostics only (does not modify files).
  --fix     Apply automatic fixes and formatting when possible.

Notes:
  In --fix mode the script uses a conservative safe check set by default.
  Formatting uses 4 spaces and never tabs.
  You can override checks with:
    LINT_CHECKS='your-checks-here' ./scripts/lint.sh --fix <targets>
  You can override format style with:
    LINT_FORMAT_STYLE='file' ./scripts/lint.sh --fix <targets>

Examples:
  ./scripts/lint.sh --check src/.*
  ./scripts/lint.sh --fix src/render/.*
  ./scripts/lint.sh --check src/Main.cpp
USAGE
}

if [[ $# -eq 0 ]]; then
  showUsage
  exit 0
fi

case "${1:-}" in
  --check)
    MODO="check"
    shift
    ;;
  --fix)
    MODO="fix"
    shift
    ;;
  -h|--help)
    showUsage
    exit 0
    ;;
esac

if [[ $# -eq 0 ]]; then
  echo "Missing target path/pattern to analyze." >&2
  showUsage
  exit 1
fi

if ! command -v run-clang-tidy >/dev/null 2>&1; then
  echo "run-clang-tidy is not installed" >&2
  exit 1
fi

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is not installed" >&2
  exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is not installed" >&2
  exit 1
fi

if [[ ! -f "$DB_VISIBLE" ]]; then
  if [[ -f "$DB_FALLBACK" ]]; then
    DB_VISIBLE="$DB_FALLBACK"
  else
    echo "Missing compilation database." >&2
    echo "Run ./scripts/compile.sh first." >&2
    exit 1
  fi
fi

if ! grep -q '/src/.*\.cpp"' "$DB_VISIBLE"; then
  echo "compile_commands.json does not belong to this project: $DB_VISIBLE" >&2
  echo "Run ./scripts/compile.sh to regenerate it." >&2
  exit 1
fi

jq 'map(.directory = "build")' "$DB_VISIBLE" > "$DB_LOCAL"
DB_VISIBLE="$DB_LOCAL"

cd "$ROOT_DIR"

SRC_FILES="$(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) | sort)"
TARGETS=()

expand_regex_target() {
  local regex="$1"
  local matched=0
  while IFS= read -r file; do
    if [[ "$file" =~ $regex ]]; then
      TARGETS+=("$file")
      matched=1
    fi
  done <<< "$SRC_FILES"
  if [[ "$matched" -eq 0 ]]; then
    echo "Warning: no files matched regex '$regex'" >&2
  fi
}

for target in "$@"; do
  normalized="${target#./}"

  if [[ -f "$normalized" ]]; then
    TARGETS+=("$normalized")
    continue
  fi

  expand_regex_target "$normalized"
done

if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "No input files resolved from provided targets." >&2
  exit 1
fi

if [[ "$MODO" == "fix" ]]; then
  CHECKS="${LINT_CHECKS:-$CHECKS_FIX_DEFAULT}"
  FORMAT_STYLE="${LINT_FORMAT_STYLE:-$FORMAT_STYLE_DEFAULT}"
  run-clang-tidy \
    -p "$(dirname "$DB_VISIBLE")" \
    -checks="$CHECKS" \
    -fix \
    -format \
    -style="$FORMAT_STYLE" \
    -extra-arg=-isystem/usr/include/c++/11 \
    -extra-arg=-isystem/usr/include/x86_64-linux-gnu/c++/11 \
    -header-filter=^src/.* \
    "${TARGETS[@]}"
  clang-format -i -style="$FORMAT_STYLE" "${TARGETS[@]}"
else
  CHECKS="${LINT_CHECKS:-$CHECKS_CHECK_DEFAULT}"
  run-clang-tidy \
    -p "$(dirname "$DB_VISIBLE")" \
    -checks="$CHECKS" \
    -extra-arg=-isystem/usr/include/c++/11 \
    -extra-arg=-isystem/usr/include/x86_64-linux-gnu/c++/11 \
    -header-filter=^src/.* \
    "${TARGETS[@]}"
fi
