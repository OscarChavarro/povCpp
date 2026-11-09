#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/processing/CurveModeler.h"

ParametricCurve* CurveModeler::createLine(double x1, double y1, double z1,
                                          double x2, double y2, double z2)
{
    ParametricCurve* lineModel;
    java::ArrayList<Vector3Dd> pointParameters;

    lineModel = new ParametricCurve();
    pointParameters.add(Vector3Dd(x1, y1, z1));
    lineModel->addPoint(pointParameters, ParametricCurve::CORNER);

    pointParameters.clear();
    pointParameters.add(Vector3Dd(x2, y2, z2));
    lineModel->addPoint(pointParameters, ParametricCurve::CORNER);

    return lineModel;
}
