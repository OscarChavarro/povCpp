//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "java/util/Collections.h"
#include "java/lang/Math.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "java/util/TreeMap.txx"
#include "java/util/TreeSet.txx"
#include <cfloat>

#include "java/lang/Double.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersectionCurveBuilder.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetIntersectionCurveBuilder Builder;

java::ArrayList<Builder::IndexList> Builder::lastTraversalNeighborPositions;

namespace {

/** Internal carrier for one face-pair point group. */
struct FacePairGroup {
    _PolyhedralBoundedSolidFace* faceA;
    _PolyhedralBoundedSolidFace* faceB;
    java::ArrayList<int> nodes;
};

bool containsValue(const java::ArrayList<int>& values, int value)
{
    return values.contains(value);
}

}

//= Report ==========================================================

Builder::Report::Report()
    : oddFacePairGroupCount(0), degenerateDirectionGroupCount(0), nodeCount(0)
{
}

bool Builder::Report::isCleanlyClosed() const
{
    size_t coveredByCycles;
    size_t i;

    if ( !openChains.isEmpty() || !isolatedNodes.isEmpty() ||
         !pinchNodes.isEmpty() || oddFacePairGroupCount > 0 ||
         degenerateDirectionGroupCount > 0 ) {
        return false;
    }
    coveredByCycles = 0;
    for ( i = 0; i < cycles.size(); i++ ) {
        coveredByCycles += cycles[i].size();
    }
    return coveredByCycles == (size_t)nodeCount;
}

java::String Builder::Report::summarize() const
{
    java::String sb;
    size_t i;

    sb += "curves: nodes=" + java::String::valueOf(nodeCount);
    sb += " cycles=" + java::String::valueOf(cycles.size()) + "[";
    for ( i = 0; i < cycles.size(); i++ ) {
        if ( i > 0 ) {
            sb += ",";
        }
        sb += java::String::valueOf(cycles[i].size());
    }
    sb += "] openChains=" + java::String::valueOf(openChains.size()) + "[";
    for ( i = 0; i < openChains.size(); i++ ) {
        if ( i > 0 ) {
            sb += ",";
        }
        sb += java::String::valueOf(openChains[i].size());
    }
    sb += "] isolated=" + java::String::valueOf(isolatedNodes.size());
    sb += " pinch=" + java::String::valueOf(pinchNodes.size());
    sb += " oddGroups=" + java::String::valueOf(oddFacePairGroupCount);
    sb += " degenerateGroups=" + java::String::valueOf(degenerateDirectionGroupCount);
    sb += java::String(" cleanlyClosed=") + (isCleanlyClosed() ? "true" : "false");
    return java::String(sb.c_str());
}

//= Helpers =========================================================

Builder::_PolyhedralBoundedSolidSetIntersectionCurveBuilder()
{
}

_PolyhedralBoundedSolidFace* Builder::halfEdgeFace(
    _PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == 0 || he->parentLoop == 0 ) {
        return 0;
    }
    return he->parentLoop->parentFace;
}

void Builder::collectFaces(const _PolyhedralBoundedSolidSetOperatorNullEdge& ne,
    java::ArrayList<_PolyhedralBoundedSolidFace*>& outFaces)
{
    _PolyhedralBoundedSolidFace* rightFace;
    _PolyhedralBoundedSolidFace* leftFace;

    outFaces.clear();
    if ( ne.e == 0 ) {
        return;
    }
    rightFace = halfEdgeFace(ne.e->rightHalf);
    leftFace = halfEdgeFace(ne.e->leftHalf);
    if ( rightFace != 0 ) {
        outFaces.add(rightFace);
    }
    if ( leftFace != 0 && leftFace != rightFace ) {
        outFaces.add(leftFace);
    }
}

bool Builder::nodePosition(const _PolyhedralBoundedSolidSetOperatorNullEdge& ne,
    Vector3Dd& outPosition)
{
    if ( ne.e == 0 || ne.e->rightHalf == 0 ||
         ne.e->rightHalf->startingVertex == 0 ) {
        return false;
    }
    outPosition = ne.e->rightHalf->startingVertex->position;
    return true;
}

bool Builder::faceNormal(_PolyhedralBoundedSolidFace* face, Vector3Dd& outNormal)
{
    InfinitePlane* plane;

    if ( face == 0 ) {
        return false;
    }
    plane = face->getContainingPlane();
    if ( plane == 0 ) {
        return false;
    }
    outNormal = plane->getNormal();
    delete plane;
    return true;
}

void Builder::link(java::ArrayList<IndexList>& neighbors, int i, int j)
{
    if ( i == j ) {
        return;
    }
    // As Java `LinkedHashSet`: insertion ordered, without repetitions
    if ( !containsValue(neighbors[i], j) ) {
        neighbors[i].add(j);
    }
    if ( !containsValue(neighbors[j], i) ) {
        neighbors[j].add(i);
    }
}

int Builder::minOf(const IndexList& values)
{
    int best;
    size_t i;

    best = values[0];
    for ( i = 1; i < values.size(); i++ ) {
        if ( values[i] < best ) {
            best = values[i];
        }
    }
    return best;
}

//= Curve report ====================================================

Builder::Report Builder::build(const NullEdgeList& sonea,
    const NullEdgeList& soneb,
    double unitVectorTolerance)
{
    Report report;
    int n;
    int k;
    int i;

    n = (int)java::Math::min(sonea.size(), soneb.size());
    if ( n == 0 ) {
        return report;
    }

    //-----------------------------------------------------------------
    // 1. Group nodes by (faceA, faceB) pair (insertion ordered, as Java
    //    `LinkedHashMap`).
    //-----------------------------------------------------------------
    java::ArrayList<FacePairGroup> groups;
    java::TreeMap<long long, size_t> groupIndex;
    java::ArrayList<_PolyhedralBoundedSolidFace*> facesA;
    java::ArrayList<_PolyhedralBoundedSolidFace*> facesB;

    for ( k = 0; k < n; k++ ) {
        collectFaces(sonea[k], facesA);
        collectFaces(soneb[k], facesB);
        for ( size_t a = 0; a < facesA.size(); a++ ) {
            _PolyhedralBoundedSolidFace* fa = facesA[a];
            for ( size_t b = 0; b < facesB.size(); b++ ) {
                _PolyhedralBoundedSolidFace* fb = facesB[b];
                long long key = (((long long)fa->id) << 32) ^
                    ((long long)fb->id & 0xffffffffLL);
                size_t* found = groupIndex.get(key);
                size_t index;
                if ( found == nullptr ) {
                    FacePairGroup group;
                    group.faceA = fa;
                    group.faceB = fb;
                    index = groups.size();
                    groups.add(group);
                    groupIndex.put(key, index);
                }
                else {
                    index = (*found);
                }
                groups[index].nodes.add(k);
            }
        }
    }

    //-----------------------------------------------------------------
    // 2. Derive curve adjacency from each group's chord structure.
    //-----------------------------------------------------------------
    java::ArrayList<IndexList> neighbors = java::Collections::nCopies<IndexList>((size_t)n, IndexList());

    for ( size_t g = 0; g < groups.size(); g++ ) {
        FacePairGroup& group = groups[g];
        int m = (int)group.nodes.size();
        if ( m < 2 ) {
            continue;
        }
        if ( m == 2 ) {
            link(neighbors, group.nodes[0], group.nodes[1]);
            continue;
        }

        Vector3Dd normalA;
        Vector3Dd normalB;
        Vector3Dd direction;
        bool hasDirection = false;
        if ( faceNormal(group.faceA, normalA) &&
             faceNormal(group.faceB, normalB) ) {
            direction = normalA.crossProduct(normalB);
            hasDirection = direction.length() > unitVectorTolerance;
        }
        if ( !hasDirection ) {
            // Parallel or degenerate planes: chord order along the
            // intersection line is undefined; report instead of guessing.
            report.degenerateDirectionGroupCount++;
            continue;
        }

        IndexList sorted = group.nodes;
        sorted.sort([&sonea, &direction](int a, int b) {
                Vector3Dd pa;
                Vector3Dd pb;
                if ( !nodePosition(sonea[a], pa) ||
                     !nodePosition(sonea[b], pb) ) {
                    return a < b;
                }
                int cmp = java::Double::compare(direction.dotProduct(pa),
                    direction.dotProduct(pb));
                if ( cmp != 0 ) {
                    return cmp < 0;
                }
                return a < b;
            });

        if ( (m % 2) != 0 ) {
            report.oddFacePairGroupCount++;
        }
        // Entry/exit parity along the intersection line: chord
        // endpoints pair as (0,1), (2,3), ... - linking consecutive
        // sorted points across chords would bridge separate curve
        // passes over the same face pair.
        for ( i = 0; i + 1 < m; i += 2 ) {
            link(neighbors, sorted[i], sorted[i + 1]);
        }
    }

    //-----------------------------------------------------------------
    // 3. Classify nodes and extract chains (from terminals) and cycles.
    //-----------------------------------------------------------------
    java::ArrayList<bool> visited = java::Collections::nCopies<bool>((size_t)n, false);

    for ( k = 0; k < n; k++ ) {
        size_t degree = neighbors[k].size();
        if ( degree == 0 ) {
            report.isolatedNodes.add(k);
            visited[k] = true;
        }
        else if ( degree > 2 ) {
            report.pinchNodes.add(k);
        }
    }

    // Chains: corridors of degree-2 nodes hanging off terminal nodes
    // (degree 1 or degree > 2). Terminal-terminal direct links are
    // deduplicated with an edge-visited set.
    java::TreeSet<long long> walkedTerminalLinks;
    for ( k = 0; k < n; k++ ) {
        size_t degree = neighbors[k].size();
        if ( degree == 2 || degree == 0 ) {
            continue;
        }
        for ( size_t t = 0; t < neighbors[k].size(); t++ ) {
            int nb = neighbors[k][t];
            size_t nbDegree = neighbors[nb].size();
            if ( nbDegree != 2 ) {
                long long a = java::Math::min(k, nb);
                long long b = java::Math::max(k, nb);
                long long linkKey = (a << 32) | b;
                if ( walkedTerminalLinks.add(linkKey) ) {
                    IndexList chain;
                    chain.add(k);
                    chain.add(nb);
                    report.openChains.add(chain);
                }
                continue;
            }
            if ( visited[nb] ) {
                continue;
            }
            IndexList path;
            path.add(k);
            int prev = k;
            int cur = nb;
            while ( neighbors[cur].size() == 2 && !visited[cur] ) {
                visited[cur] = true;
                path.add(cur);
                int next = -1;
                for ( size_t c = 0; c < neighbors[cur].size(); c++ ) {
                    if ( neighbors[cur][c] != prev ) {
                        next = neighbors[cur][c];
                        break;
                    }
                }
                if ( next < 0 ) {
                    break;
                }
                prev = cur;
                cur = next;
            }
            if ( cur != prev && !containsValue(path, cur) ) {
                path.add(cur);
            }
            report.openChains.add(path);
        }
    }

    // Cycles: remaining unvisited degree-2 components are pure cycles
    // (every corridor touching a terminal was consumed above).
    for ( k = 0; k < n; k++ ) {
        if ( visited[k] || neighbors[k].size() != 2 ) {
            continue;
        }
        IndexList path;
        path.add(k);
        visited[k] = true;
        int prev = k;
        int cur = neighbors[k][0];
        bool closed = true;
        while ( cur != k ) {
            if ( visited[cur] || neighbors[cur].size() != 2 ) {
                closed = false;
                break;
            }
            visited[cur] = true;
            path.add(cur);
            int next = -1;
            for ( size_t c = 0; c < neighbors[cur].size(); c++ ) {
                if ( neighbors[cur][c] != prev ) {
                    next = neighbors[cur][c];
                    break;
                }
            }
            if ( next < 0 ) {
                closed = false;
                break;
            }
            prev = cur;
            cur = next;
        }
        if ( closed ) {
            report.cycles.add(path);
        }
        else {
            report.openChains.add(path);
        }
    }

    report.nodeCount = n;
    return report;
}

//= Traversal order =================================================

bool Builder::computeTraversalOrder(const java::ArrayList<IndexList>& cycles,
    int nodeCount,
    IndexList& outPermutation)
{
    int covered;
    size_t i;
    size_t j;

    if ( nodeCount <= 0 ) {
        return false;
    }
    java::ArrayList<bool> seen = java::Collections::nCopies<bool>((size_t)nodeCount, false);
    covered = 0;
    for ( i = 0; i < cycles.size(); i++ ) {
        const IndexList& cycle = cycles[i];
        for ( j = 0; j < cycle.size(); j++ ) {
            if ( cycle[j] < 0 || cycle[j] >= nodeCount || seen[cycle[j]] ) {
                return false;
            }
            seen[cycle[j]] = true;
            covered++;
        }
    }
    if ( covered != nodeCount ) {
        return false;
    }

    java::ArrayList<IndexList> ordered = cycles;
    ordered.sort([](const IndexList& a, const IndexList& b) {
            return minOf(a) < minOf(b);
        });

    outPermutation = java::Collections::nCopies<int>(nodeCount, 0);
    int position = 0;
    for ( i = 0; i < ordered.size(); i++ ) {
        const IndexList& cycle = ordered[i];
        size_t len = cycle.size();
        size_t startPos = 0;
        for ( j = 1; j < len; j++ ) {
            if ( cycle[j] < cycle[startPos] ) {
                startPos = j;
            }
        }
        for ( j = 0; j < len; j++ ) {
            outPermutation[position] = cycle[(startPos + j) % len];
            position++;
        }
    }
    return true;
}

bool Builder::orderAndOrientAlongCurves(const java::ArrayList<IndexList>& cycles,
    int nodeCount,
    NullEdgeList& sonea,
    NullEdgeList& soneb,
    IndexList& outPermutation)
{
    size_t c;
    size_t j;
    IndexList validation;

    lastTraversalNeighborPositions.clear();
    if ( !computeTraversalOrder(cycles, nodeCount, validation) ) {
        return false;
    }

    java::ArrayList<IndexList> ordered = cycles;
    ordered.sort([](const IndexList& a, const IndexList& b) {
            return minOf(a) < minOf(b);
        });

    java::ArrayList<IndexList> directedCycles;
    for ( c = 0; c < ordered.size(); c++ ) {
        const IndexList& storedCycle = ordered[c];
        size_t len = storedCycle.size();

        // Direction vote: count two-face struts whose current vertex-id
        // orientation already satisfies the scanjoin role rule in the
        // stored direction. Reversing the cycle inverts every
        // unambiguous vote, so a single count decides the direction.
        int agree = 0;
        int disagree = 0;
        for ( j = 0; j < len; j++ ) {
            int current = storedCycle[j];
            int successor = storedCycle[(j + 1) % len];
            int sharedIsRightA = successorSharedHalfIsRight(
                sonea[current], sonea[successor]);
            if ( sharedIsRightA >= 0 ) {
                // A-side rule: half toward successor must be LEFT.
                if ( sharedIsRightA == 0 ) {
                    agree++;
                }
                else {
                    disagree++;
                }
            }
            int sharedIsRightB = successorSharedHalfIsRight(
                soneb[current], soneb[successor]);
            if ( sharedIsRightB >= 0 ) {
                // B-side rule: half toward successor must be RIGHT.
                if ( sharedIsRightB == 1 ) {
                    agree++;
                }
                else {
                    disagree++;
                }
            }
        }
        IndexList directedCycle;
        if ( disagree > agree ) {
            for ( j = 0; j < len; j++ ) {
                directedCycle.add(storedCycle[(len - j) % len]);
            }
        }
        else {
            directedCycle = storedCycle;
        }

        // Orient the disagreeing minority along the chosen direction.
        for ( j = 0; j < len; j++ ) {
            int current = directedCycle[j];
            int successor = directedCycle[(j + 1) % len];
            orientTowardSuccessor(sonea[current], sonea[successor], false);
            orientTowardSuccessor(soneb[current], soneb[successor], true);
        }

        // Rotation: the first cycle starts at its minimum member index;
        // later cycles start at the node geometrically closest to the
        // first cycle's start, so the interleaving below begins in phase.
        size_t startPos = 0;
        if ( directedCycles.isEmpty() ) {
            for ( j = 1; j < len; j++ ) {
                if ( directedCycle[j] < directedCycle[startPos] ) {
                    startPos = j;
                }
            }
        }
        else {
            Vector3Dd anchor;
            bool hasAnchor = nodePosition(sonea[directedCycles[0][0]], anchor);
            double bestDistance = DBL_MAX;
            for ( j = 0; j < len; j++ ) {
                Vector3Dd p;
                if ( !hasAnchor || !nodePosition(sonea[directedCycle[j]], p) ) {
                    continue;
                }
                double d = p.subtract(anchor).length();
                if ( d < bestDistance ) {
                    bestDistance = d;
                    startPos = j;
                }
            }
        }
        IndexList rotated = java::Collections::nCopies<int>((long)len, 0);
        for ( j = 0; j < len; j++ ) {
            rotated[j] = directedCycle[(startPos + j) % len];
        }
        directedCycles.add(rotated);
    }

    // Interleave the cycles round-robin so parallel curves advance
    // together. A geometric-proximity merge was tried here and regressed
    // four star motifs: round-robin with phase-aligned starts is the
    // deterministic pacing that preserves every case the legacy emission
    // order handled.
    outPermutation = java::Collections::nCopies<int>(nodeCount, 0);
    int position = 0;
    size_t round = 0;
    while ( position < nodeCount ) {
        for ( c = 0; c < directedCycles.size(); c++ ) {
            const IndexList& cycle = directedCycles[c];
            if ( round < cycle.size() ) {
                outPermutation[position] = cycle[round];
                position++;
            }
        }
        round++;
    }

    // Export curve-junction adjacency in processing-position space.
    IndexList positionOf = java::Collections::nCopies<int>(nodeCount, 0);
    for ( j = 0; j < (size_t)nodeCount; j++ ) {
        positionOf[outPermutation[j]] = (int)j;
    }
    java::ArrayList<IndexList> neighborPositions = java::Collections::nCopies<IndexList>((size_t)nodeCount, IndexList());
    for ( c = 0; c < directedCycles.size(); c++ ) {
        const IndexList& cycle = directedCycles[c];
        size_t len = cycle.size();
        for ( j = 0; j < len; j++ ) {
            int original = cycle[j];
            int previousOriginal = cycle[(j + len - 1) % len];
            int nextOriginal = cycle[(j + 1) % len];
            IndexList partners;
            partners.add(positionOf[previousOriginal]);
            partners.add(positionOf[nextOriginal]);
            neighborPositions[positionOf[original]] = partners;
        }
    }
    lastTraversalNeighborPositions = neighborPositions;
    return true;
}

int Builder::successorSharedHalfIsRight(
    const _PolyhedralBoundedSolidSetOperatorNullEdge& current,
    const _PolyhedralBoundedSolidSetOperatorNullEdge& successor)
{
    _PolyhedralBoundedSolidFace* currentRightFace;
    _PolyhedralBoundedSolidFace* currentLeftFace;

    if ( current.e == 0 || successor.e == 0 ) {
        return -1;
    }
    currentRightFace = halfEdgeFace(current.e->rightHalf);
    currentLeftFace = halfEdgeFace(current.e->leftHalf);
    if ( currentRightFace == 0 || currentLeftFace == 0 ||
         currentRightFace == currentLeftFace ) {
        return -1;
    }
    _PolyhedralBoundedSolidFace* successorRightFace =
        halfEdgeFace(successor.e->rightHalf);
    _PolyhedralBoundedSolidFace* successorLeftFace =
        halfEdgeFace(successor.e->leftHalf);
    bool rightShared = currentRightFace == successorRightFace ||
        currentRightFace == successorLeftFace;
    bool leftShared = currentLeftFace == successorRightFace ||
        currentLeftFace == successorLeftFace;
    if ( rightShared == leftShared ) {
        return -1;
    }
    return rightShared ? 1 : 0;
}

void Builder::orientTowardSuccessor(
    _PolyhedralBoundedSolidSetOperatorNullEdge& current,
    const _PolyhedralBoundedSolidSetOperatorNullEdge& successor,
    bool successorSideIsRight)
{
    int sharedIsRight = successorSharedHalfIsRight(current, successor);

    if ( sharedIsRight < 0 ) {
        return;
    }
    if ( (sharedIsRight == 1) != successorSideIsRight ) {
        _PolyhedralBoundedSolidHalfEdge* tmp = current.e->rightHalf;
        current.e->rightHalf = current.e->leftHalf;
        current.e->leftHalf = tmp;
    }
}
