#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"

PolyhedralBoundedSolid* SimpleTestGeometryLibrary::createTestObjectMANT1986_1()
{
    PolyhedralBoundedSolid* solid;

    solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(solid, Vector3Dd(0.00, 0.40, 0.00), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 1, 2, Vector3Dd(0.94, 0.40, 0.00));
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 2, 3, Vector3Dd(0.94, 0.40, 0.46));
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 3, 4, Vector3Dd(0.60, 0.40, 0.30));
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 4, 5, Vector3Dd(0.37, 0.40, 0.30));
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 5, 6, Vector3Dd(0.18, 0.40, 0.46));
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 6, 7, Vector3Dd(0.00, 0.40, 0.30));
    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1, 7, 6, 1, 2, 2);

    Matrix4x4d T;
    T = T.translation(0, -0.4, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        solid, solid->findFace(1), T);

    return solid;
}

namespace {

PolyhedralBoundedSolid* translatedBox(double sx, double sy, double sz,
    double tx, double ty, double tz)
{
    Matrix4x4d T;
    T = T.translation(tx, ty, tz);
    Box box(Vector3Dd(sx, sy, sz));
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    PolyhedralBoundedSolidModeler::applyTransformation(solid, T);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

/**
C++ ownership: the operands of a set operation stay owned by the caller.
*/
PolyhedralBoundedSolid* unionAndRelease(PolyhedralBoundedSolid* a,
    PolyhedralBoundedSolid* b)
{
    PolyhedralBoundedSolid* result = PolyhedralBoundedSolidModeler::setOp(
        a, b, PolyhedralBoundedSolidModeler::UNION);
    delete a;
    delete b;
    return result;
}

}

PolyhedralBoundedSolid* SimpleTestGeometryLibrary::createTestObjectAPPE1967_3()
{
    PolyhedralBoundedSolid* a;
    PolyhedralBoundedSolid* b;

    a = createTestObjectAPPE1967_1();
    b = createTestObjectAPPE1967_2();

    return unionAndRelease(a, b);
}

PolyhedralBoundedSolid* SimpleTestGeometryLibrary::createTestObjectAPPE1967_2()
{
    PolyhedralBoundedSolid* a;
    PolyhedralBoundedSolid* b;
    PolyhedralBoundedSolid* c;
    PolyhedralBoundedSolid* d;
    PolyhedralBoundedSolid* ab;
    PolyhedralBoundedSolid* cd;

    a = translatedBox(0.6, 0.2, 0.2, 0.3, 0.1+0.4, 0.1+0.4);
    b = translatedBox(0.2, 0.2, 1.0, 0.5, 0.5, 0.5);
    c = translatedBox(0.6, 0.2, 0.2, 0.7, 0.5, 0.9);
    d = translatedBox(0.2, 1.0, 0.2, 0.9, 0.5, 0.9);

    //-----------------------------------------------------------------
    ab = unionAndRelease(a, b);
    cd = unionAndRelease(c, d);
    return unionAndRelease(ab, cd);
}

PolyhedralBoundedSolid* SimpleTestGeometryLibrary::createTestObjectAPPE1967_1()
{
    PolyhedralBoundedSolid* a;
    PolyhedralBoundedSolid* b;
    PolyhedralBoundedSolid* c;
    PolyhedralBoundedSolid* d;
    PolyhedralBoundedSolid* e;
    PolyhedralBoundedSolid* f;
    PolyhedralBoundedSolid* g;
    PolyhedralBoundedSolid* h;
    PolyhedralBoundedSolid* ac;
    PolyhedralBoundedSolid* bd;
    PolyhedralBoundedSolid* eg;
    PolyhedralBoundedSolid* fh;
    PolyhedralBoundedSolid* abcd;
    PolyhedralBoundedSolid* efgh;

    a = translatedBox(1, 0.2, 0.2, 0.5, 0.1, 0.1);
    b = translatedBox(1, 0.2, 0.2, 0.5, 0.9, 0.1);
    c = translatedBox(0.2, 1, 0.2, 0.1, 0.5, 0.1);
    d = translatedBox(0.2, 1, 0.2, 0.9, 0.5, 0.1);

    //-----------------------------------------------------------------
    ac = unionAndRelease(a, c);
    bd = unionAndRelease(b, d);
    abcd = unionAndRelease(bd, ac);

    //-----------------------------------------------------------------
    e = translatedBox(0.2, 1, 0.2, 0.1, 0.5, 0.1);
    f = translatedBox(0.2, 1, 0.2, 0.1, 0.5, 0.9);
    g = translatedBox(0.2, 0.2, 1, 0.1, 0.1, 0.5);
    h = translatedBox(0.2, 0.2, 1, 0.1, 0.9, 0.5);

    //-----------------------------------------------------------------
    eg = unionAndRelease(e, g);
    fh = unionAndRelease(f, h);
    efgh = unionAndRelease(eg, fh);
    return unionAndRelease(abcd, efgh);
}

java::ArrayList<PolyhedralBoundedSolid*> SimpleTestGeometryLibrary::createTestObjectPairMANT1986_2()
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;

    operands.add(translatedBox(1, 0.5, 0.6, 0.5, 0.25, 0.3));
    operands.add(translatedBox(1, 0.5, 0.6,
        0.5+0.24, 0.25-0.18, 0.3+0.42));
    return operands;
}

java::ArrayList<PolyhedralBoundedSolid*> SimpleTestGeometryLibrary::createTestObjectPairMANT1988_3()
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    PolyhedralBoundedSolid* a;
    PolyhedralBoundedSolid* b;

    //-----------------------------------------------------------------
    a = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(a,          Vector3Dd(0.00+0.05, 0.42+0.05, 0.00+0.05), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(a, 1, 1, 2, Vector3Dd(0.92+0.05, 0.42+0.05, 0.00+0.05));
    PolyhedralBoundedSolidEulerOperators::smev(a, 1, 2, 3, Vector3Dd(0.92+0.05, 0.42+0.05, 0.72+0.05));
    PolyhedralBoundedSolidEulerOperators::smev(a, 1, 3, 4, Vector3Dd(0.70+0.05, 0.42+0.05, 0.72+0.05));
    PolyhedralBoundedSolidEulerOperators::smev(a, 1, 4, 5, Vector3Dd(0.70+0.05, 0.42+0.05, 0.18+0.05));
    PolyhedralBoundedSolidEulerOperators::smev(a, 1, 5, 6, Vector3Dd(0.00+0.05, 0.42+0.05, 0.18+0.05));
    PolyhedralBoundedSolidEulerOperators::mef(a, 1, 1, 6, 5, 1, 2, 2);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(a);

    Matrix4x4d T;
    T = T.translation(0, -0.42, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        a, a->findFace(1), T);

    //-----------------------------------------------------------------
    b = translatedBox(0.58, 0.42, 0.18,
        0.05 +0.58/2.0+(0.92-0.58) /*+ 0.0001*/,
        0.05 + 0.42/2.0 - 0.42/2.0,
        0.05 + 0.18/2.0 + 0.18 /*+ 0.0001*/);

    operands.add(a);
    operands.add(b);
    return operands;
}

java::ArrayList<PolyhedralBoundedSolid*> SimpleTestGeometryLibrary::createTestObjectPairMANT1988_6_13()
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    Matrix4x4d T;

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* leftView = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(leftView, Vector3Dd(0, 1, 0), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 1, 2, Vector3Dd(3.1/3.7, 1, 0));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 2, 3, Vector3Dd(3.1/3.7, 1, 0.6/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 3, 4, Vector3Dd(1.6/3.7, 1, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 4, 5, Vector3Dd(0, 1, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::mef(leftView, 1, 1, 5, 4, 1, 2, 2);

    T = Matrix4x4d();
    T = T.translation(0, -1.0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        leftView, leftView->findFace(1), T);

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* frontView = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(frontView, Vector3Dd(0, 0, 0), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 1, 2, Vector3Dd(0, 1, 0));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 2, 3, Vector3Dd(0, 1, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 3, 4, Vector3Dd(0, 2.8/3.7, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 4, 5, Vector3Dd(0, 2.8/3.7, 0.3/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 5, 6, Vector3Dd(0, 0.9/3.7, 0.3/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 6, 7, Vector3Dd(0, 0.9/3.7, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 7, 8, Vector3Dd(0, 0, 1.2/3.7));
    PolyhedralBoundedSolidEulerOperators::mef(frontView, 1, 1, 8, 7, 1, 2, 2);

    T = Matrix4x4d();
    T = T.translation(1, 0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        frontView, frontView->findFace(1), T);

    operands.add(leftView);
    operands.add(frontView);
    return operands;
}

java::ArrayList<PolyhedralBoundedSolid*> SimpleTestGeometryLibrary::createTestObjectPairMANT1988_15_1()
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    Matrix4x4d T;

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* leftView = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(leftView, Vector3Dd(0, 1, 0), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 1, 2, Vector3Dd(1, 1, 0));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 2, 3, Vector3Dd(1, 1, 0.25));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 3, 4, Vector3Dd(1.0/3.0, 1, 0.25));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 4, 5, Vector3Dd(1.0/3.0, 1, 1));
    PolyhedralBoundedSolidEulerOperators::smev(leftView, 1, 5, 6, Vector3Dd(0, 1, 1));
    PolyhedralBoundedSolidEulerOperators::mef(leftView, 1, 1, 6, 5, 1, 2, 2);

    T = Matrix4x4d();
    T = T.translation(0, -1.0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        leftView, leftView->findFace(1), T);

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* frontView = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(frontView, Vector3Dd(0, 0, 0), 1, 1);
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 1, 2, Vector3Dd(0, 1, 0));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 2, 3, Vector3Dd(0, 1, 7.0/12.0));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 3, 4, Vector3Dd(0, 5.0/6.0, 1));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 4, 5, Vector3Dd(0, 1.0/6.0, 1));
    PolyhedralBoundedSolidEulerOperators::smev(frontView, 1, 5, 6, Vector3Dd(0, 0, 7.0/12.0));
    PolyhedralBoundedSolidEulerOperators::mef(frontView, 1, 1, 6, 5, 1, 2, 2);

    T = Matrix4x4d();
    T = T.translation(1, 0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        frontView, frontView->findFace(1), T);

    operands.add(leftView);
    operands.add(frontView);
    return operands;
}

java::ArrayList<PolyhedralBoundedSolid*> SimpleTestGeometryLibrary::createTestObjectPairMANT1988_15_2(
    int situation)
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* block = translatedBox(0.5, 1, 0.6,
        0.25 + 0.1375, 0.5, 0.3);

    //-----------------------------------------------------------------
    double facesModelDelta = 0.05;
    PolyhedralBoundedSolid* wedge = new PolyhedralBoundedSolid();

    if ( situation == -1 ) {
        // Lowered wedge, generating a closed holed object
        PolyhedralBoundedSolidEulerOperators::mvfs(wedge, Vector3Dd(0, 0.225, 0.3 - facesModelDelta), 1, 1);
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 1, 2, Vector3Dd(0, 0.775, 0.3 - facesModelDelta));
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 2, 3, Vector3Dd(0, 0.5, 0.6 - facesModelDelta));
    }
    else if ( situation == 1 ) {
        // Raised wedge, generating an open object with no holes
        PolyhedralBoundedSolidEulerOperators::mvfs(wedge, Vector3Dd(0, 0.225, 0.3 + facesModelDelta), 1, 1);
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 1, 2, Vector3Dd(0, 0.775, 0.3 + facesModelDelta));
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 2, 3, Vector3Dd(0, 0.5, 0.6 + facesModelDelta));
    }
    else {
        // Original on edge wedge, generating non-manifold object
        PolyhedralBoundedSolidEulerOperators::mvfs(wedge, Vector3Dd(0, 0.225, 0.3), 1, 1);
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 1, 2, Vector3Dd(0, 0.775, 0.3));
        PolyhedralBoundedSolidEulerOperators::smev(wedge, 1, 2, 3, Vector3Dd(0, 0.5, 0.6));
    }

    PolyhedralBoundedSolidEulerOperators::mef(wedge, 1, 1, 3, 2, 1, 2, 2);

    Matrix4x4d transformationMatrix;
    transformationMatrix = transformationMatrix.translation(0.775, 0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        wedge, wedge->findFace(1), transformationMatrix);

    operands.add(block);
    operands.add(wedge);
    return operands;
}
