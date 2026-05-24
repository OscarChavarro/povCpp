#ifndef __LEGACY_BOOLEAN_H__
#define __LEGACY_BOOLEAN_H__

class LegacyBoolean {
  public:
    // The original renderer stores booleans in integer fields throughout its data model.
    static constexpr int TRUE_VALUE = 1;
    static constexpr int FALSE_VALUE = 0;
};

#endif
