#ifndef __RANDOM_SEGMENT_ORDER__
#define __RANDOM_SEGMENT_ORDER__

#include <cstdlib>

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
class _RandomSegmentOrder {
  private:
    static int nextPermutationIndex;
    static java::ArrayList<int> segmentPermutation;
    static double randomUnit();

  public:
    static int generateRandomOrdering(int n);
    static int chooseSegment(void);
};

#endif
