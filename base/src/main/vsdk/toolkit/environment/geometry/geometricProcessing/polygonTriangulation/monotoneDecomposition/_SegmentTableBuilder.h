#ifndef __SEGMENT_TABLE_BUILDER__
#define __SEGMENT_TABLE_BUILDER__

class _SegmentTableBuilder {
  public:
    static int prepareSegments(double *vertices, int numberOfVertices,
                               int *contourSizes, int numberOfContours);
};

#endif
