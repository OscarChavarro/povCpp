#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "java/util/ArrayList.txx"

java::ArrayList<const PovRayMaterial *> &
PovRayMaterialConstancy::constantMaterials()
{
    static java::ArrayList<const PovRayMaterial *> instances;
    return instances;
}

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

void
PovRayMaterialConstancy::unmarkConstant(const PovRayMaterial *material)
{
    constantMaterials().remove(material);
}
