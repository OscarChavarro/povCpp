#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_SegmentTableBuilder.h"
int _SegmentTableBuilder::prepareSegments(double *vertices,
                                          int numberOfVertices,
                                          int *contourSizes,
                                          int numberOfContours) {
    int contourCount;
    int i;
    int j;
    int numPoints;
    int first;
    int last;

    _Construct::prepareStorage(numberOfVertices);

    for (contourCount = 0, i = 1; contourCount < numberOfContours;
         contourCount++) {
        numPoints = contourSizes[contourCount];
        first = i;
        last = first + numPoints - 1;
        for (j = 0; j < numPoints; j++, i++) {
            _Construct::segmentAt(i).startPoint.x = vertices[2 * (i - 1)];
            _Construct::segmentAt(i).startPoint.y = vertices[2 * (i - 1) + 1];
            if (i == last) {
                _Construct::segmentAt(i).nextSegmentIndex = first;
                _Construct::segmentAt(i).previousSegmentIndex = i - 1;
                _Construct::segmentAt(i - 1).endPoint = _Construct::segmentAt(i).startPoint;
            } else if (i == first) {
                    _Construct::segmentAt(i).nextSegmentIndex = i + 1;
                    _Construct::segmentAt(i).previousSegmentIndex = last;
                    _Construct::segmentAt(last).endPoint = _Construct::segmentAt(i).startPoint;
            } else {
                    _Construct::segmentAt(i).previousSegmentIndex = i - 1;
                    _Construct::segmentAt(i).nextSegmentIndex = i + 1;
                    _Construct::segmentAt(i - 1).endPoint = _Construct::segmentAt(i).startPoint;
            }
            _Construct::segmentAt(i).hasBeenInserted = false;
        }
    }

    return i - 1;
}
