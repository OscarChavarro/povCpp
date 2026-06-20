#include "io/pov/material/PovRayMaterialConstancy.h"
#include "environment/material/PovRayMaterial.h"
#include "java/util/ArrayList.txx"

namespace {

java::ArrayList<const PovRayMaterial *> &
constantMaterials()
{
    static java::ArrayList<const PovRayMaterial *> instances;
    return instances;
}

} // namespace

void
PovRayMaterialConstancy::markConstant(const PovRayMaterial *material)
{
    if (!isConstant(material)) {
        constantMaterials().add(material);
    }
}

bool
PovRayMaterialConstancy::isConstant(const PovRayMaterial *material)
{
    java::ArrayList<const PovRayMaterial *> &instances = constantMaterials();
    for (long int i = 0; i < instances.size(); i++) {
        if (instances[i] == material) {
            return true;
        }
    }
    return false;
}
