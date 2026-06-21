#ifndef __LOADED_IMAGE_REGISTRY__
#define __LOADED_IMAGE_REGISTRY__

#include "java/util/ArrayList.h"

class ControlledRGBAImageHDRUncompressed;

// Tracks every image_map/bump_map/material_map image loaded during parsing.
// These are intentionally shared (not cloned) across every PovRayMaterial
// generation and clone that references them - ImageMapPigment::copy() and
// PovRayMaterialBuilder's "rebuild from base" constructor both carry the same
// pointer forward rather than copying the pixel data - so no single owner can
// safely free one (whichever generation/clone is destroyed first would leave
// the others dangling). This registry is the single owner instead: freed once,
// after rendering completes (PovRayApplication::run(), which is the last point
// any texture lookup could still read from one), not tied to any one
// PovRayMaterial's lifetime.
class LoadedImageRegistry {
  public:
    static void registerImage(ControlledRGBAImageHDRUncompressed *image);
    static void freeAll();

  private:
    static java::ArrayList<ControlledRGBAImageHDRUncompressed *> &loadedImages();
};

#endif
