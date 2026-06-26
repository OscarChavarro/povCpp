#include "environment/material/povray/DefaultTextureAliasTracker.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "java/util/ArrayList.txx"

java::ArrayList<TextureAliasEntry> &
DefaultTextureAliasTracker::entries()
{
    static java::ArrayList<TextureAliasEntry> instances{};
    return instances;
}

long int
DefaultTextureAliasTracker::findEntryIndex(PovRayMaterial *material)
{
    java::ArrayList<TextureAliasEntry> &list = entries();
    for (long int i = 0; i < list.size(); i++) {
        if (list[i].getMaterial() == material) {
            return i;
        }
    }
    return -1;
}

void
DefaultTextureAliasTracker::trackAlias(PovRayMaterial *defaultTexture)
{
    if (defaultTexture == nullptr) {
        return;
    }
    const long int idx = findEntryIndex(defaultTexture);
    if (idx != -1) {
        entries()[idx].incrementCount();
        return;
    }
    entries().add(TextureAliasEntry(defaultTexture));
}

void
DefaultTextureAliasTracker::releaseAlias(PovRayMaterial *material)
{
    if (material == nullptr) {
        return;
    }
    java::ArrayList<TextureAliasEntry> &list = entries();
    const long int idx = findEntryIndex(material);
    if (idx == -1) {
        return;
    }
    list[idx].decrementCount();
    if (list[idx].isRetired() && list[idx].getCount() <= 0) {
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
    java::ArrayList<TextureAliasEntry> &list = entries();
    const long int idx = findEntryIndex(oldDefaultTexture);
    if (idx == -1) {
        // Nothing ever aliased it (no untextured object was placed before
        // this retirement) - safe to free right away.
        PovRayMaterialConstancy::unmarkConstant(oldDefaultTexture);
        delete oldDefaultTexture;
        return;
    }
    list[idx].markRetired();
    if (list[idx].getCount() <= 0) {
        PovRayMaterialConstancy::unmarkConstant(oldDefaultTexture);
        delete oldDefaultTexture;
        list.remove(idx);
    }
}
