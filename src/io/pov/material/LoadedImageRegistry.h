#ifndef __LOADED_IMAGE_REGISTRY__
#define __LOADED_IMAGE_REGISTRY__

#include "java/util/ArrayList.h"

class ControlledRGBAImageHDRUncompressed;

class LoadedImageRegistry {
  public:
    static void registerImage(ControlledRGBAImageHDRUncompressed *image);
    static void freeAll();

  private:
    static java::ArrayList<ControlledRGBAImageHDRUncompressed *> &loadedImages();
};

#endif
