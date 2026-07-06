#ifndef __CULL_SAFE_ENTRY__
#define __CULL_SAFE_ENTRY__

// Sort key (chosen axis centroid coordinate) paired with the entry's
// position in the bucket - private replacement for std::pair<double, int>.
class CullSafeEntry {
  public:
    CullSafeEntry() : key(0.0), position(0) {}
    CullSafeEntry(double key, int position) : key(key), position(position) {}

    double getKey() const { return key; }
    int getPosition() const { return position; }

  private:
    double key;
    int position;
};

#endif
