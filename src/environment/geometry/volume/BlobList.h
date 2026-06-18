#ifndef __BLOB_LIST_H__
#define __BLOB_LIST_H__

#include "environment/geometry/volume/BlobElement.h"

class BlobList {
  public:
    BlobElement &getElem();
    const BlobElement &getElem() const;
    BlobList *getNext() const;
    void setNext(BlobList *value);

  private:
    BlobElement elem;
    BlobList *next;
};

inline BlobElement &
BlobList::getElem()
{
    return elem;
}

inline const BlobElement &
BlobList::getElem() const
{
    return elem;
}

inline BlobList *
BlobList::getNext() const
{
    return next;
}

inline void
BlobList::setNext(BlobList *value)
{
    next = value;
}

#endif
