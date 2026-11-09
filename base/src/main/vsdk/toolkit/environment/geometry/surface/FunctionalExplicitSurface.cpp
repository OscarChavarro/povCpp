#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpression.h"
#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpressionException.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
FunctionalExplicitSurface::FunctionalExplicitSurface(const java::String& fxy)
    : xyFunction(0), internalTriangleMesh(0)
{
    init(fxy);
}

FunctionalExplicitSurface::~FunctionalExplicitSurface()
{
    if (internalTriangleMesh != 0) delete internalTriangleMesh;
    delete xyFunction;
}

void FunctionalExplicitSurface::init(const java::String& fxy)
{
    functionExpression = fxy;
    delete xyFunction;
    xyFunction = new AlgebraicExpression();
    try {
        xyFunction->setExpression(fxy);
    }
    catch (const AlgebraicExpressionException& e) {
        Logger::reportMessage("FunctionalExplicitSurface", Logger::WARNING,
            "constructor",
            java::String("Cannot create algebraic expression for \"") + fxy +
            "\":\n" + e.getMessage());
        try {
            xyFunction->setExpression("0");
        }
        catch (const AlgebraicExpressionException& e2) {
            Logger::reportMessage("FunctionalExplicitSurface", Logger::FATAL_ERROR,
                "constructor",
                java::String("So bad. Something is wrong with algebraic expressions!:\n") +
                e2.getMessage());
        }
    }
    minXBound = minYBound = minZBound = -1.0;
    maxXBound = maxYBound = maxZBound = 1.0;
    tesselationHintX = 10;
    tesselationHintY = 10;
    updateInternalGeometry();
}

java::String FunctionalExplicitSurface::getFunctionExpression() const { return functionExpression; }

void FunctionalExplicitSurface::setBounds(double minXBound, double minYBound, double minZBound,
                                          double maxXBound, double maxYBound, double maxZBound)
{
    this->minXBound = minXBound; this->minYBound = minYBound; this->minZBound = minZBound;
    this->maxXBound = maxXBound; this->maxYBound = maxYBound; this->maxZBound = maxZBound;
    updateInternalGeometry();
}

void FunctionalExplicitSurface::setTesselationHint(int tesx, int tesy) { tesselationHintX = tesx; tesselationHintY = tesy; updateInternalGeometry(); }
int FunctionalExplicitSurface::getTesselationHintX() const { return tesselationHintX; }
int FunctionalExplicitSurface::getTesselationHintY() const { return tesselationHintY; }
double FunctionalExplicitSurface::getMinXBound() const { return minXBound; }
double FunctionalExplicitSurface::getMinYBound() const { return minYBound; }
double FunctionalExplicitSurface::getMinZBound() const { return minZBound; }
double FunctionalExplicitSurface::getMaxXBound() const { return maxXBound; }
double FunctionalExplicitSurface::getMaxYBound() const { return maxYBound; }
double FunctionalExplicitSurface::getMaxZBound() const { return maxZBound; }

int FunctionalExplicitSurface::coord(int tesselationHintX, int, int ix, int iy) { return ((tesselationHintX+1)*iy) + ix; }

void FunctionalExplicitSurface::updateInternalGeometry()
{
    if (tesselationHintX <= 0 || tesselationHintY <= 0) return;
    if (internalTriangleMesh != 0) delete internalTriangleMesh;
    internalTriangleMesh = new TriangleMesh();

    double dx = (maxXBound - minXBound) / ((double)tesselationHintX);
    double dy = (maxYBound - minYBound) / ((double)tesselationHintY);

    internalTriangleMesh->initVertexPositionsArray((tesselationHintX+1)*(tesselationHintY+1));
    java::ArrayList<double>& v = internalTriangleMesh->getVertexPositions();

    // As the Java version, coordinates are accumulated (x += dx), and an
    // evaluation error leaves the mesh without triangles
    int index = 0;
    int ix;
    int iy;
    double x;
    double y;
    double z;
    try {
        for (iy = 0, y = minYBound; iy <= tesselationHintY; iy++, y += dy) {
            xyFunction->defineValue("y", y);
            for (ix = 0, x = minXBound; ix <= tesselationHintX; ix++, x += dx) {
                xyFunction->defineValue("x", x);
                z = xyFunction->eval();
                if (z > maxZBound) z = maxZBound;
                if (z < minZBound) z = minZBound;
                v[3*index+0] = x;
                v[3*index+1] = y;
                v[3*index+2] = z;
                index++;
            }
        }
    }
    catch (const AlgebraicExpressionException& e) {
        Logger::reportMessage("FunctionalExplicitSurface", Logger::WARNING, "constructor",
            java::String("Cannot evaluate algebraic expression!") + e.getMessage());
        return;
    }

    internalTriangleMesh->initTriangleArrays(tesselationHintX*tesselationHintY*2);
    java::ArrayList<int>& t = internalTriangleMesh->getTriangleIndexes();

    index = 0;
    for (iy = 0; iy < tesselationHintY; iy++) {
        for (ix = 0; ix < tesselationHintX; ix++) {
            t[3*index+0] = coord(tesselationHintX, tesselationHintY, ix, iy);
            t[3*index+1] = coord(tesselationHintX, tesselationHintY, ix+1, iy);
            t[3*index+2] = coord(tesselationHintX, tesselationHintY, ix+1, iy+1);
            index++;

            t[3*index+0] = coord(tesselationHintX, tesselationHintY, ix, iy);
            t[3*index+1] = coord(tesselationHintX, tesselationHintY, ix+1, iy+1);
            t[3*index+2] = coord(tesselationHintX, tesselationHintY, ix, iy+1);
            index++;
        }
    }

    internalTriangleMesh->calculateNormals();
}

TriangleMesh* FunctionalExplicitSurface::getInternalTriangleMesh() const { return internalTriangleMesh; }

/*
Check the general interface contract in superclass method
Geometry.getMinMax.
@return a new 6 valued double array containing the coordinates of a min-max
bounding box for current geometry.
*/
double* FunctionalExplicitSurface::getMinMax() { return internalTriangleMesh ? internalTriangleMesh->getMinMax() : 0; }

/*
Check the general interface contract in superclass method
Geometry.doIntersectionFirstHit.

\todo  Should not delegate work over tesselated geometry version. Should
evaluate directly from algebraic function surface!
@param inOut_Ray
@return true if given ray intersects current FunctionalExplicitSurface
*/
Ray* FunctionalExplicitSurface::doIntersectionFirstHit(const Ray& inOut_Ray)
{
    if (internalTriangleMesh == 0) return 0;
    RayHit hit;
    if (internalTriangleMesh->doIntersectionFirstHit(inOut_Ray, &hit) && hit.getRay() != 0) return new Ray(*hit.getRay());
    return 0;
}

bool FunctionalExplicitSurface::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    return internalTriangleMesh ? internalTriangleMesh->doIntersectionFirstHit(inRay, outHit) : false;
}

/*
Check the general interface contract in superclass method
Geometry.doExtraInformation.
@param inRay
@param inT
@param outData
*/
void FunctionalExplicitSurface::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if (internalTriangleMesh == 0 || outData == 0) return;
    RayHit hit;
    if (internalTriangleMesh->doIntersectionFirstHit(inRay.withT(inT), &hit)) outData->clone(hit);
}

/*
Check the general interface contract in superclass method
Geometry.doContainmentTest.
\todo  Check efficiency for this implementation. Note that for the
special application of volume rendering generation, it is better
to provide another method, to add voxels after a path following
over the line.
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int FunctionalExplicitSurface::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    return internalTriangleMesh ? internalTriangleMesh->doContainmentTest(p, distanceTolerance) : OUTSIDE;
}
