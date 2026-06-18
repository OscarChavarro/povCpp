#ifndef __HEIGHT_FIELD_BLOCK_H__
#define __HEIGHT_FIELD_BLOCK_H__


class HeightFieldBlock {
  public:
    double getMinY() const;
    void setMinY(double value);
    double getMaxY() const;
    void setMaxY(double value);

  private:
    double minY;
    double maxY;
};

inline double
HeightFieldBlock::getMinY() const
{
    return minY;
}

inline void
HeightFieldBlock::setMinY(double value)
{
    minY = value;
}

inline double
HeightFieldBlock::getMaxY() const
{
    return maxY;
}

inline void
HeightFieldBlock::setMaxY(double value)
{
    maxY = value;
}

#endif
