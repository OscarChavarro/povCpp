#ifndef __TEXTURE_ALIAS_ENTRY__
#define __TEXTURE_ALIAS_ENTRY__

class PovRayMaterial;

// One bookkeeping record for DefaultTextureAliasTracker: a default texture
// that one or more objects currently alias, how many are still aliasing it,
// and whether it has been retired (superseded by a later `default{}` block).
class TextureAliasEntry {
  public:
    // Required by java::ArrayList<T> to pre-allocate storage; not meant to
    // be used directly to build a meaningful entry (use the constructor
    // below for that).
    TextureAliasEntry() : material(nullptr), count(0), retired(false) {}

    explicit TextureAliasEntry(PovRayMaterial *material)
        : material(material), count(1), retired(false) {}

    PovRayMaterial *getMaterial() const { return material; }
    int getCount() const { return count; }
    bool isRetired() const { return retired; }

    void incrementCount() { count++; }
    void decrementCount() { count--; }
    void markRetired() { retired = true; }

  private:
    PovRayMaterial *material;
    int count;
    bool retired;
};

#endif
