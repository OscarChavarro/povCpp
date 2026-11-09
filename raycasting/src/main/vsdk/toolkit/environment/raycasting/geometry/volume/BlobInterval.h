#ifndef __BLOB_INTERVAL__
#define __BLOB_INTERVAL__


class BlobInterval {
  public:
    int getType() const;
    void setType(int value);
    int getIndex() const;
    void setIndex(int value);
    double getBound() const;
    void setBound(double value);

  private:
    int type;
    int index;
    double bound;
};

inline int
BlobInterval::getType() const
{
    return type;
}

inline void
BlobInterval::setType(int value)
{
    type = value;
}

inline int
BlobInterval::getIndex() const
{
    return index;
}

inline void
BlobInterval::setIndex(int value)
{
    index = value;
}

inline double
BlobInterval::getBound() const
{
    return bound;
}

inline void
BlobInterval::setBound(double value)
{
    bound = value;
}

#endif
