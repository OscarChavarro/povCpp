#ifndef __INCREMENTAL_SEGMENT_INSERTER__
#define __INCREMENTAL_SEGMENT_INSERTER__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationSegment.h"
class _IncrementalSegmentInserter {
  public:
    static int addSegment(int segmentIndex);

  private:
    static void normalizeSegmentForInsertion(_TriangulationSegment &segment, bool &isSwapped);
    static void updateLowerNeighbourLinksAfterSplit(int splitTrapIndex,
                                                    int originalUpperTrapIndex);
    static int splitTrapezoidAtEndpoint(int segmentIndex, _TriangulationSegment &segment,
                                        bool useFirstEndpoint);
    static int locateOrInsertEndpointTrapezoid(int segmentIndex, _TriangulationSegment &segment,
                                               bool useFirstEndpoint,
                                               bool endpointAlreadyInserted,
                                               bool &wasEndpointInserted);
    static bool isLeftOf(int segmentIndex, Vector2Dd *queryPoint);
    static bool inserted(int segmentIndex, int whichPoint);
    static int mergeTrapezoids(int segmentIndex, int tfirst, int tlast,
                               int side);
};

#endif
