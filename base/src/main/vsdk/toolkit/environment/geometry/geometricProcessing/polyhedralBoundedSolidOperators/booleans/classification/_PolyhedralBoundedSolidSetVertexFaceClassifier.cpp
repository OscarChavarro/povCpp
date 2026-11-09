#include "java/lang/String.h"
#include <cstdio>

#include "java/lang/Boolean.h"
#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetVertexFaceClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetVertexFaceClassifier Classifier;
typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector OnSector;

int Classifier::debugFlags = 0;

namespace {

const char* const TRACE_COPLANAR_TANGENTIAL_PROPERTY =
    "vsdk.setop.traceCoplanarTangential";
const int DEBUG_01_STRUCTURE = 0x01;
const int DEBUG_03_VERTEX_FACE_CLASSIFIER = 0x04;
const int DEBUG_99_SHOW_OPERATIONS = 0x40;

bool isCoplanarTangentialTraceEnabled()
{
    return java::Boolean::getBoolean(TRACE_COPLANAR_TANGENTIAL_PROPERTY);
}

void traceCoplanarTangential(const java::String& message)
{
    if ( !isCoplanarTangentialTraceEnabled() ) {
        return;
    }
    printf("[SetOpCoplanarTrace] %s\n", message.c_str());
}

int classFor(int op, bool inForUnion)
{
    if ( inForUnion ) {
        return (op == _PolyhedralBoundedSolidOperator::UNION) ? OnSector::IN : OnSector::OUT;
    }
    return (op == _PolyhedralBoundedSolidOperator::UNION) ? OnSector::OUT : OnSector::IN;
}

}

Classifier::_PolyhedralBoundedSolidSetVertexFaceClassifier()
    : sonea(nullptr), soneb(nullptr)
{
}

void Classifier::classify(_PolyhedralBoundedSolidVertex* v,
                          _PolyhedralBoundedSolidFace* f,
                          int op,
                          int BvsA,
                          int flags,
                          NullEdgeList* inSonea,
                          NullEdgeList* inSoneb,
                          PolyhedralBoundedSolid* inSolidA,
                          PolyhedralBoundedSolid* inSolidB)
{
    debugFlags = flags;
    sonea = inSonea;
    soneb = inSoneb;
    vertexFaceClassify(v, f, op, BvsA, inSolidA, inSolidB);
}

int Classifier::compareToZero(double value)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor::compareToZero(value);
}

void Classifier::applyCoplanarRulesToVertexFaceNeighborhood(
    java::ArrayList<SectorOnFace>& nbr,
    _PolyhedralBoundedSolidFace* referenceFace,
    InfinitePlane* referencePlane,
    int BvsA,
    int op,
    bool useMirrorFace)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidFace* localFace;
    Vector3Dd c;
    double d;
    size_t i;
    size_t nnbr;

    nnbr = nbr.size();
    java::ArrayList<SectorOnFace> backup(nbr);

    for ( i = 0; i < nnbr; i++ ) {
        he = nbr[i].sector;
        if ( he == nullptr || he->parentLoop == nullptr ||
             he->parentLoop->parentFace == nullptr ) {
            continue;
        }

        if ( useMirrorFace ) {
            he = he->mirrorHalfEdge();
            if ( he == nullptr || he->parentLoop == nullptr ||
                 he->parentLoop->parentFace == nullptr ) {
                continue;
            }
        }
        localFace = he->parentLoop->parentFace;
        if ( localFace->getContainingPlane() == nullptr ||
             referencePlane == nullptr ) {
            continue;
        }

        c = localFace->getContainingPlane()->getNormal().crossProduct(
            referencePlane->getNormal());
        d = c.dotProduct(c);
        if ( compareToZero(d) != 0 ) {
            continue;
        }

        d = localFace->getContainingPlane()->getNormal().dotProduct(
            referencePlane->getNormal());
        if ( compareToZero(d) == 1 ) {
            if ( BvsA != 0 ) {
                nbr[i].cl = classFor(op, true);
                nbr[(i + 1) % nnbr].cl = classFor(op, true);
            }
            else {
                nbr[i].cl = classFor(op, false);
                nbr[(i + 1) % nnbr].cl = classFor(op, false);
            }
        }
        else {
            nbr[i].cl = classFor(op, true);
            nbr[(i + 1) % nnbr].cl = classFor(op, true);
        }

        traceCoplanarTangential(
            "vf legacy coplanar rule op=" + java::String::valueOf(op) +
            " side=" + java::String::valueOf(BvsA) +
            " face=" + java::String::valueOf(referenceFace->id) +
            " localFace=" + java::String::valueOf(localFace->id) +
            " sectorIndex=" + java::String::valueOf(i) +
            " class=" + java::String::valueOf(nbr[i].cl));
    }

    size_t ins = 0;
    size_t outs = 0;
    for ( i = 0; i < nnbr; i++ ) {
        if ( nbr[i].cl == OnSector::OUT ) {
            outs++;
        }
        else {
            ins++;
        }
    }
    if ( outs == nnbr || ins == nnbr ) {
        for ( i = 0; i < nnbr; i++ ) {
            nbr[i].cl = backup[i].cl;
        }
    }
}

int Classifier::nextVertexId(PolyhedralBoundedSolid* current,
                             PolyhedralBoundedSolid* other)
{
    int a;
    int b;
    int m;

    a = current->getMaxVertexId();
    b = other->getMaxVertexId();
    m = a;
    if ( b > a ) {
        m = b;
    }

    return m + 1;
}

/**
Constructs a vector along the bisector of the sector defined by `he`.
This is the inward-oriented variant of the bisector from problem
[MANT1988].14.1 used by the vertex/face classifier.
*/
Vector3Dd Classifier::inside(_PolyhedralBoundedSolidHalfEdge* he)
{
    Vector3Dd middle;
    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n;

    a = (he->next())->startingVertex->position.subtract(
        he->startingVertex->position);
    b = (he->previous())->startingVertex->position.subtract(
        he->startingVertex->position);
    a = a.normalized();
    b = b.normalized();

    n = he->parentLoop->parentFace->getContainingPlane()->getNormal();

    middle = n.crossProduct(a);
    middle = middle.normalized();

    return middle;
}

/**
Current method is the first step for the initial vertex/face
classification of sectors (vertex neighborhood) for `vtx`, as indicated on
section [MANT1988].14.5.2 and program [MANT1988].14.4, but biased towards
the set operator classifier as proposed on section [MANT1988].15.6.1 and
problem [MANT1988].15.4.
*/
java::ArrayList<Classifier::SectorOnFace> Classifier::vertexFaceGetNeighborhood(
    _PolyhedralBoundedSolidVertex* vtx,
    InfinitePlane* referencePlane,
    int /*BvsA*/)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    Vector3Dd bisect;
    double d;
    java::ArrayList<SectorOnFace> neighborSectorsInfo;

    he = vtx->emanatingHalfEdge;
    do {
        SectorOnFace c;
        c.sector = he;
        d = referencePlane->pointDistance((he->next())->startingVertex->position);
        c.cl = compareToZero(d);
        c.isWide = false;
        c.position = (he->next())->startingVertex->position;
        c.situation = SectorOnFace::UNDEFINED;
        c.referencePlane = referencePlane;
        if ( checkWideness(he) ) {
            bisect = inside(he).add(vtx->position);
            c.situation = SectorOnFace::CROSSING_EDGE;
            neighborSectorsInfo.add(c);

            SectorOnFace wide;
            wide.sector = he;
            d = referencePlane->pointDistance(bisect);
            wide.cl = compareToZero(d);
            wide.isWide = true;
            wide.position = bisect;
            wide.situation = SectorOnFace::CROSSING_EDGE;
            wide.referencePlane = referencePlane;
            neighborSectorsInfo.add(wide);
        }
        else {
            neighborSectorsInfo.add(c);
        }
        he = (he->mirrorHalfEdge())->next();
    } while ( he != vtx->emanatingHalfEdge );

    size_t i;

    for ( i = 0; i < neighborSectorsInfo.size(); i++ ) {
        SectorOnFace& c = neighborSectorsInfo[i];
        if ( c.cl == SectorOnFace::ON && c.situation == SectorOnFace::UNDEFINED ) {
            c.situation = SectorOnFace::INPLANE_EDGE;
        }
    }

    return neighborSectorsInfo;
}

/**
Reclassifies the vertex/face neighborhood per [MANT1988].14.5
(sections 14.5.1 and 14.5.2), biased towards the set-operator classifier
as proposed in section 15.6.1 and problem 15.4. The `useMirrorFace`
flag of the underlying processor is always `false` here.
*/
void Classifier::vertexFaceReclassifyOnSectors(
    java::ArrayList<SectorOnFace>& nbr,
    _PolyhedralBoundedSolidFace* referenceFace,
    InfinitePlane* referencePlane,
    int BvsA,
    int op)
{
    applyCoplanarRulesToVertexFaceNeighborhood(
        nbr, referenceFace, referencePlane, BvsA, op, false);
}

void Classifier::printNbr(const java::ArrayList<SectorOnFace>& neighborSectorsInfo)
{
    size_t i;

    for ( i = 0; i < neighborSectorsInfo.size(); i++ ) {
        printf("    . %s\n", neighborSectorsInfo[i].toString().c_str());
    }
}

bool Classifier::inplaneEdgesOn(const java::ArrayList<SectorOnFace>& nbr)
{
    size_t i;

    for ( i = 0; i < nbr.size(); i++ ) {
        if ( nbr[i].situation == SectorOnFace::INPLANE_EDGE ) {
            return true;
        }
    }
    return false;
}

/**
Current method implements the set of changes from table [MANT1988].15.3
for the edge reclassification rules.
*/
void Classifier::vertexFaceReclassifyOnEdges(java::ArrayList<SectorOnFace>& nbr,
                                             int op)
{
    size_t i;

    for ( i = 0; i < nbr.size(); i++ ) {
        nbr[i].applyRules(op);
    }
}

namespace {

bool isInClass(int cl)
{
    return cl == _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::AinB ||
        cl == _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::BinA;
}

bool isOutClass(int cl)
{
    return cl == _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::AoutB ||
        cl == _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::BoutA;
}

}

/**
This method implements the third stage of the vertex/face classifier:
given the previously reclassified list of vertex neighbors, insert a new
vertex in the direction of the last "in" before an "out" sector of the
sequence. This follows section [MANT1988].14.6.2 and program
[MANT1988].14.7, biased for set operations as indicated by
[MANT1988].15.6.1.
*/
void Classifier::vertexFaceInsertNullEdges(java::ArrayList<SectorOnFace>& nbr,
                                           _PolyhedralBoundedSolidFace* f,
                                           _PolyhedralBoundedSolidVertex* v,
                                           int BvsA,
                                           PolyhedralBoundedSolid* inSolidA,
                                           PolyhedralBoundedSolid* inSolidB)
{
    size_t start;
    size_t i;
    _PolyhedralBoundedSolidHalfEdge* head;
    _PolyhedralBoundedSolidHalfEdge* tail;
    PolyhedralBoundedSolid* solida;
    size_t nnbr = nbr.size();
    NullEdgeList* sone = nullptr;

    solida = v->emanatingHalfEdge->parentLoop->parentFace->parentSolid;

    if ( nnbr <= 0 ) {
        return;
    }

    i = 0;
    while ( !(isInClass(nbr[i].cl) && isOutClass(nbr[(i + 1) % nnbr].cl)) ) {
        i++;
        if ( i >= nnbr ) {
            return;
        }
    }
    start = i;
    head = nbr[i].sector;

    while ( true ) {
        while ( !(isOutClass(nbr[i].cl) && isInClass(nbr[(i + 1) % nnbr].cl)) ) {
            i = (i + 1) % nnbr;
        }
        tail = nbr[i].sector;

        if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 &&
             (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
            printf("       -> LMEV (Vertex/face split):\n");
            printf("          . (%zu) H1: %s\n", start, head->toString().c_str());
            printf("          . (%zu) H2: %s\n", i, tail->toString().c_str());
        }

        PolyhedralBoundedSolidEulerOperators::lmev(solida, head, tail,
            nextVertexId(inSolidA, inSolidB), head->startingVertex->position);

        if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 &&
             (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
            printf("          . New vertex: %d\n", head->startingVertex->id);
        }

        if ( BvsA != 0 ) {
            sone = soneb;
        }
        else {
            sone = sonea;
        }
        sone->add(_PolyhedralBoundedSolidSetOperatorNullEdge(
            head->previous()->parentEdge));

        makeRing(f, v, BvsA, inSolidA, inSolidB);

        while ( !(isInClass(nbr[i].cl) && isOutClass(nbr[(i + 1) % nnbr].cl)) ) {
            i = (i + 1) % nnbr;
            if ( i == start ) {
                return;
            }
        }
    }
}

void Classifier::vertexFaceClassify(_PolyhedralBoundedSolidVertex* v,
                                    _PolyhedralBoundedSolidFace* f,
                                    int op,
                                    int BvsA,
                                    PolyhedralBoundedSolid* inSolidA,
                                    PolyhedralBoundedSolid* inSolidB)
{
    java::ArrayList<SectorOnFace> nbr;

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 ) {
            printf("  * ");
        }
        else {
            printf("  - ");
        }
        printf("Vertex/face pair V[%d] / f[%d]\n", v->id, f->id);
    }

    nbr = vertexFaceGetNeighborhood(v, f->getContainingPlane(), BvsA);
    if ( inplaneEdgesOn(nbr) ) {
        java::Collections::reverse(nbr);
    }

    if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 ) {
        printf("   - Initial sector neigborhood by near end vertices:\n");
        printNbr(nbr);
    }

    vertexFaceReclassifyOnSectors(nbr, f, f->getContainingPlane(), BvsA, op);

    size_t i;
    for ( i = 0; i < nbr.size(); i++ ) {
        nbr[i].updateLabel(BvsA);
    }

    if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 ) {
        printf("   - Sector neigborhood reclassified on sectors (8-way boundary classification):\n");
        printNbr(nbr);
    }

    vertexFaceReclassifyOnEdges(nbr, op);

    if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 ) {
        printf("   - Sector neigborhood reclassified on edges:\n");
        printNbr(nbr);
    }

    vertexFaceInsertNullEdges(nbr, f, v, BvsA, inSolidA, inSolidB);
}

void Classifier::makeRing(_PolyhedralBoundedSolidFace* f,
                          _PolyhedralBoundedSolidVertex* v,
                          int type,
                          PolyhedralBoundedSolid* inSolidA,
                          PolyhedralBoundedSolid* inSolidB)
{
    PolyhedralBoundedSolid* solida;
    PolyhedralBoundedSolid* solidb;
    PolyhedralBoundedSolid* ringSolid;
    _PolyhedralBoundedSolidHalfEdge* he;

    solida = inSolidA;
    solidb = inSolidB;
    if ( type == 1 ) {
        solida = inSolidB;
        solidb = inSolidA;
    }

    he = f->boundariesList.get(0)->boundaryStartHalfEdge;
    ringSolid = he->parentLoop->parentFace->parentSolid;

    int vn1;
    int vn2;
    vn1 = nextVertexId(solida, solidb);
    PolyhedralBoundedSolidEulerOperators::lmev(ringSolid, he, he, vn1, v->position);
    he = he->previous();
    PolyhedralBoundedSolidEulerOperators::lkemr(ringSolid, he->mirrorHalfEdge(), he);

    vn2 = nextVertexId(solida, solidb);
    PolyhedralBoundedSolidEulerOperators::lmev(ringSolid, he, he, vn2, v->position);

    if ( (debugFlags & DEBUG_03_VERTEX_FACE_CLASSIFIER) != 0x00 &&
         (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
        printf("       -> MAKE_RING (Vertex/face pierce):\n");
        printf("          . New vertexes: %d/%d.\n", vn1, vn2);
    }

    NullEdgeList* sone = nullptr;
    if ( type == 1 ) {
        sone = sonea;
    }
    else {
        sone = soneb;
    }
    sone->add(_PolyhedralBoundedSolidSetOperatorNullEdge(he->parentEdge));
}
