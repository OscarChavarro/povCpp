#ifndef __BLOB_LIST_H__
#define __BLOB_LIST_H__

#include "environment/geometry/BlobElement.h"

class BlobList {
  public:
    BlobElement elem;
    BlobList *next;
};

#endif
