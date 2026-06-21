#include "io/pov/material/DefaultTextureAliasTracker.h"
#include "environment/material/PovRayMaterial.h"
#include "io/pov/material/PovRayMaterialConstancy.h"
#include "java/util/ArrayList.txx"

namespace {

struct AliasEntry {
    PovRayMaterial *material;
    int count;
    bool retired;
};

java::ArrayList<AliasEntry> &
entries()
{
    static java::ArrayList<AliasEntry> instances{};
    return instances;
}

long int
findEntryIndex(PovRayMaterial *material)
{
    java::ArrayList<AliasEntry> &list = entries();
    for (long int i = 0; i < list.size(); i++) {
        if (list[i].material == material) {
            return i;
        }
    }
    return -1;
}

} // namespace

void
DefaultTextureAliasTracker::trackAlias(PovRayMaterial *defaultTexture)
{
    if (defaultTexture == nullptr) {
        return;
    }
    const long int idx = findEntryIndex(defaultTexture);
    if (idx != -1) {
        entries()[idx].count++;
        return;
    }
    entries().add(AliasEntry{defaultTexture, 1, false});
}

void
DefaultTextureAliasTracker::releaseAlias(PovRayMaterial *material)
{
    if (material == nullptr) {
        return;
    }
    java::ArrayList<AliasEntry> &list = entries();
    const long int idx = findEntryIndex(material);
    if (idx == -1) {
        return;
    }
    list[idx].count--;
    if (list[idx].retired && list[idx].count <= 0) {
        PovRayMaterialConstancy::unmarkConstant(material);
        delete material;
        list.remove(idx);
    }
}

void
DefaultTextureAliasTracker::retire(PovRayMaterial *oldDefaultTexture)
{
    if (oldDefaultTexture == nullptr) {
        return;
    }
    java::ArrayList<AliasEntry> &list = entries();
    const long int idx = findEntryIndex(oldDefaultTexture);
    if (idx == -1) {
        // Nothing ever aliased it (no untextured object was placed before
        // this retirement) - safe to free right away.
        PovRayMaterialConstancy::unmarkConstant(oldDefaultTexture);
        delete oldDefaultTexture;
        return;
    }
    list[idx].retired = true;
    if (list[idx].count <= 0) {
        PovRayMaterialConstancy::unmarkConstant(oldDefaultTexture);
        delete oldDefaultTexture;
        list.remove(idx);
    }
}
