#include "io/pov/material/LoadedImageRegistry.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "java/util/ArrayList.txx"

namespace {

java::ArrayList<ControlledRGBAImageHDRUncompressed *> &
loadedImages()
{
    static java::ArrayList<ControlledRGBAImageHDRUncompressed *> instances{};
    return instances;
}

} // namespace

void
LoadedImageRegistry::registerImage(ControlledRGBAImageHDRUncompressed *image)
{
    loadedImages().add(image);
}

void
LoadedImageRegistry::freeAll()
{
    java::ArrayList<ControlledRGBAImageHDRUncompressed *> &instances = loadedImages();
    for (long int i = 0; i < instances.size(); i++) {
        delete instances[i];
    }
    instances.clear();
}
