#include <cmath>

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_InsertionBatchSchedule.h"
double _InsertionBatchSchedule::log2Value(double x) { return log(x) / log(2.0); }

int _InsertionBatchSchedule::mathLogStarN(int n) {
    // [SEID1991].3 Staged insertion schedule uses iterated logarithm depth.
    int i;
    double v;

    for (i = 0, v = (double)n; v >= 1; i++) {
        v = log2Value(v);
    }
    return (i - 1);
}

int _InsertionBatchSchedule::mathN(int n, int h) {
    // [SEID1991].3 Boundary for stage h in batched randomized insertion.
    int i;
    double v;

    for (i = 0, v = (int)n; i < h; i++) {
        v = log2Value(v);
    }
    return (int)ceil((double)1.0 * n / v);
}
