#ifndef __CHUNK_HEADER_H__
#define __CHUNK_HEADER_H__

class ChunkHeader {
  public:
    long getName() const;
    void setName(long value);
    long getSize() const;
    void setSize(long value);

  private:
    long name;
    long size;
};

inline long
ChunkHeader::getName() const
{
    return name;
}

inline void
ChunkHeader::setName(long value)
{
    name = value;
}

inline long
ChunkHeader::getSize() const
{
    return size;
}

inline void
ChunkHeader::setSize(long value)
{
    size = value;
}

#endif
