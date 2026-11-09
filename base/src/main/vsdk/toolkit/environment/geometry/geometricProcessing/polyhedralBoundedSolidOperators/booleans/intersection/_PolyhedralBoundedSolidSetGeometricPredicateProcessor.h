//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_GEOMETRIC_PREDICATE_PROCESSOR__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_GEOMETRIC_PREDICATE_PROCESSOR__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"

class InfinitePlane;

/**
Geometric predicates and coplanar-angle algebra used by the set-operations
classifiers from sections [MANT1988].14.5, [MANT1988].15.6.1, and
[MANT1988].15.6.2.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetGeometricPredicateProcessor`.
C++ port note: the package private members of the Java class are public
here.
*/
class _PolyhedralBoundedSolidSetGeometricPredicateProcessor
    : public _PolyhedralBoundedSolidOperator {
public:
    typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace SectorOnFace;
    typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex SectorOnVertex;

    /**
    Structured trace of a single `sectoroverlap` invocation captured when
    the static collector is active.
    */
    struct SectoroverlapTraceEntry {
        int callIndex;
        int faceA;
        int faceB;
        int vertexAFrom;
        int vertexATo;
        int vertexBFrom;
        int vertexBTo;
        double a1;
        double a2;
        double b1;
        double b2;
        double diffA2B1;
        double diffB2A1;
        bool boundaryRayContact;
        bool decision;
    };

    static void enableSectoroverlapTrace();
    static void disableSectoroverlapTrace();
    /**
    @return the trace, or null when it is disabled
    */
    static const java::ArrayList<SectoroverlapTraceEntry>* getSectoroverlapTrace();

    static int compareToZero(double value);
    static int pointInFace(_PolyhedralBoundedSolidFace* face, const Vector3Dd& point);
    static _PolyhedralBoundedSolidFace::PointInsideResult pointInFaceDetailed(
        _PolyhedralBoundedSolidFace* face, const Vector3Dd& point);
    static bool colinearVectors(const Vector3Dd& a, const Vector3Dd& b);
    static bool colinearVectorsWithDirection(const Vector3Dd& a, const Vector3Dd& b);

    /**
    Following program [MANT1988].15.9. According to the sector intersection
    test from section [MANT1988].15.6.2, the variables are interpreted as in
    figure [MANT1988].15.8 and equation [MANT1988].15.5.
    */
    static bool sctrwitthin(const Vector3Dd& dir, const Vector3Dd& ref1,
                            const Vector3Dd& ref2, const Vector3Dd& ref12);

    /**
    Strict version of the sector-within test from section [MANT1988].15.6.2
    that excludes the boundary-line cases implicit in figure [MANT1988].15.8
    and equation [MANT1988].15.5.
    */
    static bool sctrwitthinProper(const Vector3Dd& dir, const Vector3Dd& ref1,
                                  const Vector3Dd& ref2, const Vector3Dd& ref12);

    /**
    Checks overlap of two coplanar sectors for the vertex/vertex classifier.
    Following section [MANT1988].15.6.2, where the operation is required but
    left implicit after program [MANT1988].15.9.
    */
    static bool sectoroverlap(const SectorOnVertex& na, const SectorOnVertex& nb,
                              bool withDebug);

    /**
    Resolves the class to propagate for coplanar sector pairs when applying the
    8-way boundary-classification logic of section [MANT1988].15.3 and the
    vertex/vertex classifier of section [MANT1988].15.6.2.
    */
    static int resolveCoplanarVertexVertexClass(int op, bool sameOrientation,
                                                bool sideA);

    /**
    Applies the coplanar reclassification rules for the vertex/face classifier,
    starting from sections [MANT1988].14.5.1 and [MANT1988].14.5.2 and biased
    toward set operations as proposed in [MANT1988].15.6.1 and problem
    [MANT1988].15.4.
    */
    static void applyCoplanarRulesToVertexFaceNeighborhood(
        java::ArrayList<SectorOnFace>& nbr,
        _PolyhedralBoundedSolidFace* referenceFace,
        InfinitePlane* referencePlane,
        int BvsA, int op,
        bool useMirrorFace);

    static int classifyCoplanarSectorRelation(
        const SectorOnFace* sectorInfo,
        _PolyhedralBoundedSolidFace* referenceFace);

private:
    static java::ArrayList<SectoroverlapTraceEntry>* sectoroverlapTrace;
    static int sectoroverlapCallCounter;

    static void recordSectoroverlapCall(const SectorOnVertex& na,
                                        const SectorOnVertex& nb,
                                        double a1, double a2, double b1,
                                        double b2, bool decision);
    static double angleFromVectors(const Vector3Dd& u, const Vector3Dd& v,
                                   const Vector3Dd& a);
    static int resolveCoplanarSectorClass(int op, int BvsA,
                                          bool sameOrientation);

    /**
    @return the tolerances of the current operation (for the helpers of
    the implementation file)
    */
    static const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context();
};

#endif
