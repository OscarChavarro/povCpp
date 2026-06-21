#ifndef __DEFAULT_TEXTURE_ALIAS_TRACKER__
#define __DEFAULT_TEXTURE_ALIAS_TRACKER__

#include "java/util/ArrayList.h"
#include "environment/material/TextureAliasEntry.h"

class PovRayMaterial;

// ObjectParser::parseObject/parseComposite initialize an untextured object's
// objectTexture directly to ctx.getDefaultTexture() (an alias, not a clone -
// see ensurePrivateTexture()'s comment in ObjectParser.cpp for why: cloning
// unconditionally would cost one allocation per untextured object, of which a
// scene can have many). That alias normally lives as long as the owning
// BoundedGeometry, which is fine while the scene's default texture itself
// lives until ~Scene(). But a `default { texture {...} }` block replaces
// ctx's default texture mid-parse, and objects placed *before* that point may
// still be aliasing the *old* one - it can't simply be freed when replaced.
//
// This tracker makes that safe without giving up the no-clone-for-untextured-
// objects optimization: every alias start/end is counted, and a retired
// (superseded) default texture is only actually freed once its count reaches
// zero - whether that happens immediately (nothing was aliasing it) or later
// (when the last aliasing object is transformed away from it or destroyed).
class DefaultTextureAliasTracker {
  public:
    // Call wherever an object's texture is initialized to (aliases) the
    // scene's current default texture.
    static void trackAlias(PovRayMaterial *defaultTexture);

    // Call wherever such an alias ends: ensurePrivateTexture() cloning away
    // from it, or the owning object being destroyed while still aliasing it.
    // Safe no-op if `material` was never tracked (e.g. it is some other
    // marked-constant texture, not the default).
    static void releaseAlias(PovRayMaterial *material);

    // Call when ctx's default texture is about to be replaced (a `default{}`
    // block), passing the *old* one. Frees it immediately if nothing is
    // currently aliasing it, otherwise defers the free until the last
    // releaseAlias() call brings its count to zero.
    static void retire(PovRayMaterial *oldDefaultTexture);

  private:
    // All currently tracked default-texture aliases, process-wide.
    static java::ArrayList<TextureAliasEntry> &entries();

    // Index of the entry tracking `material`, or -1 if it isn't tracked.
    static long int findEntryIndex(PovRayMaterial *material);
};

#endif
