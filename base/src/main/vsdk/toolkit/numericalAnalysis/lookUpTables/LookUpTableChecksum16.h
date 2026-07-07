#ifndef __LOOKUPTABLECHECKSUM16__
#define __LOOKUPTABLECHECKSUM16__

/**
Precomputed CRC-16 (CCITT/Modbus-style, reflected polynomial 0xA001) lookup
table. table[i] is the CRC-16 remainder for a single byte with value i,
letting callers update a running checksum byte by byte without recomputing
the polynomial division.
*/
class LookUpTableChecksum16 {
  private:
    static const unsigned short table[256];

  public:
    unsigned short eval(int index) const;
    int eval(const char *buf, int count) const;
};

/**
Returns the raw table entry for a byte value (table[index & 0xff]).
*/
inline unsigned short LookUpTableChecksum16::eval(int index) const
{
    return table[index & 0xff];
}

#endif
