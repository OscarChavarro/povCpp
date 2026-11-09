#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/environment/geometry/volume/Arrow.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/gui/gizmo/ReferenceFrameGizmo.h"
#include "vsdk/toolkit/gui/gizmo/TranslateGizmo.h"
#include "vsdk/toolkit/gui/viewport/ViewportElementScaler.h"
#include "vsdk/toolkit/processing/CurveModeler.h"

const double TranslateGizmo::DEFAULT_LINE_WIDTH = 1.0;

namespace {
const double SEGMENT_LENGTH = 0.32;
const double SEGMENT_WIDTH = 0.02;
const double BOX_SIDE = 0.3;
const double BOX_HEIGHT = 0.01;
const double ARROW_LENGTH = 1.0;

double toRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}
}

TranslateGizmo::TranslateGizmo(Camera* cam)
    : hasTransformationMatrix(false), camera(nullptr),
      lineModel3dsmax(nullptr), segmentModel3dsmax(nullptr),
      baseApparentSizeInPixels(DEFAULT_APPARENT_SIZE_IN_PIXELS),
      apparentSizeInPixels(DEFAULT_APPARENT_SIZE_IN_PIXELS),
      lineWidth(DEFAULT_LINE_WIDTH), inputGizmo(3)
{
    ReferenceFrameGizmo axisColors;

    for ( int axis = 0; axis < 3; axis++ ) {
        inputGizmo.setFieldColor(axis, axisColors.getAxisColor(axis));
    }
    persistentSelection = X_AXIS_GROUP;
    volatileSelection = NULL_GROUP;

    // Total arrow length = 0.2 empty + 0.5 base + 0.3 head
    arrowModel = new Arrow(0.5* ARROW_LENGTH, 0.3* ARROW_LENGTH, 0.025, 0.05);
    cylinderModel = new Cone(SEGMENT_WIDTH, SEGMENT_WIDTH, SEGMENT_LENGTH);
    boxModel = new Box(BOX_SIDE, BOX_SIDE, BOX_HEIGHT);
    coneModel = new Cone(0.05, 0, 0.3* ARROW_LENGTH);

    for ( int i = 0; i < 12; i++ ) {
        elements.add(new SimpleBody());
    }

    for ( int i = 0; i < 15; i++ ) {
        elementInstances3dsmax.add(new SimpleBody());
    }

    setCamera(cam);
    selectedResizing = true;
    currentScale = 1.0;
}

TranslateGizmo::~TranslateGizmo()
{
    long i;
    for ( i = 0; i < elements.size(); i++ ) {
        delete elements.get(i);
    }
    for ( i = 0; i < elementInstances3dsmax.size(); i++ ) {
        delete elementInstances3dsmax.get(i);
    }
    delete arrowModel;
    delete cylinderModel;
    delete boxModel;
    delete coneModel;
    delete lineModel3dsmax;
    delete segmentModel3dsmax;
}

int TranslateGizmo::getApparentSizeInPixels() const
{
    return apparentSizeInPixels;
}

void TranslateGizmo::setApparentSizeInPixels(int du)
{
    apparentSizeInPixels = du;
}

int TranslateGizmo::getBaseApparentSizeInPixels() const
{
    return baseApparentSizeInPixels;
}

void TranslateGizmo::setBaseApparentSizeInPixels(int size)
{
    if ( size > 0 ) {
        baseApparentSizeInPixels = size;
    }
}

double TranslateGizmo::getLineWidth() const
{
    return lineWidth;
}

void TranslateGizmo::setLineWidth(double lineWidth)
{
    if ( lineWidth > 0.0 ) {
        this->lineWidth = lineWidth;
    }
}

void TranslateGizmo::applyScale(const ViewportElementScaler* scaler)
{
    if ( scaler == nullptr ) {
        return;
    }
    setApparentSizeInPixels(scaler->scaleSize(baseApparentSizeInPixels));
    setLineWidth(scaler->scaleLength(DEFAULT_LINE_WIDTH));
}

double TranslateGizmo::getLineWidthInWorldUnits() const
{
    return lineWidth * currentScale / apparentSizeInPixels;
}

java::ArrayList<TranslateGizmoLineSegment>
TranslateGizmo::getLineSegments() const
{
    java::ArrayList<TranslateGizmoLineSegment> segments;
    Vector3Dd zAxis(0, 0, 1);
    long i;

    for ( i = 0; i < elements.size(); i++ ) {
        SimpleBody* element = elements.get(i);
        Geometry* g = element->getGeometry();
        double length;

        if ( g == arrowModel ) {
            length = currentScale*0.5* ARROW_LENGTH;
        }
        else if ( g == cylinderModel ) {
            length = currentScale* SEGMENT_LENGTH;
        }
        else {
            continue;
        }

        Vector3Dd start = element->getPosition();
        Vector3Dd end = start.add(
            element->getRotation().multiply(zAxis).multiply(length));
        ColorRgb color;
        if ( element->getMaterial() != nullptr ) {
            color = element->getMaterial()->getDiffuse();
        }

        segments.add(TranslateGizmoLineSegment(start, end, color));
    }
    return segments;
}

bool TranslateGizmo::buildLineStrip(const TranslateGizmoLineSegment& segment,
                                    Vector3Dd outStrip[4]) const
{
    Vector3Dd direction = segment.end().subtract(segment.start());
    double length = direction.length();

    if ( length < VSDK::EPSILON || camera == nullptr ) {
        return false;
    }
    direction = direction.multiply(1/length);

    // Direction from the eye to the line, to face the camera
    Vector3Dd view;

    if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
        view = camera->getFront();
    }
    else {
        view = segment.start().add(segment.end()).multiply(0.5).subtract(
            camera->getPosition());
    }

    Vector3Dd side = direction.crossProduct(view);

    if ( side.length() < VSDK::EPSILON ) {
        // Line pointing to the viewer: any direction in screen is valid
        side = camera->getUp();
    }

    double halfWidth = getLineWidthInWorldUnits()/2;
    side = side.normalized().multiply(halfWidth);

    Vector3Dd start = segment.start().subtract(direction.multiply(halfWidth));
    Vector3Dd end = segment.end().add(direction.multiply(halfWidth));

    outStrip[0] = start.add(side);
    outStrip[1] = start.subtract(side);
    outStrip[2] = end.add(side);
    outStrip[3] = end.subtract(side);
    return true;
}

double TranslateGizmo::getCurrentScale() const
{
    return currentScale;
}

void TranslateGizmo::setCamera(Camera* cam)
{
    camera = cam;
}

Camera* TranslateGizmo::getCamera() const
{
    return camera;
}

java::ArrayList<SimpleBody*>& TranslateGizmo::getElements()
{
    return elements;
}

java::ArrayList<SimpleBody*>& TranslateGizmo::getElements3dsmax()
{
    int i;
    SimpleBody* r;
    SimpleBody* o;
    SimpleBody* r2;
    Geometry* g;
    SimpleMaterial red = createMaterial(0.78, 0, 0);
    SimpleMaterial green = createMaterial(0, 0.61, 0);
    SimpleMaterial blue = createMaterial(0, 0, 0.76);

    Matrix4x4d R = transformationMatrix.withoutTranslation();
    Matrix4x4d subR;
    Matrix4x4d eleR;
    Vector3Dd subP;
    Vector3Dd eleP;

    coneModel->setBottomRadius(currentScale*0.05);
    coneModel->setHeight(currentScale*0.3* ARROW_LENGTH);
    boxModel->setSize(currentScale*(BOX_SIDE+0.025),
                      currentScale*(BOX_SIDE+0.025), currentScale*BOX_HEIGHT);

    //-----------------------------------------------------------------
    ParametricCurve* lineModel;
    ParametricCurve* segmentModel;

    lineModel = CurveModeler::createLine(0, 0, 0,
        0, 0, currentScale*0.7);

    segmentModel = CurveModeler::createLine(0, 0, 0,
        0, 0, currentScale* SEGMENT_LENGTH);

    //-----------------------------------------------------------------
    for ( i = 0; i < elements.size(); i++ ) {
        r = elementInstances3dsmax.get(i);
        o = elements.get(i);

        r->setMaterial(o->getMaterial() != nullptr ?
            new SimpleMaterial(*o->getMaterial()) : nullptr);
        r->setRotation(o->getRotation());
        r->setRotationInverse(o->getRotationInverse());

        g = o->getGeometry();
        if ( g != nullptr && dynamic_cast<Arrow*>(g) != nullptr ) {
            r->setGeometryReference(coneModel);
            switch ( i ) {
              case 0:
                // Rotation
                subR = subR.axisRotation(toRadians(90.0), 0, 1, 0);
                eleR = R.multiply(subR);
                r->setRotation(eleR);
                r->setRotationInverse(eleR.invert());
                // Translation
                subP = Vector3Dd(0, 0, currentScale*0.7* ARROW_LENGTH);
                eleP = eleR.multiply(subP).add(getPosition());
                r->setPosition(eleP);
                r->setMaterial(new SimpleMaterial(red));
                break;
              case 1:
                // Rotation
                subR = subR.axisRotation(toRadians(90.0), -1, 0, 0);
                eleR = R.multiply(subR);
                r->setRotation(eleR);
                r->setRotationInverse(eleR.invert());
                // Translation
                subP = Vector3Dd(0, 0, currentScale*0.7* ARROW_LENGTH);
                eleP = eleR.multiply(subP).add(getPosition());
                r->setPosition(eleP);
                r->setMaterial(new SimpleMaterial(green));
                break;
              case 2:
                // Rotation
                subR = Matrix4x4d();
                eleR = R.multiply(subR);
                r->setRotation(eleR);
                r->setRotationInverse(eleR.invert());
                // Translation
                subP = Vector3Dd(0, 0, currentScale*0.7* ARROW_LENGTH);
                eleP = eleR.multiply(subP).add(getPosition());
                r->setPosition(eleP);
                r->setMaterial(new SimpleMaterial(blue));
                break;
            }

            r2 = elementInstances3dsmax.get(i+12);
            r2->setMaterial(o->getMaterial() != nullptr ?
                new SimpleMaterial(*o->getMaterial()) : nullptr);
            r2->setRotation(o->getRotation());
            r2->setRotationInverse(o->getRotationInverse());
            r2->setPosition(o->getPosition());
            r2->setGeometryReference(lineModel);
        }
        else if ( g != nullptr && dynamic_cast<Cone*>(g) != nullptr ) {
            r->setPosition(o->getPosition());
            r->setGeometryReference(segmentModel);
        }
        else {
            r->setPosition(o->getPosition());
            r->setGeometryReference(g);
        }
    }

    // Instances not updated in this call (as the line of an axis hidden
    // because it points to the viewer of an orthogonal camera) still refer
    // to the previous curves; in Java the garbage collector keeps those
    // alive, here they are moved to the new ones before deleting the old
    for ( i = 0; i < elementInstances3dsmax.size(); i++ ) {
        r = elementInstances3dsmax.get(i);
        if ( lineModel3dsmax != nullptr &&
             r->getGeometry() == lineModel3dsmax ) {
            r->setGeometryReference(lineModel);
        }
        else if ( segmentModel3dsmax != nullptr &&
                  r->getGeometry() == segmentModel3dsmax ) {
            r->setGeometryReference(segmentModel);
        }
    }
    delete lineModel3dsmax;
    delete segmentModel3dsmax;
    lineModel3dsmax = lineModel;
    segmentModel3dsmax = segmentModel;

    return elementInstances3dsmax;
}

SimpleMaterial TranslateGizmo::createMaterial(double r, double g, double b)
{
    SimpleMaterial m;

    m = m.withAmbient(ColorRgb(0.2, 0.2, 0.2));
    m = m.withDiffuse(ColorRgb(r, g, b));
    m = m.withSpecular(ColorRgb(1, 1, 1));
    return m;
}

void TranslateGizmo::placeElement(SimpleBody* r, const Matrix4x4d& rotation,
                                  const Matrix4x4d& subR,
                                  const Vector3Dd& subP,
                                  bool positionFromElementRotation) const
{
    Matrix4x4d eleR = rotation.multiply(subR);
    r->setRotation(eleR);
    r->setRotationInverse(eleR.invert());
    Vector3Dd eleP;
    if ( positionFromElementRotation ) {
        eleP = eleR.multiply(subP).add(getPosition());
    }
    else {
        eleP = rotation.multiply(subP).add(getPosition());
    }
    r->setPosition(eleP);
}

void TranslateGizmo::calculateGeometryState(const Vector3Dd& /*translation*/,
                                            const Matrix4x4d& /*rotation*/,
                                            bool /*autosize*/,
                                            int initialdu, Camera* camera)
{
    //-----------------------------------------------------------------
    int i;
    SimpleMaterial red = createMaterial(0.78, 0, 0);
    SimpleMaterial green = createMaterial(0, 0.61, 0);
    SimpleMaterial blue = createMaterial(0, 0, 0.76);
    SimpleMaterial yellow = createMaterial(1, 1, 0);
    SimpleMaterial yellowTransparent = createMaterial(1, 1, 0);

    yellowTransparent = yellowTransparent.withOpacity(0.2);

    if ( camera == nullptr ) {
        return;
    }

    int currentSelection;
    if ( volatileSelection == NULL_GROUP ) {
        currentSelection = persistentSelection;
    }
    else {
        currentSelection = volatileSelection;
    }

    Matrix4x4d R = transformationMatrix.withoutTranslation();
    Matrix4x4d subR;

    camera->updateVectors();

    if ( selectedResizing ) {
        Vector3Dd p = getPosition();
        Vector3Dd right = camera->getLeft().multiply(-1);

        right = right.normalized();
        Vector3Dd a;
        Vector3Dd b;

        // Keeps the last scale if the size in pixels can not be measured
        if ( camera->projectPointUsingRayMethod(p, &a) &&
             camera->projectPointUsingRayMethod(p.add(right), &b) ) {
            double factor = a.subtract(b).length();

            if ( factor > VSDK::EPSILON ) {
                currentScale = ((double)initialdu)/factor;
            }
        }
    }
    double scale = currentScale;

    arrowModel->setBaseLength(scale*0.5* ARROW_LENGTH);
    arrowModel->setHeadLength(scale*0.3* ARROW_LENGTH);
    arrowModel->setBaseRadius(scale*0.025);
    arrowModel->setHeadRadius(scale*0.05);
    cylinderModel->setBottomRadius(scale*SEGMENT_WIDTH);
    cylinderModel->setTopRadius(scale*SEGMENT_WIDTH);
    cylinderModel->setHeight(scale* SEGMENT_LENGTH);
    boxModel->setSize(scale*BOX_SIDE, scale*BOX_SIDE, scale*BOX_HEIGHT);

    //-----------------------------------------------------------------
    Vector3Dd front = camera->getFront();
    Vector3Dd axisI(1, 0, 0);
    Vector3Dd axisJ(0, 1, 0);
    Vector3Dd axisK(0, 0, 1);
    bool orthogonalCamera;
    bool iPar;
    bool jPar;
    bool kPar;

    orthogonalCamera = (camera->getProjectionMode() ==
                        Camera::PROJECTION_MODE_ORTHOGONAL);
    iPar = (std::fabs(front.dotProduct(axisI)) > 1.0-VSDK::EPSILON);
    jPar = (std::fabs(front.dotProduct(axisJ)) > 1.0-VSDK::EPSILON);
    kPar = (std::fabs(front.dotProduct(axisK)) > 1.0-VSDK::EPSILON);

    Matrix4x4d identity;
    Matrix4x4d rotY = subR.axisRotation(toRadians(90.0), 0, 1, 0);
    Matrix4x4d rotMinusX = subR.axisRotation(toRadians(90.0), -1, 0, 0);
    Matrix4x4d rotX = subR.axisRotation(toRadians(90.0), 1, 0, 0);

    int index;
    for ( i = 0, index = 1;
          index <= 12 && i < elements.size();
          index++, i++ ) {
        SimpleBody* r = elements.get(i);
        r->setGeometryReference(nullptr);
        switch ( index ) {
          case X_AXIS_ELEMENT:
            if ( !(orthogonalCamera && iPar) ) {
                // Basic model
                r->setGeometryReference(arrowModel);
                if ( currentSelection == X_AXIS_GROUP ||
                     currentSelection == XY_PLANE_GROUP ||
                     currentSelection == XZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(red));
                }
                placeElement(r, R, rotY,
                    Vector3Dd(0, 0, scale*0.2* ARROW_LENGTH), true);
            }
            break;
          case Y_AXIS_ELEMENT:
            if ( !(orthogonalCamera && jPar) ) {
                // Basic model
                r->setGeometryReference(arrowModel);
                if ( currentSelection == Y_AXIS_GROUP ||
                     currentSelection == XY_PLANE_GROUP ||
                     currentSelection == YZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(green));
                }
                placeElement(r, R, rotMinusX,
                    Vector3Dd(0, 0, scale*0.2* ARROW_LENGTH), true);
            }
            break;
          case Z_AXIS_ELEMENT:
            if ( !(orthogonalCamera && kPar) ) {
                // Basic model
                r->setGeometryReference(arrowModel);
                if ( currentSelection == Z_AXIS_GROUP ||
                     currentSelection == YZ_PLANE_GROUP ||
                     currentSelection == XZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(blue));
                }
                placeElement(r, R, identity,
                    Vector3Dd(0, 0, scale*0.2* ARROW_LENGTH), true);
            }
            break;
          case XYY_SEGMENT_ELEMENT:
            if ( !(orthogonalCamera && (iPar || jPar)) ) {
                // Basic model
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == XY_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(green));
                }
                placeElement(r, R, rotY,
                    Vector3Dd(0, scale* SEGMENT_LENGTH, 0), true);
            }
            break;
          case XYX_SEGMENT_ELEMENT:
            if ( !(orthogonalCamera && (iPar || jPar)) ) {
                // Basic model
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == XY_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(red));
                }
                placeElement(r, R, rotMinusX,
                    Vector3Dd(scale* SEGMENT_LENGTH, 0, 0), true);
            }
            break;
          case YZZ_SEGMENT_ELEMENT:
            // Basic model
            if ( !(orthogonalCamera && (jPar || kPar)) ) {
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == YZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(blue));
                }
                placeElement(r, R, rotMinusX,
                    Vector3Dd(0, 0, scale* SEGMENT_LENGTH), false);
            }
            break;
          case YZY_SEGMENT_ELEMENT:
            if ( !(orthogonalCamera && (jPar || kPar)) ) {
                // Basic model
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == YZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(green));
                }
                placeElement(r, R, identity,
                    Vector3Dd(0, scale* SEGMENT_LENGTH, 0), false);
            }
            break;
          case XZZ_SEGMENT_ELEMENT:
            if ( !(orthogonalCamera && (iPar || kPar)) ) {
                // Basic model
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == XZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(blue));
                }
                placeElement(r, R, rotY,
                    Vector3Dd(0, 0, scale* SEGMENT_LENGTH), false);
            }
            break;
          case XZX_SEGMENT_ELEMENT:
            if ( !(orthogonalCamera && (iPar || kPar)) ) {
                // Basic model
                r->setGeometryReference(cylinderModel);
                if ( currentSelection == XZ_PLANE_GROUP ) {
                    r->setMaterial(new SimpleMaterial(yellow));
                }
                else {
                    r->setMaterial(new SimpleMaterial(red));
                }
                placeElement(r, R, identity,
                    Vector3Dd(scale* SEGMENT_LENGTH, 0, 0), false);
            }
            break;
          case XY_BOX_ELEMENT:
            if ( !(orthogonalCamera && (iPar || jPar)) ) {
                // Basic model
                r->setGeometryReference(nullptr);
                if ( currentSelection != XY_PLANE_GROUP ) {
                    break;
                }
                r->setGeometryReference(boxModel);
                r->setMaterial(new SimpleMaterial(yellowTransparent));
                placeElement(r, R, identity,
                    Vector3Dd(scale*BOX_SIDE/2, scale*BOX_SIDE/2, 0), false);
            }
            break;
          case YZ_BOX_ELEMENT:
            if ( !(orthogonalCamera && (jPar || kPar)) ) {
                // Basic model
                r->setGeometryReference(nullptr);
                if ( currentSelection != YZ_PLANE_GROUP ) {
                    break;
                }
                r->setGeometryReference(boxModel);
                r->setMaterial(new SimpleMaterial(yellowTransparent));
                placeElement(r, R, rotY,
                    Vector3Dd(0, scale*BOX_SIDE/2, scale*BOX_SIDE/2), false);
            }
            break;
          case XZ_BOX_ELEMENT:
            if ( !(orthogonalCamera && (iPar || kPar)) ) {
                // Basic model
                r->setGeometryReference(nullptr);
                if ( currentSelection != XZ_PLANE_GROUP ) {
                    break;
                }
                r->setGeometryReference(boxModel);
                r->setMaterial(new SimpleMaterial(yellowTransparent));
                placeElement(r, R, rotX,
                    Vector3Dd(scale*BOX_SIDE/2, 0, scale*BOX_SIDE/2), false);
            }
            break;
        }
    }
}

Vector3Dd TranslateGizmo::getPosition() const
{
    return transformationMatrix.extractTranslation();
}

void TranslateGizmo::setPosition(const Vector3Dd& p)
{
    transformationMatrix = transformationMatrix.withTranslation(p);
    hasTransformationMatrix = true;
}

void TranslateGizmo::setTransformationMatrix(
    const Matrix4x4d& transformationMatrix)
{
    this->transformationMatrix = transformationMatrix;
    hasTransformationMatrix = true;

    Matrix4x4d R = transformationMatrix.withoutTranslation();
    calculateGeometryState(getPosition(),
                           R, selectedResizing, apparentSizeInPixels,
                           camera);
}

Matrix4x4d TranslateGizmo::getTransformationMatrix() const
{
    return transformationMatrix;
}

void TranslateGizmo::updateGeometryState()
{
    Matrix4x4d R = transformationMatrix.withoutTranslation();

    calculateGeometryState(getPosition(), R, selectedResizing,
        apparentSizeInPixels, camera);
}

int TranslateGizmo::getPersistentSelection() const
{
    return persistentSelection;
}

void TranslateGizmo::setPersistentSelection(int selection)
{
    persistentSelection = selection;
}

int TranslateGizmo::getVolatileSelection() const
{
    return volatileSelection;
}

void TranslateGizmo::setVolatileSelection(int selection)
{
    volatileSelection = selection;
}

int TranslateGizmo::getCurrentSelection() const
{
    if ( volatileSelection == NULL_GROUP ) {
        return persistentSelection;
    }
    return volatileSelection;
}

InputGizmo* TranslateGizmo::getInputGizmo()
{
    if ( hasTransformationMatrix ) {
        Vector3Dd position = getPosition();

        inputGizmo.setValue(0, position.x());
        inputGizmo.setValue(1, position.y());
        inputGizmo.setValue(2, position.z());
    }
    for ( int axis = 0; axis < 3; axis++ ) {
        inputGizmo.setFieldHighlighted(axis, isAxisHighlighted(axis));
    }
    return &inputGizmo;
}

bool TranslateGizmo::isAxisHighlighted(int axis) const
{
    int currentSelection = getCurrentSelection();

    switch ( axis ) {
      case 0:
        return currentSelection == X_AXIS_GROUP ||
            currentSelection == XY_PLANE_GROUP ||
            currentSelection == XZ_PLANE_GROUP;
      case 1:
        return currentSelection == Y_AXIS_GROUP ||
            currentSelection == XY_PLANE_GROUP ||
            currentSelection == YZ_PLANE_GROUP;
      case 2:
        return currentSelection == Z_AXIS_GROUP ||
            currentSelection == YZ_PLANE_GROUP ||
            currentSelection == XZ_PLANE_GROUP;
      default:
        return false;
    }
}

bool TranslateGizmo::isSelectedResizing() const
{
    return selectedResizing;
}

void TranslateGizmo::setSelectedResizing(bool selectedResizing)
{
    this->selectedResizing = selectedResizing;
}
