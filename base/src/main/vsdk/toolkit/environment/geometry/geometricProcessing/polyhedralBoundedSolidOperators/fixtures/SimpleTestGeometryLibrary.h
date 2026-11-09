//= References:                                                             =
//= [APPE1967] Appel, Arthur. "The notion of quantitative invisibility and  =
//=          the machine rendering of solids". Proceedings, ACM National    =
//=          meeting 1967.                                                  =
//= [MANT1986] Mantyla Martti. "Boolean Operations of 2-Manifolds through   =
//=     Vertex Neighborhood Classification". ACM Transactions on Graphics,  =
//=     Vol. 5, No. 1, January 1986, pp. 1-29.                              =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __SIMPLE_TEST_GEOMETRY_LIBRARY__
#define __SIMPLE_TEST_GEOMETRY_LIBRARY__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/processing/ProcessingElement.h"

class PolyhedralBoundedSolid;

/**
This is a utility class containing a lot of geometry examples (mostly
geometry generating procedures). This is a companion class for the
`ComputationalGeometry` and `GeometricModeler` classes, which holds
geometrical queries and other geometry generation procedures.

This class comprises static methods, depending on only of Vitral's Entity's.
From a design point of view, it can be viewed as a "strategy" design pattern
in the sense that encapsulates algorithms. It also could be viewed as a "factory"
or "abstract factory", as it is a class for creating objects, following
the data hierarchy of interface "geometry".

An important characteristic of this class is that the simple geometries that
creates, are usually "inspired" (or "borrowed") from classic textbooks and
papers from the computer graphics academic community. Most of the methods
contain references to figures and other data published by original authors.

The simple geometry test object provided here are useful because:
  - They are procedurally generated. That means that no input/output is used.
    This is useful in developing environments where input/output is not
    available, or where its operation is not ported yet (for example mobile
    devices), so this class can be used for testing Vitral software
    infrastructure, before input/output operation are made available.
  - For algorithm benchmarking. Usually, original authors has invented
    this simple test objects for testing algorithms and measuring its
    performance.  When reimplementing the original algorithms in Vitral,
    running them with similar test objects is useful when comparing
    Vitral's implementation performance.

C++ port note: only the objects that do not need the boolean pipeline of
[MANT1988] chapters 14 and 15 (splitter, set operator) are ported, as it is
not available in the C++ port yet (see `PolyhedralBoundedSolidModeler`).
*/
class SimpleTestGeometryLibrary : public ProcessingElement {
public:
    /**
    This method builds a test solid for evaluating the splitting algorithm in
    a controlled way. The generated object is similar to that shown on figures
    [MANT1986].4., [MANT1986].5., [MANT1986].8., [MANT1986].10.,
    [MANT1988].14.2., [MANT1988].14.3., and [MANT1988].14.6.

    Generated solid is interesting when splitting with respect to the plane
    Z=0.3 because stress the splitting algorithm to consider multiple vertex
    classification cases.
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createTestObjectMANT1986_1();

    /**
    This method uses basic blocks and constructive solid geometry to build up
    a test object similar to the one appearing in the lower part of figure
    [APPE1967].7. Note that this method returns a solid with two shells with
    a total of 54 vertices, 84 edges and 32 faces, as expected from description
    reported in [APPE1967]. This method is provided for benchmarking and
    comparison purposes!
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createTestObjectAPPE1967_3();

    /**
    Builds, with basic blocks and constructive solid geometry, a test object
    similar to the one appearing in the upper part of figure [APPE1967].7.
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createTestObjectAPPE1967_2();

    /**
    Builds, with basic blocks and constructive solid geometry, a test object
    similar to the one appearing in the middle of figure [APPE1967].7.
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createTestObjectAPPE1967_1();

    /**
    Test pair similar to figures [MANT1986].11. and [MANT1988].15.4, the
    simplest case for set operations (only the vertex-face classifier is
    called, without sector reclassification).
    @return two new solids, owned by the caller
    */
    static java::ArrayList<PolyhedralBoundedSolid*> createTestObjectPairMANT1986_2();

    /**
    Test pair similar to figures [MANT1986].12. and [MANT1988].15.5, whose
    vertices touch in several different ways and with overlapping faces.
    @return two new solids, owned by the caller
    */
    static java::ArrayList<PolyhedralBoundedSolid*> createTestObjectPairMANT1988_3();

    /**
    Test pair similar to figure [MANT1988].6.13: left and front view
    extrusions for profile set operations (section [MANT1988].6.4.2.).
    @return two new solids, owned by the caller
    */
    static java::ArrayList<PolyhedralBoundedSolid*> createTestObjectPairMANT1988_6_13();

    /**
    Test pair similar to figure [MANT1988].15.1: view profiles difficult to
    intersect due to their overlapping faces.
    @return two new solids, owned by the caller
    */
    static java::ArrayList<PolyhedralBoundedSolid*> createTestObjectPairMANT1988_15_1();

    /**
    Test pair similar to figures [MANT1988].15.2. and [MANT1988].15.3.
    @param situation -1: holed object, 0: limit case, 1: open object
    @return two new solids (block and wedge), owned by the caller
    */
    static java::ArrayList<PolyhedralBoundedSolid*> createTestObjectPairMANT1988_15_2(
        int situation);
};

#endif
