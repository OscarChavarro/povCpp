#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_RandomSegmentOrder.h"
int _RandomSegmentOrder::nextPermutationIndex;
java::ArrayList<int> _RandomSegmentOrder::segmentPermutation;

double _RandomSegmentOrder::randomUnit() {
    return ((double)rand() / (double)RAND_MAX);
}

int _RandomSegmentOrder::generateRandomOrdering(int n) {
    // [SEID1991].3 Random segmentPermutation for incremental insertion order.
    int i;
    int m;
    java::ArrayList<int> localPermutation;
    int *p;

    nextPermutationIndex = 1;
    segmentPermutation.clear();
    segmentPermutation.reserve(n + 1);
    for (i = 0; i <= n; i++) {
        segmentPermutation.add(0);
    }

    for (i = 0; i <= n; i++) {
        localPermutation.add(i);
    }

    p = localPermutation.data();
    for (i = 1; i <= n; i++, p++) {
        m = (int)(floor((randomUnit() * 32000))) % (n + 1 - i) + 1;
        segmentPermutation[i] = p[m];
        if (m != 1) {
            p[m] = p[1];
        }
    }
    return 0;
}

int _RandomSegmentOrder::chooseSegment(void) {
    return segmentPermutation[nextPermutationIndex++];
}
