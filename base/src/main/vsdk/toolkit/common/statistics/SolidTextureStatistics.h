#ifndef __VSDK_TOOLKIT_COMMON_STATISTICS_SOLIDTEXTURESTATISTICS_H__
#define __VSDK_TOOLKIT_COMMON_STATISTICS_SOLIDTEXTURESTATISTICS_H__

class SolidTextureStatistics {
  public:
    long callsToNoise = 0;
    long callsToDNoise = 0;

    void reset() { callsToNoise = 0L; callsToDNoise = 0L; }
};

#endif
