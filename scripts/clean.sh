#!/usr/bin/env bash
set -euo pipefail

# Remove generated images
find . -type f -name '*.tga' -delete

# Remove common editor backup/temp files
find . -type f -name '*~' -delete
find . -type f -name '#*' -delete

# Remove build and output directories
rm -rf build
rm -rf base/build
rm -rf output outputCsgByRaySegments
rm -rf compile_commands.json
