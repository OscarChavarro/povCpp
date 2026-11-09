#include "java/lang/String.h"
#include <cmath>
#include <cstdio>

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/slicing/_PolyhedralBoundedSolidSplitter.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

//= _PolyhedralBoundedSolidSplitterSectorClassification ===================

_PolyhedralBoundedSolidSplitterSectorClassification::
_PolyhedralBoundedSolidSplitterSectorClassification()
    : sector(nullptr), cl(ON), isWide(false), situation(UNDEFINED)
{
}

java::String _PolyhedralBoundedSolidSplitterSectorClassification::toString() const
{
    java::String msg = "{";
    msg += sector->toString().c_str();
    switch ( cl ) {
      case ABOVE: msg += " ABOVE"; break;
      case BELOW: msg += " BELOW"; break;
      case ON: msg += " ON"; break;
      default: msg += "<INVALID!>"; break;
    }
    if ( isWide ) {
        msg += " (W) ";
    }

    switch ( situation ) {
      case COPLANAR_FACE: msg += "<COPLANAR_FACE>"; break;
      case INPLANE_EDGE: msg += "<INPLANE_EDGE>"; break;
      case CROSSING_EDGE: msg += "<CROSSING_EDGE>"; break;
      default: msg += "<UNDEFINED>"; break;
    }

    msg += "}";
    return java::String(msg.c_str());
}

//= _PolyhedralBoundedSolidSplitterNullEdge ===============================

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
    _PolyhedralBoundedSolidSplitterNullEdge::nullEdgeNumericContext =
    PolyhedralBoundedSolidNumericPolicy::defaultContext();

_PolyhedralBoundedSolidSplitterNullEdge::_PolyhedralBoundedSolidSplitterNullEdge(
    _PolyhedralBoundedSolidEdge* e)
    : e(e)
{
}

void _PolyhedralBoundedSolidSplitterNullEdge::setNumericContext(
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext* context)
{
    if ( context == nullptr ) {
        nullEdgeNumericContext = PolyhedralBoundedSolidNumericPolicy::defaultContext();
    }
    else {
        nullEdgeNumericContext = *context;
    }
}

int _PolyhedralBoundedSolidSplitterNullEdge::compareTo(
    const _PolyhedralBoundedSolidSplitterNullEdge& other) const
{
    Vector3Dd a;
    Vector3Dd b;

    a = this->e->rightHalf->startingVertex->position;
    b = other.e->rightHalf->startingVertex->position;

    if ( PolyhedralBoundedSolidNumericPolicy
        ::compare(a.x(), b.x(), nullEdgeNumericContext.bigEpsilon()) != 0 ) {
        if ( a.x() < b.x() ) {
            return -1;
        }
        return 1;
    }
    else {
        if ( PolyhedralBoundedSolidNumericPolicy
            ::compare(a.y(), b.y(), nullEdgeNumericContext.bigEpsilon()) != 0 ) {
            if ( a.y() < b.y() ) {
                return -1;
            }
            return 1;
        }
        else {
            if ( a.z() < b.z() ) {
                return -1;
            }
            return 1;
        }
    }
}

//= _PolyhedralBoundedSolidSplitter =======================================

java::ArrayList<_PolyhedralBoundedSolidVertex*> _PolyhedralBoundedSolidSplitter::soov;
java::ArrayList<_PolyhedralBoundedSolidSplitterNullEdge> _PolyhedralBoundedSolidSplitter::sone;
java::ArrayList<_PolyhedralBoundedSolidFace*> _PolyhedralBoundedSolidSplitter::sonf;
java::ArrayList<_PolyhedralBoundedSolidFace*> _PolyhedralBoundedSolidSplitter::facesToFixAbove;
java::ArrayList<_PolyhedralBoundedSolidFace*> _PolyhedralBoundedSolidSplitter::facesToFixBelow;
java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> _PolyhedralBoundedSolidSplitter::ends;
java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> _PolyhedralBoundedSolidSplitter::tieds;

int _PolyhedralBoundedSolidSplitter::compareToZero(double value)
{
    return PolyhedralBoundedSolidNumericPolicy::compareToZero(value,
        numericContext);
}

/**
Implements function `addsoov` from section [MANT1988].14.4. and program
[MANT1988].14.2.
*/
void _PolyhedralBoundedSolidSplitter::addsoov(_PolyhedralBoundedSolidVertex* v)
{
    size_t i;

    for ( i = 0; i < soov.size(); i++ ) {
        if ( soov[i] == v ) {
            return;
        }
    }
    soov.add(v);
}

/**
Implements solid splitting reduction step as indicated on sections
[MANT1988].14.2.1 and [MANT1988].14.4 and program [MANT1988].14.2.

This method is responsible for generating the set of coplanar
vertices of `inSolid` (with respect to `inSplittingPlane`) and store
them on `soov` for later usage.

This method subdivides all edges of `inSolid` that intersects
`inSplittingPlane` at their intersection points.
*/
void _PolyhedralBoundedSolidSplitter::splitGenerate(
    PolyhedralBoundedSolid* inSolid, const InfinitePlane& inSplittingPlane)
{
    _PolyhedralBoundedSolidEdge* e;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;
    Vector3Dd p;
    double d1;
    double d2;
    double t;
    int s1;
    int s2;
    long i;

    soov.clear();
    for ( i = 0; i < inSolid->getEdgesList().size(); i++ ) {
        e = inSolid->getEdgesList().get(i);
        v1 = e->rightHalf->startingVertex;
        v2 = e->leftHalf->startingVertex;
        d1 = inSplittingPlane.pointDistance(v1->position);
        d2 = inSplittingPlane.pointDistance(v2->position);
        s1 = compareToZero(d1);
        s2 = compareToZero(d2);
        if ( (s1 == -1 && s2 == 1) || (s1 == 1 && s2 == -1) ) {
            t = d1 / (d1 - d2);
            p = v1->position.add((v2->position.subtract(v1->position)).multiply(t));
            he = e->leftHalf->next();
            PolyhedralBoundedSolidEulerOperators::lmev(inSolid, e->rightHalf, he,
                inSolid->getMaxVertexId()+1, p);
            addsoov(he->previous()->startingVertex);
        }
        else {
            if ( s1 == 0 ) {
                addsoov(v1);
            }
            if ( s2 == 0 ) {
                addsoov(v2);
            }
        }
    }
}

/**
Current method is the first step for the initial classification of vertex
neighborhood for `vtx`, as indicated on section [MANT1988].14.5.2. and
program [MANT1988].14.4.

Vitral SDK's implementation of this procedure extends the original from
[MANT1988] by adding extra information flags to sector classifications
`.isWide`, `.position` and `.situation`. Those flags are an additional
aid for debugging purposes and specifically the `situation` flag will be
later used on `splitClassify` to correct the ordering of sectors in order
to keep consistency with Vitral SDK's interpretation of coordinate system.
*/
java::ArrayList<_PolyhedralBoundedSolidSplitterSectorClassification>
_PolyhedralBoundedSolidSplitter::getNeighborhood(
    _PolyhedralBoundedSolidVertex* vtx, const InfinitePlane& inSplittingPlane)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    Vector3Dd bisect;
    double d;
    java::ArrayList<SectorClassification> neighborSectorsInfo;

    he = vtx->emanatingHalfEdge;

    do {
        SectorClassification c;
        c.sector = he;
        d = inSplittingPlane.pointDistance((he->next())->startingVertex->position);
        c.cl = compareToZero(d);
        c.isWide = false;
        c.position = (he->next())->startingVertex->position;
        c.situation = SectorClassification::UNDEFINED;
        if ( checkSplitterSectorWideness(he) ) {
            bisect = bisector(he);
            c.situation = SectorClassification::CROSSING_EDGE;
            neighborSectorsInfo.add(c);

            SectorClassification wide;
            wide.sector = he;
            d = inSplittingPlane.pointDistance(bisect);
            wide.cl = compareToZero(d);
            wide.isWide = true;
            wide.position = bisect;
            wide.situation = SectorClassification::CROSSING_EDGE;
            neighborSectorsInfo.add(wide);
        }
        else {
            neighborSectorsInfo.add(c);
        }
        he = (he->mirrorHalfEdge())->next();
    } while ( he != vtx->emanatingHalfEdge );

    //-----------------------------------------------------------------
    // Extra pass, not from original [MANT1988] code
    size_t i;

    for ( i = 0; i < neighborSectorsInfo.size(); i++ ) {
        SectorClassification& c = neighborSectorsInfo[i];
        if ( c.cl == SectorClassification::ON &&
             c.situation == SectorClassification::UNDEFINED ) {
            c.situation = SectorClassification::INPLANE_EDGE;
        }
    }

    return neighborSectorsInfo;
}

/**
The splitter needs the oriented interior angle from [MANT1988].14.5.2.
Boolean set operations intentionally use the legacy cross-product
predicate inherited from `_PolyhedralBoundedSolidOperator`; that
predicate treats degenerate/straight sectors as wide and is not
interchangeable with the splitter classification.
*/
bool _PolyhedralBoundedSolidSplitter::checkSplitterSectorWideness(
    _PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == nullptr || he->parentLoop == nullptr ||
         he->parentLoop->parentFace == nullptr ||
         he->parentLoop->parentFace->getContainingPlane() == nullptr ||
         he->previous() == nullptr || he->next() == nullptr ) {
        return true;
    }

    Vector3Dd vertex = he->startingVertex->position;
    Vector3Dd incoming = vertex.subtract(
        he->previous()->startingVertex->position);
    Vector3Dd outgoing = he->next()->startingVertex->position.subtract(vertex);
    Vector3Dd faceNormal =
        he->parentLoop->parentFace->getContainingPlane()->getNormal();

    if ( incoming.length() <= numericContext.unitVectorTolerance() ||
         outgoing.length() <= numericContext.unitVectorTolerance() ||
         faceNormal.length() <= numericContext.unitVectorTolerance() ) {
        return true;
    }

    incoming = incoming.normalized();
    outgoing = outgoing.normalized();
    faceNormal = faceNormal.normalized();

    double signedTurn = std::atan2(
        faceNormal.dotProduct(incoming.crossProduct(outgoing)),
        incoming.dotProduct(outgoing));
    double interiorAngle = signedTurn < 0.0
        ? 2.0 * M_PI + signedTurn
        : signedTurn;

    return interiorAngle > M_PI + numericContext.angleTolerance();
}

bool _PolyhedralBoundedSolidSplitter::inplaneEdgesOn(
    const java::ArrayList<SectorClassification>& nbr)
{
    size_t i;

    for ( i = 0; i < nbr.size(); i++ ) {
        if ( nbr[i].situation == SectorClassification::INPLANE_EDGE ) return true;
    }
    return false;
}

/**
Current method applies the first reclassification rule presented at
sections [MANT1988].14.5.1 and [MANT1988].14.5.2:
For the given vertex neigborhood, classify each edge according to whether
its final vertex lies above, on or below the `inSplittingPlane`. Tag
the edge with the corresponding label ABOVE, ON or BELOW.
Following program [MANT1988].14.5.
*/
void _PolyhedralBoundedSolidSplitter::reclassifyOnSectors(
    java::ArrayList<SectorClassification>& nbr,
    const InfinitePlane& inSplittingPlane)
{
    _PolyhedralBoundedSolidFace* f;
    Vector3Dd c;
    double d;
    size_t i;

    for ( i = 0; i < nbr.size(); i++ ) {
        SectorClassification& l = nbr[i];
        f = l.sector->parentLoop->parentFace;
        c = f->getContainingPlane()->getNormal().crossProduct(inSplittingPlane.getNormal());
        d = c.dotProduct(c);
        if ( compareToZero(d) == 0 ) {
            // Entering this means "faces are coplanar"
            d = f->getContainingPlane()->getNormal().dotProduct(inSplittingPlane.getNormal());
            if ( compareToZero(d) == 1 ) {
                l.cl = SectorClassification::BELOW;
                l.situation = SectorClassification::COPLANAR_FACE;
                nbr[(i+1)%nbr.size()].cl = SectorClassification::BELOW;
            }
            else {
                l.cl = SectorClassification::ABOVE;
                l.situation = SectorClassification::COPLANAR_FACE;
                nbr[(i+1)%nbr.size()].cl = SectorClassification::ABOVE;
            }
        }
    }
}

/**
Current method applies the second reclassification rule presented at
sections [MANT1988].14.5.1 and [MANT1988].14.5.2:
After applying the first rule on method `reclassifyOnSectors`, ON edges
may appear in only four kinds of consecutive arrangements. For each
of the following arrangements, ON edge is reclassified as ABOVE or BELOW:
  - Sequence ABOVE/ON/ABOVE -> reclassified as BELOW
  - Sequence ABOVE/ON/BELOW -> reclassified as BELOW
  - Sequence BELOW/ON/BELOW -> reclassified as ABOVE
  - Sequence BELOW/ON/ABOVE -> reclassified as BELOW
Those 4 rules are designed so that nonmanifold results will be represented
as disconnected models.
Following program [MANT1988].14.6.
*/
void _PolyhedralBoundedSolidSplitter::reclassifyOnEdges(
    java::ArrayList<SectorClassification>& nbr)
{
    size_t i;
    size_t n = nbr.size();

    for ( i = 0; i < n; i++ ) {
        if ( nbr[i].cl == SectorClassification::ON ) {
            if ( nbr[(n+i-1) % n].cl == SectorClassification::BELOW ) {
                if ( nbr[(i+1) % n].cl == SectorClassification::BELOW ) {
                    nbr[i].cl = SectorClassification::ABOVE;
                }
                else {
                    nbr[i].cl = SectorClassification::BELOW;
                }
            }
            else {
                nbr[i].cl = SectorClassification::BELOW;
            }
        }
    }
}

/**
Following section [MANT1988].14,6,2 and program [MANT1988].14.7.
Note, this code is horrible! YUCK! :P

With respect to the original algorithm from [MANT1988], current
implementation adds an extra check to ensure the orientation of the
edges from below to above.
*/
void _PolyhedralBoundedSolidSplitter::insertNullEdges(
    java::ArrayList<SectorClassification>& nbr,
    PolyhedralBoundedSolid* inSolid, const InfinitePlane& inSplittingPlane)
{
    size_t start;
    size_t i;
    _PolyhedralBoundedSolidHalfEdge* head;
    _PolyhedralBoundedSolidHalfEdge* tail;
    size_t nnbr = nbr.size();

    if ( nnbr <= 0 ) return;

    //- Locate the head of an ABOVE-sequence --------------------------
    i = 0;
    while ( !( nbr[i].cl == SectorClassification::BELOW &&
               nbr[(i+1)%nnbr].cl == SectorClassification::ABOVE )  ) {
        i++;
        if ( i >= nnbr ) {
            return;
        }
    }
    start = i;
    head = nbr[i].sector;

    //-----------------------------------------------------------------
    while ( true ) {
        //- Locate the final sector of the sequence ------------------
        while ( !( nbr[i].cl == SectorClassification::ABOVE &&
                   nbr[(i+1)%nnbr].cl == SectorClassification::BELOW ) ) {
            i = (i+1) % nnbr;
        }
        tail = nbr[i].sector;

        //- Insert null edge -----------------------------------------
        int d1;
        d1 = const_cast<InfinitePlane&>(inSplittingPlane).doContainmentTestHalfSpace(
            head->next()->startingVertex->position,
            numericContext.epsilon());

        if ( d1 != Geometry::OUTSIDE ) {
            PolyhedralBoundedSolidEulerOperators::lmev(inSolid, tail, head,
                inSolid->getMaxVertexId()+1, head->startingVertex->position);
            sone.add(_PolyhedralBoundedSolidSplitterNullEdge(
                tail->previous()->parentEdge));
        }
        else {
            PolyhedralBoundedSolidEulerOperators::lmev(inSolid, head, tail,
                inSolid->getMaxVertexId()+1, head->startingVertex->position);
            sone.add(_PolyhedralBoundedSolidSplitterNullEdge(
                head->previous()->parentEdge));
        }

        //- Locate the start of the next sequence --------------------
        while ( !( nbr[i].cl == SectorClassification::BELOW &&
                   nbr[(i+1) % nnbr].cl == SectorClassification::ABOVE ) ) {
            i = (i+1) % nnbr;
            if ( i == start ) {
                return;
            }
        }
    }
}

/**
Vertex neighborhood classifier, as presented in section [MANT1988].14.5,
and program 14.3.

It appears that original algorithm from [MANT1988] assumes a left handed
geometry or orientation or other difference to current Vitral SDK
implementation of the boundary representation. That difference implies
a reverse order in some cases, so `inplaneEdgesOn` check is added here
to keep current implementation's consistency.
*/
void _PolyhedralBoundedSolidSplitter::splitClassify(
    PolyhedralBoundedSolid* inSolid, const InfinitePlane& inSplittingPlane)
{
    size_t i;

    sone.clear();

    /// Following variable `nbr` from program [MANT1988].14.3.
    java::ArrayList<SectorClassification> nbr;

    for ( i = 0; i < soov.size(); i++ ) {
        nbr = getNeighborhood(soov[i], inSplittingPlane);
        if ( inplaneEdgesOn(nbr) ) {
            java::Collections::reverse(nbr);
        }
        reclassifyOnSectors(nbr, inSplittingPlane);
        reclassifyOnEdges(nbr);
        insertNullEdges(nbr, inSolid, inSplittingPlane);
    }
}

/**
Following section [MANT1988].14.7.2. and program [MANT1988].14.9.
*/
_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidSplitter::canJoin(
    _PolyhedralBoundedSolidHalfEdge* he)
{
    _PolyhedralBoundedSolidHalfEdge* ret;
    size_t i;

    for ( i = 0; i < ends.size(); i++ ) {
        if ( neighbor(he, ends[i]) ) {
            ret = ends[i];
            ends.remove((long)i);
            tieds.add(ret);
            return ret;
        }
    }
    ends.add(he);
    return nullptr;
}

void _PolyhedralBoundedSolidSplitter::printNbr(
    const java::ArrayList<SectorClassification>& neighborSectorsInfo)
{
    size_t i;

    for ( i = 0; i < neighborSectorsInfo.size(); i++ ) {
        printf("  - %s\n", neighborSectorsInfo[i].toString().c_str());
    }
}

void _PolyhedralBoundedSolidSplitter::printEnds()
{
    size_t i;

    for ( i = 0; i < ends.size(); i++ ) {
        printf("  - ends[%zu]: %s\n", i, ends[i]->toString().c_str());
    }
}

/**
Following section [MANT1988].14.7.2. and program [MANT1988].14.10.
*/
void _PolyhedralBoundedSolidSplitter::cut(_PolyhedralBoundedSolidHalfEdge* he)
{
    PolyhedralBoundedSolid* s;

    s = he->parentLoop->parentFace->parentSolid;

    if ( he->parentEdge->rightHalf->parentLoop ==
         he->parentEdge->leftHalf->parentLoop ) {
        sonf.add(he->parentLoop->parentFace);
        PolyhedralBoundedSolidEulerOperators::lkemr(s, he->parentEdge->rightHalf,
            he->parentEdge->leftHalf);
    }
    else {
        PolyhedralBoundedSolidEulerOperators::lkef(s, he->parentEdge->rightHalf,
            he->parentEdge->leftHalf);
    }
}

bool _PolyhedralBoundedSolidSplitter::isLoose(_PolyhedralBoundedSolidHalfEdge* he)
{
    size_t i;

    for ( i = 0; i < tieds.size(); i++ ) {
        if ( he == tieds[i] ) return false;
    }

    return true;
}

/**
Following section [MANT1988].14.7.2. and program [MANT1988].14.9.
*/
void _PolyhedralBoundedSolidSplitter::splitConnect()
{
    size_t i;

    ends.clear();
    tieds.clear();

    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidEdge* nextedge;
    _PolyhedralBoundedSolidHalfEdge* h1;
    _PolyhedralBoundedSolidHalfEdge* h2;

    sonf.clear();

    // As Java `Collections.sort`: stable, with the order of `compareTo`
    sone.sort([](const _PolyhedralBoundedSolidSplitterNullEdge& a,
           const _PolyhedralBoundedSolidSplitterNullEdge& b) {
            return a.compareTo(b) < 0;
        });

    for ( i = 0; i < sone.size(); i++ ) {
        nextedge = sone[i].e;
        h1 = canJoin(nextedge->rightHalf);

        if ( h1 != nullptr ) {
            join(h1, nextedge->rightHalf, false);
            tieds.add(nextedge->rightHalf);
            if ( !isLoose(h1->mirrorHalfEdge()) ) {
                cut(h1);
            }
        }
        h2 = canJoin(nextedge->leftHalf);

        if ( h2 != nullptr ) {
            join(h2, nextedge->leftHalf, false);
            tieds.add(nextedge->leftHalf);
            if ( !isLoose(h2->mirrorHalfEdge()) ) {
                cut(h2);
            }
        }
        if ( h1 != nullptr && h2 != nullptr ) {
            cut(nextedge->rightHalf);
        }
    }
}

/**
Following section [MANT1988].14.8. and program [MANT1988].14.12.
*/
void _PolyhedralBoundedSolidSplitter::classify(PolyhedralBoundedSolid* /*s*/,
                                               PolyhedralBoundedSolid* above,
                                               PolyhedralBoundedSolid* below)
{
    size_t i;
    facesToFixAbove.clear();
    facesToFixBelow.clear();

    for ( i = 0; i < sonf.size()/2; i++ ) {
        movefac(sonf[i], above);
        facesToFixAbove.add(sonf[i]);
        movefac(sonf[i+sonf.size()/2], below);
        facesToFixBelow.add(sonf[i+sonf.size()/2]);
    }
}

bool _PolyhedralBoundedSolidSplitter::isNullFace(_PolyhedralBoundedSolidFace* f)
{
    size_t i;

    for ( i = 0; i < sonf.size(); i++ ) {
        if ( sonf[i] == f ) return true;
    }
    return false;
}

/**
C++ port note: Java replaces the lists of the solid with new empty ones;
here they are emptied without deleting their nodes, which now belong to
the solids built by the split.
*/
void _PolyhedralBoundedSolidSplitter::destroy(PolyhedralBoundedSolid* inSolid)
{
    inSolid->getPolygonsList().clear();
    inSolid->getEdgesList().clear();
    inSolid->getVerticesList().clear();
}

/**
A face `a` is "inside" other `b` (and should be an internal loop) if
all vertices from `b` are inside the polygon of `a`.

PRE: given faces are coplanar.
*/
bool _PolyhedralBoundedSolidSplitter::faceInsideFace(
    _PolyhedralBoundedSolidFace* a, _PolyhedralBoundedSolidFace* b)
{
    long i;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* heStart;

    for ( i = 0; i < b->boundariesList.size(); i++ ) {
        heStart = b->boundariesList.get(i)->boundaryStartHalfEdge;
        he = heStart;
        do {
            if ( a->testPointInside(he->startingVertex->position,
                    numericContext.bigEpsilon()) == Geometry::OUTSIDE ) {
                return false;
            }
            he = he->next();
        } while ( he != heStart );
    }
    return true;
}

void _PolyhedralBoundedSolidSplitter::fixNullFaces(
    java::ArrayList<_PolyhedralBoundedSolidFace*>& l)
{
    if ( l.size() == 1 ) {
        l.remove(0L);
        return;
    }

    size_t i;
    size_t j;

    for ( i = 0; i < l.size(); i++ ) {
        for ( j = 0; j < l.size(); j++ ) {
            if ( i == j ) continue;
            if ( faceInsideFace(l[j], l[i]) ) {
                PolyhedralBoundedSolidEulerOperators::lkfmrh(
                    l[i]->parentSolid, l[i], l[j]);
                l.remove((long)j);
                // Repeat he process with remaining list
                fixNullFaces(l);
                return;
            }
        }
    }
}

/**
Following section [MANT1988].14.8. and program [MANT1988].14.11.
*/
void _PolyhedralBoundedSolidSplitter::splitFinish(
    PolyhedralBoundedSolid* inSolid,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow)
{
    size_t i;
    size_t firstHalfSize;
    _PolyhedralBoundedSolidFace* newface;
    PolyhedralBoundedSolid* newAbove;
    PolyhedralBoundedSolid* newBelow;

    firstHalfSize = sonf.size();
    for ( i = 0; i < firstHalfSize; i++ ) {
        newface = PolyhedralBoundedSolidEulerOperators::lmfkrh(inSolid,
            sonf[i]->boundariesList.get(1), inSolid->getMaxFaceId()+1);
        sonf.add(newface);
    }
    newAbove = new PolyhedralBoundedSolid();
    newBelow = new PolyhedralBoundedSolid();
    classify(inSolid, newAbove, newBelow);
    fixNullFaces(facesToFixAbove);
    fixNullFaces(facesToFixBelow);
    cleanup(newAbove);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(newAbove);
    cleanup(newBelow);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(newBelow);
    outSolidsAbove.add(newAbove);
    outSolidsBelow.add(newBelow);
    destroy(inSolid);
}

void _PolyhedralBoundedSolidSplitter::split(
    PolyhedralBoundedSolid* inSolid,
    const InfinitePlane& inSplittingPlane,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
    java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow)
{
    PolyhedralBoundedSolidStatistics::recordSplitCall();
    int aboveBefore = (int)outSolidsAbove.size();
    int belowBefore = (int)outSolidsBelow.size();
    //-----------------------------------------------------------------
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolid(inSolid);
    setNumericContext(&context);
    _PolyhedralBoundedSolidSplitterNullEdge::setNumericContext(&numericContext);

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(inSolid);
    splitGenerate(inSolid, inSplittingPlane);
    splitClassify(inSolid, inSplittingPlane);

    if ( sone.size() <= 0 ) {
        // Plane should be tested here before asuming this order!
        PolyhedralBoundedSolidStatistics::recordSplitNoNullEdgesCase();
        outSolidsAbove.add(inSolid);
        outSolidsBelow.add(new PolyhedralBoundedSolid());
        PolyhedralBoundedSolidStatistics::recordSplitProducedSolids(
            (int)outSolidsAbove.size() - aboveBefore,
            (int)outSolidsBelow.size() - belowBefore);
        return;
    }

    splitConnect();
    splitFinish(inSolid, outSolidsAbove, outSolidsBelow);
    PolyhedralBoundedSolidStatistics::recordSplitProducedSolids(
        (int)outSolidsAbove.size() - aboveBefore,
        (int)outSolidsBelow.size() - belowBefore);

    //-----------------------------------------------------------------
    soov.clear();
    sone.clear();
    sonf.clear();
}
