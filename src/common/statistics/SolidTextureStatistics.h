#ifndef __SOLID_TEXTURE_STATISTICS_H__
#define __SOLID_TEXTURE_STATISTICS_H__

class SolidTextureStatistics {
  public:
    long callsToNoise = 0;
    long callsToDNoise = 0;
    void reset() { callsToNoise = 0L; callsToDNoise = 0L; }
};

#endif
