#!/usr/bin/env bash
set -euo pipefail

# Remove generated images
find . -type f -name '*.tga' -delete

# Remove common editor backup/temp files
find . -type f -name '*~' -delete
find . -type f -name '#*' -delete

# Remove build directory
rm -rf build

echo "Cleanup completed: removed .tga, *~, #* files, and build/."
