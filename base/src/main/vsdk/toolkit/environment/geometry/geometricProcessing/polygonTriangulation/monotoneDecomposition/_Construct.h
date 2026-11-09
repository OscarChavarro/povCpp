#ifndef __CONSTRUCT__
#define __CONSTRUCT__

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationSegment.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationTrapezoid.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationTrapezoidQueryNode.h"
constexpr int T_X = 1;
constexpr int T_Y = 2;
constexpr int T_SINK = 3;

constexpr int SEGMENT_SIZE = 200;
constexpr int QSIZE = 8 * SEGMENT_SIZE;
constexpr int TRAPEZOID_TABLE_SIZE = 4 * SEGMENT_SIZE;

constexpr int SEGMENT_FIRST_ENDPOINT = 1;
constexpr int SEGMENT_LAST_ENDPOINT = 2;

constexpr int TRIANGULATION_INFINITY = 1 << 30;

constexpr double TRIANGULATOR_EPSILON = 1.0e-7;

constexpr int S_LEFT = 1;
constexpr int S_RIGHT = 2;

constexpr int ST_VALID = 1;
constexpr int ST_INVALID = 2;

class _Construct {
  public:
    static int nextQueryNodeIndex;
    static int nextTrapezoidIndex;
    static java::ArrayList<_TriangulationTrapezoidQueryNode *> queryNodes;
    static java::ArrayList<_TriangulationTrapezoid *> trapezoids;
    static java::ArrayList<_TriangulationSegment> segments;
    static void resetTrapezoidQueryNodes();
    static void resetTrapezoids();
    static void prepareStorage(int nseg);

  private:
    static int max(Vector2Dd *yvalPoint, Vector2Dd *firstPoint,
                   Vector2Dd *secondPoint);
    static int min(Vector2Dd *yvalPoint, Vector2Dd *firstPoint,
                   Vector2Dd *secondPoint);
    static _TriangulationTrapezoidQueryNode *initQueryStructure(int segmentIndex);
    static int findNewRoots(int segmentIndex);

  public:
    static bool greaterThan(Vector2Dd *firstPoint, Vector2Dd *secondPoint);
    static bool equalTo(Vector2Dd *firstPoint, Vector2Dd *secondPoint);
    static bool greaterThanEqualTo(Vector2Dd *firstPoint,
                                   Vector2Dd *secondPoint);
    static bool lessThan(Vector2Dd *firstPoint, Vector2Dd *secondPoint);
    static double cross(const Vector2Dd &v0, const Vector2Dd &v1,
                        const Vector2Dd &v2);
    static double dot(const Vector2Dd &v0, const Vector2Dd &v1);
    static bool fpEqual(double firstValue, double secondValue);
    static int constructTrapezoids(int nseg);
    static _TriangulationSegment &segmentAt(int index);
    static _TriangulationTrapezoid *trapezoidAt(int index);
    static void setTrapezoidAt(int index, _TriangulationTrapezoid *trapezoid);
    static _TriangulationTrapezoidQueryNode *allocateQueryNode();
    static int allocateTrapezoidIndex();
};

#endif
