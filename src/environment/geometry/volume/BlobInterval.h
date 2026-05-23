#ifndef __BLOB_INTERVAL_H__
#define __BLOB_INTERVAL_H__

#include "common/LegacyBoolean.h"

class BlobInterval {
  public:
    int type;
    int index;
    double bound;
};

#endif
