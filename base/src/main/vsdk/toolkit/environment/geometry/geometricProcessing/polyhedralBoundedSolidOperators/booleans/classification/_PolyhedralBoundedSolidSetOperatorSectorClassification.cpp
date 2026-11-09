#include "java/lang/String.h"
#include <cmath>

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_PolyhedralBoundedSolidSetOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

java::String vectorText(const Vector3Dd& v)
{
    java::String* text = v.toString();
    java::String result(text->c_str());
    delete text;
    return result;
}

}

//= On vertex =============================================================

_PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex::
_PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex()
    : he(nullptr), hasReferenceFrame(false), wide(false)
{
}

double _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex::getAngle() const
{
    if ( !hasReferenceFrame ) {
        return -1000;
    }

    double x;
    double y;
    double an;
    Vector3Dd a = ref1;

    if ( _PolyhedralBoundedSolidSetOperator::colinearVectorsWithDirection(ref1,
             referenceLine) ) {
        a = ref2;
    }

    Vector3Dd u;
    Vector3Dd v;

    u = referenceU.normalized();
    v = referenceV.normalized();
    a = a.normalized();

    x = a.dotProduct(u);
    y = a.dotProduct(v);

    an = std::acos(x);
    if ( y < 0 ) an *= -1;

    return an;
}

java::String _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex::toString() const
{
    java::String msg = "R1: " + vectorText(ref1) + " R2: " + vectorText(ref2) +
        " HE " + java::String::valueOf(he->startingVertex->id) + "/" +
        java::String::valueOf(he->next()->startingVertex->id) + (wide ? "(W)" : "(nw)");
    return java::String(msg.c_str());
}

int _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex::compareTo(
    const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex& other) const
{
    double a;
    double b;

    a = this->getAngle();
    b = other.getAngle();

    if ( a > b ) return 1;
    if ( a < b ) return -1;
    return 0;
}

//= On sector =============================================================

_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector::
_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector()
    : secta(0), sectb(0), s1a(0), s2a(0), s1b(0), s2b(0), intersect(false),
      hea(nullptr), heb(nullptr), wa(false), wb(false)
{
}

const char* _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector::label(int i)
{
    switch ( i ) {
      case ON: return "on";
      case OUT: return "OUT";
      case IN: return "IN";
    }
    return "<Unknown>";
}

void _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector::fillCases()
{
    if ( s1a == ON ) {
        switch ( s2a ) {
        case IN: s1a = OUT; break;
        case OUT: s1a = IN; break;
        }
    }
    if ( s2a == ON ) {
        switch ( s1a ) {
        case IN: s2a = OUT; break;
        case OUT: s2a = IN; break;
        }
    }
    if ( s1b == ON ) {
        switch ( s2b ) {
        case IN: s1b = OUT; break;
        case OUT: s1b = IN; break;
        }
    }
    if ( s2b == ON ) {
        switch ( s1b ) {
        case IN: s2b = OUT; break;
        case OUT: s2b = IN; break;
        }
    }
}

java::String _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector::toString() const
{
    java::String msg = "Sector pair ";

    msg += "A[" + java::String::valueOf(secta+1) + "] / B[" + java::String::valueOf(sectb+1) + "]: ";

    msg += "VERTICES ( " +
        java::String::valueOf(hea->startingVertex->id) + "-" +
        java::String::valueOf(hea->next()->startingVertex->id) + (wa ? "(W)" : "(nw)") + " / " +
        java::String::valueOf(heb->startingVertex->id) + "-" +
        java::String::valueOf(heb->next()->startingVertex->id) + (wb ? "(W)" : "(nw)") + " ) - ";
    msg += java::String("[") + label(s1a) + "/" + label(s2a) + ", " + label(s1b) +
        "/" + label(s2b) + "] ";
    if ( intersect ) {
        msg += "intersecting";
    }
    else {
        msg += "(droped)";
    }

    if ( s1a != 0 && s1b != 0 && s2a != 0 && s2b != 0 && intersect ) {
        msg += " (**) ";
    }
    return java::String(msg.c_str());
}

//= On face ===============================================================

_PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::
_PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace()
    : sector(nullptr), referencePlane(nullptr), cl(0), isWide(false),
      situation(UNDEFINED), reverse(false), coplanarRelation(COPLANAR_UNKNOWN)
{
}

void _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::applyRules(int op)
{
    if ( op == UNION ) {
        switch ( cl ) {
          case AonBplus:     cl = AoutB;    break;
          case AonBminus:    cl = AinB;    break;
          case BonAplus:     cl = BinA;    break;
          case BonAminus:    cl = BinA;    break;
        }
    }
    else if ( op == INTERSECTION ) {
        switch ( cl ) {
          case AonBplus:     cl = AinB;    break;
          case AonBminus:    cl = AoutB;    break;
          case BonAplus:     cl = BoutA;    break;
          case BonAminus:    cl = BoutA;    break;
        }
    }
    else if ( op == SUBTRACT ) {
        switch ( cl ) {
          case AonBplus:     cl = AinB;    break;
          case AonBminus:    cl = AoutB;    break;
          case BonAplus:     cl = BoutA;    break;
          case BonAminus:    cl = BoutA;    break;
        }
    }
}

void _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::updateLabel(int BvsA)
{
    InfinitePlane* a = sector->parentLoop->parentFace->getContainingPlane();
    InfinitePlane* b = referencePlane;

    if ( BvsA == 0 ) {
        switch ( cl ) {
          case ABOVE: cl = AoutB; break;
          case BELOW: cl = AinB; break;
          case ON:
            if ( coplanarRelation != COPLANAR_UNKNOWN &&
                 coplanarRelation != COPLANAR_OVERLAP ) {
                cl = AoutB;
            }
            else if ( a->overlapsWith(*b, numericContext.bigEpsilon()) ) {
                cl = AonBplus;
            }
            else {
                cl = AonBminus;
            }
            break;
        }
    }
    else {
        switch ( cl ) {
          case ABOVE: cl = BoutA; break;
          case BELOW: cl = BinA; break;
          case ON:
            if ( coplanarRelation != COPLANAR_UNKNOWN &&
                 coplanarRelation != COPLANAR_OVERLAP ) {
                cl = BoutA;
            }
            else if ( a->overlapsWith(*b, numericContext.bigEpsilon()) ) {
                cl = BonAplus;
            }
            else {
                cl = BonAminus;
            }
            break;
        }
    }
}

java::String _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace::toString() const
{
    java::String msg = "Sector(";
    msg += java::String(sector->toString().c_str()) + " | ";
    switch ( cl ) {
      case ABOVE: msg += " ABOVE"; break;
      case BELOW: msg += " BELOW"; break;
      case ON: msg += " ON"; break;
      case AinB: msg += "AinB"; break;
      case AoutB: msg += "AoutB"; break;
      case BinA: msg += "BinA"; break;
      case BoutA: msg += "BoutA"; break;
      case AonBplus: msg += "AonBplus"; break;
      case AonBminus: msg += "AonBminus"; break;
      case BonAplus: msg += "BonAplus"; break;
      case BonAminus: msg += "BonAminus"; break;
      default: msg += "<INVALID!>"; break;
    }
    msg += " ";
    if ( isWide ) {
        msg += "(W) ";
    }

    switch ( situation ) {
      case COPLANAR_FACE: msg += "<COPLANAR_FACE>"; break;
      case INPLANE_EDGE: msg += "<INPLANE_EDGE>"; break;
      case CROSSING_EDGE: msg += "<CROSSING_EDGE>"; break;
      default: msg += "<UNDEFINED>"; break;
    }

    msg += ")";
    return java::String(msg.c_str());
}
