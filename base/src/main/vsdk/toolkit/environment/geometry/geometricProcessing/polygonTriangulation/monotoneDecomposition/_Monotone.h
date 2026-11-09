#ifndef __MONOTONE__
#define __MONOTONE__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_MonotoneChainNode.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_VertexChain.h"
constexpr int SP_SIMPLE_LRUP = 1;
constexpr int SP_SIMPLE_LRDN = 2;
constexpr int SP_2UP_2DN = 3;
constexpr int SP_2UP_LEFT = 4;
constexpr int SP_2UP_RIGHT = 5;
constexpr int SP_2DN_LEFT = 6;
constexpr int SP_2DN_RIGHT = 7;
constexpr int SP_NOSPLIT = -1;

constexpr int TR_FROM_UP = 1;
constexpr int TR_FROM_DN = 2;

constexpr int TRI_LHS = 1;
constexpr int TRI_RHS = 2;

class _Monotone {
  public:
    static int monotonateTrapezoids(int n);
    static int triangulateMonotonePolygons(int numberOfVertices,
                                           int numberOfMonotonePolygons,
                                           int op[][3]);

  private:
    static _MonotoneChainNode monotoneChainNodes[TRAPEZOID_TABLE_SIZE];
    static _VertexChain vertexChains[SEGMENT_SIZE];
    static int monotonePolygonEntryNode[SEGMENT_SIZE];
    static int visitedTrapezoids[TRAPEZOID_TABLE_SIZE];
    static int nextChainNodeIndex;
    static int nextOutputTriangleIndex;
    static int nextMonotonePolygonIndex;
    static int newMonotone();
    static int newChainElement();
    static double getAngle(Vector2Dd *basePoint, Vector2Dd *nextPoint,
                           Vector2Dd *otherPoint);
    static int getVertexPositions(int v0, int v1, int *ip, int *iq);
    static int makeNewMonotonePolygon(int mcur, int v0, int v1);
    static int traversePolygon(int mcur, int trnum, int from, int dir);
    static int triangulateSinglePolygon(int nvert, int posmax, int side,
                                        int op[][3]);
    static double crossSine(const Vector2Dd &firstVector,
                            const Vector2Dd &secondVector);
    static double length(const Vector2Dd &vector);
};

#endif
