#include "java/lang/Math.h"
#include "java/lang/RuntimeException.h"
#include "java/util/function/Function.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/Arrow.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/gui/gizmo/RayGizmo.h"

const double RayGizmo::DEFAULT_DISABLE_TIME = 2.0;
const double RayGizmo::ARROW_BASE_LENGTH = 3.0;
const double RayGizmo::ARROW_HEAD_LENGTH = 1.0;
const double RayGizmo::ARROW_BASE_RADIUS = 0.15;
const double RayGizmo::ARROW_HEAD_RADIUS = 0.40;
const ColorRgb RayGizmo::DEFAULT_RAY_COLOR = ColorRgb(1, 0.8, 0.8);
const ColorRgb RayGizmo::DEFAULT_NORMAL_COLOR = ColorRgb(1, 1, 0.8);
const ColorRgb RayGizmo::INPUT_FIELD_COLORS[RayGizmo::NUMBER_OF_INPUT_FIELDS] = {
    ColorRgb(1, 0, 0),
    ColorRgb(0, 1, 0),
    ColorRgb(0, 0, 1),
    ColorRgb(1, 1, 0),
    ColorRgb(0, 1, 1)
};

RayGizmo::RaySnapshot::RaySnapshot()
    : rotationAngleInRadians(0.0)
{
}

RayGizmo::RaySnapshot::RaySnapshot(
    double rotationAngleInRadians,
    const java::ArrayList<Ray>& rays,
    const java::ArrayList<Intersection*>& intersections)
    : rotationAngleInRadians(rotationAngleInRadians)
{
    if ( rays.size() != intersections.size() ) {
        throw java::RuntimeException("rays and intersections must be the same size");
    }
    for ( long i = 0; i < rays.size(); i++ ) {
        this->rays.add(rays.get(i));
        Intersection* p = intersections.get(i);
        this->intersections.add(p == 0 ? 0 : new Intersection(*p));
    }
}

RayGizmo::RaySnapshot::RaySnapshot(const RaySnapshot& other)
    : rotationAngleInRadians(other.rotationAngleInRadians)
{
    for ( long i = 0; i < other.rays.size(); i++ ) {
        rays.add(other.rays.get(i));
        Intersection* p = other.intersections.get(i);
        intersections.add(p == 0 ? 0 : new Intersection(*p));
    }
}

RayGizmo::RaySnapshot& RayGizmo::RaySnapshot::operator=(const RaySnapshot& other)
{
    if ( this == &other ) {
        return *this;
    }
    clear();
    rotationAngleInRadians = other.rotationAngleInRadians;
    for ( long i = 0; i < other.rays.size(); i++ ) {
        rays.add(other.rays.get(i));
        Intersection* p = other.intersections.get(i);
        intersections.add(p == 0 ? 0 : new Intersection(*p));
    }
    return *this;
}

RayGizmo::RaySnapshot::~RaySnapshot()
{
    clear();
}

void RayGizmo::RaySnapshot::clear()
{
    for ( long i = 0; i < intersections.size(); i++ ) {
        delete intersections.get(i);
    }
    intersections.clear();
    rays.clear();
}

RayGizmo::RayGizmo(java::Function<const Ray&, Intersection*> intersectionCallback, int maxNumOfReflections)
    : arrow(new Arrow(ARROW_BASE_LENGTH, ARROW_HEAD_LENGTH, ARROW_BASE_RADIUS, ARROW_HEAD_RADIUS)),
      dotSphere(new Sphere(ARROW_BASE_RADIUS)),
      body(new SimpleBody()),
      intersectionCallback(intersectionCallback),
      inputGizmo(5, 3, 3),
      maxNumOfReflections(maxNumOfReflections),
      pendingSnapshot(0),
      currentPosition(0, 0, 0),
      currentDirection(0, 0, 1),
      currentRotationAngleInRadians(0.0),
      currentSnapshot(0),
      lastDataTime(std::time(0)),
      previousDataTime(std::time(0)),
      visible(true),
      rollIndicatorEnabled(false),
      disableAfterElapsedSeconds(DEFAULT_DISABLE_TIME),
      sourceRayColor(DEFAULT_RAY_COLOR)
{
    pthread_mutex_init(&pendingMutex, 0);

    body->setGeometry(arrow);

    SimpleMaterial* mat = new SimpleMaterial();
    *mat = mat->withAmbient(ColorRgb(0.1, 0.0, 0.0));
    *mat = mat->withDiffuse(ColorRgb(0.9, 0.2, 0.1));
    *mat = mat->withSpecular(ColorRgb(1.0, 1.0, 1.0));
    *mat = mat->withPhongExponent(32.0);
    body->setMaterial(mat);

    for ( int i = 0; i < NUMBER_OF_INPUT_FIELDS; i++ ) {
        inputGizmo.setFieldColor(i, INPUT_FIELD_COLORS[i]);
    }

    currentPosition = Vector3Dd(0, 0, 0);
    currentDirection = Vector3Dd(0, 0, 1);
    applyTransform(currentPosition, currentDirection);
    syncInputGizmo(currentPosition, currentDirection);

    normalRayColors.add(DEFAULT_NORMAL_COLOR);
    reflectedRayColors.add(DEFAULT_RAY_COLOR);
    refractedRayColors.add(DEFAULT_RAY_COLOR);
}

RayGizmo::~RayGizmo()
{
    pthread_mutex_lock(&pendingMutex);
    delete pendingSnapshot;
    pendingSnapshot = 0;
    pthread_mutex_unlock(&pendingMutex);
    pthread_mutex_destroy(&pendingMutex);

    delete currentSnapshot;
    delete dotSphere;
    delete body;
}

Vector3Dd RayGizmo::getPosition() const
{
    return currentPosition;
}

Vector3Dd RayGizmo::getDirection() const
{
    return currentDirection;
}

void RayGizmo::setRay(const Ray& ray, double rotationAngleInRadians)
{
    java::ArrayList<Ray> rays;
    java::ArrayList<Intersection*> intersections;

    Intersection* primaryIntersection = intersectionCallback ? intersectionCallback.apply(ray) : 0;
    rays.add(ray);
    intersections.add(primaryIntersection);

    if ( intersectionCallback && primaryIntersection != 0 ) {
        Ray currentRay = ray;
        Intersection* currentIntersection = primaryIntersection;
        for ( int i = 0; i < maxNumOfReflections; i++ ) {
            Ray* reflectedRay = computeReflectedRay(currentRay, currentIntersection);
            if ( reflectedRay == 0 ) {
                break;
            }
            Intersection* reflectedIntersection = intersectionCallback.apply(*reflectedRay);
            rays.add(*reflectedRay);
            intersections.add(reflectedIntersection);
            delete reflectedRay;
            if ( reflectedIntersection == 0 ) {
                break;
            }
            currentRay = rays.get(rays.size() - 1);
            currentIntersection = reflectedIntersection;
        }
    }

    RaySnapshot* snap = new RaySnapshot(rotationAngleInRadians, rays, intersections);
    for ( long i = 0; i < intersections.size(); i++ ) {
        delete intersections.get(i);
    }

    pthread_mutex_lock(&pendingMutex);
    delete pendingSnapshot;
    pendingSnapshot = snap;
    pthread_mutex_unlock(&pendingMutex);

    syncInputGizmo(ray.getOrigin(), ray.getDirection());
    visible = true;
    recordDataArrival();
}

int RayGizmo::getMaxNumOfReflections() const
{
    return maxNumOfReflections;
}

void RayGizmo::setMaxNumOfReflections(int maxNumOfReflections)
{
    this->maxNumOfReflections =
        maxNumOfReflections < 0 ? 0 : maxNumOfReflections;
}

InputGizmo* RayGizmo::getInputGizmo()
{
    return &inputGizmo;
}

void RayGizmo::update()
{
    if ( inactivityThresholdExceeded() ) {
        visible = false;
    }
    previousDataTime = lastDataTime;
}

bool RayGizmo::isVisible() const
{
    return visible;
}

void RayGizmo::setVisible(bool visible)
{
    this->visible = visible;
}

bool RayGizmo::isRollIndicatorEnabled() const
{
    return rollIndicatorEnabled;
}

void RayGizmo::setRollIndicatorEnabled(bool enabled)
{
    rollIndicatorEnabled = enabled;
}

double RayGizmo::getDisableAfterElapsedSeconds() const
{
    return disableAfterElapsedSeconds;
}

void RayGizmo::setDisableAfterElapsedSeconds(double disableAfterElapsedSeconds)
{
    this->disableAfterElapsedSeconds = disableAfterElapsedSeconds;
}

std::time_t RayGizmo::getLastDataTime() const
{
    return lastDataTime;
}

std::time_t RayGizmo::getPreviousDataTime() const
{
    return previousDataTime;
}

RayGizmo::RaySnapshot* RayGizmo::acquireSnapshot()
{
    pthread_mutex_lock(&pendingMutex);
    RaySnapshot* snap = pendingSnapshot;
    pendingSnapshot = 0;
    pthread_mutex_unlock(&pendingMutex);

    if ( snap == 0 ) {
        return 0;
    }
    if ( snap->rays.size() > 0 ) {
        Ray primary = snap->rays.get(0);
        applyTransform(primary.getOrigin(), primary.getDirection());
        syncInputGizmo(primary.getOrigin(), primary.getDirection());
    }
    currentRotationAngleInRadians = snap->rotationAngleInRadians;
    delete currentSnapshot;
    currentSnapshot = new RaySnapshot(*snap);
    delete snap;
    return currentSnapshot;
}

RayGizmo::RaySnapshot* RayGizmo::getCurrentSnapshot() const
{
    return currentSnapshot;
}

double RayGizmo::getRotationAngleInRadians() const
{
    return currentRotationAngleInRadians;
}

SimpleBody* RayGizmo::getBody() const
{
    return body;
}

Arrow* RayGizmo::getArrow() const
{
    return arrow;
}

SimpleScene* RayGizmo::buildScene() const
{
    SimpleScene* scene = new SimpleScene();
    double arrowTotalLength = ARROW_BASE_LENGTH + ARROW_HEAD_LENGTH;

    if ( currentSnapshot == 0 || currentSnapshot->rays.size() == 0 ) {
        scene->addBody(buildRayBody(
            Ray(currentPosition, currentDirection), 0,
            sourceRayColor, arrowTotalLength));
        return scene;
    }

    Ray sourceRay = currentSnapshot->rays.get(0);
    Intersection* sourceIntersection = currentSnapshot->intersections.get(0);
    scene->addBody(buildRayBody(sourceRay, sourceIntersection, sourceRayColor, arrowTotalLength));

    if ( sourceIntersection == 0 ) {
        addDotBodies(scene, sourceRay, arrowTotalLength);
    }
    else {
        addNormalBody(scene, sourceIntersection, normalRayColors.get(0 % normalRayColors.size()));
    }

    for ( long i = 1; i < currentSnapshot->rays.size(); i++ ) {
        Ray reflRay = currentSnapshot->rays.get(i);
        Intersection* reflIntersection = currentSnapshot->intersections.get(i);
        ColorRgb reflColor = reflectedRayColors.get((i - 1) % reflectedRayColors.size());
        scene->addBody(buildRayBody(reflRay, reflIntersection, reflColor, arrowTotalLength));

        if ( reflIntersection == 0 ) {
            addDotBodies(scene, reflRay, arrowTotalLength);
        }
        else {
            addNormalBody(scene, reflIntersection, normalRayColors.get(i % normalRayColors.size()));
        }
    }

    return scene;
}

SimpleBody* RayGizmo::buildRayBody(
    const Ray& ray, const Intersection* intersection, const ColorRgb& color,
    double arrowTotalLength) const
{
    double scaleZ = 1.0;
    if ( intersection != 0 ) {
        double hitT = intersection->t;
        if ( hitT > 1e-6 && arrowTotalLength > 1e-6 ) {
            scaleZ = hitT / arrowTotalLength;
        }
    }
    return buildBody(
        new Arrow(ARROW_BASE_LENGTH, ARROW_HEAD_LENGTH, ARROW_BASE_RADIUS, ARROW_HEAD_RADIUS),
        ray.getOrigin(),
        rotationFromZToDirection(ray.getDirection()),
        Vector3Dd(1.0, 1.0, scaleZ),
        color);
}

void RayGizmo::addNormalBody(SimpleScene* scene, const Intersection* intersection, const ColorRgb& color) const
{
    if ( intersection == 0 ) {
        return;
    }
    scene->addBody(buildBody(
        new Arrow(ARROW_BASE_LENGTH, ARROW_HEAD_LENGTH, ARROW_BASE_RADIUS, ARROW_HEAD_RADIUS),
        intersection->point,
        rotationFromZToDirection(intersection->normal),
        Vector3Dd(0.5, 0.5, 1.0),
        color));
}

void RayGizmo::addDotBodies(SimpleScene* scene, const Ray& ray, double arrowTotalLength) const
{
    double len = ray.getDirection().length();
    if ( len < VSDK::EPSILON ) {
        return;
    }
    Vector3Dd dir = ray.getDirection().multiply(1.0 / len);
    Vector3Dd origin = ray.getOrigin();
    double factors[] = { 1.25, 1.50, 1.75 };
    for ( int i = 0; i < 3; i++ ) {
        Vector3Dd dotPos = origin.add(dir.multiply(arrowTotalLength * factors[i]));
        scene->addBody(buildDotBody(new Sphere(ARROW_BASE_RADIUS), dotPos, ColorRgb(1.0, 1.0, 0.0)));
    }
}

SimpleBody* RayGizmo::buildBody(
    Arrow* geometry, const Vector3Dd& position, const Matrix4x4d& rotation,
    const Vector3Dd& scale, const ColorRgb& color)
{
    SimpleBody* b = new SimpleBody();
    b->setGeometry(geometry);
    b->setPosition(position);
    b->setRotation(rotation);
    b->setScale(scale);
    b->setMaterial(materialFromColor(color));
    return b;
}

SimpleBody* RayGizmo::buildDotBody(Sphere* geometry, const Vector3Dd& position, const ColorRgb& color)
{
    SimpleBody* b = new SimpleBody();
    b->setGeometry(geometry);
    b->setPosition(position);
    b->setMaterial(materialFromColor(color));
    return b;
}

SimpleMaterial* RayGizmo::materialFromColor(const ColorRgb& color)
{
    SimpleMaterial* m = new SimpleMaterial();
    *m = m->withAmbient(ColorRgb(color.r() * 0.1, color.g() * 0.1, color.b() * 0.1));
    *m = m->withDiffuse(color);
    *m = m->withSpecular(ColorRgb(1.0, 1.0, 1.0));
    *m = m->withPhongExponent(32.0);
    return m;
}

ColorRgb RayGizmo::getSourceRayColor() const
{
    return sourceRayColor;
}

void RayGizmo::setSourceRayColor(const ColorRgb& sourceRayColor)
{
    this->sourceRayColor = sourceRayColor;
}

java::ArrayList<ColorRgb>& RayGizmo::getNormalRayColors()
{
    return normalRayColors;
}

void RayGizmo::setNormalRayColors(const java::ArrayList<ColorRgb>& normalRayColors)
{
    this->normalRayColors = normalRayColors;
}

java::ArrayList<ColorRgb>& RayGizmo::getReflectedRayColors()
{
    return reflectedRayColors;
}

void RayGizmo::setReflectedRayColors(const java::ArrayList<ColorRgb>& reflectedRayColors)
{
    this->reflectedRayColors = reflectedRayColors;
}

java::ArrayList<ColorRgb>& RayGizmo::getRefractedRayColors()
{
    return refractedRayColors;
}

void RayGizmo::setRefractedRayColors(const java::ArrayList<ColorRgb>& refractedRayColors)
{
    this->refractedRayColors = refractedRayColors;
}

void RayGizmo::recordDataArrival()
{
    previousDataTime = lastDataTime;
    lastDataTime = std::time(0);
}

bool RayGizmo::inactivityThresholdExceeded() const
{
    double elapsed = std::difftime(std::time(0), lastDataTime);
    return disableAfterElapsedSeconds > 0.0 && elapsed > disableAfterElapsedSeconds;
}

void RayGizmo::applyTransform(const Vector3Dd& position, const Vector3Dd& direction)
{
    currentPosition = position;
    currentDirection = direction;

    Matrix4x4d rotation = rotationFromZToDirection(direction);
    Matrix4x4d rotationInverse = rotation.invert();

    body->setPosition(position);
    body->setRotation(rotation);
    body->setRotationInverse(rotationInverse);
}

void RayGizmo::syncInputGizmo(const Vector3Dd& position, const Vector3Dd& direction)
{
    Vector3Dd d = direction.length() > VSDK::EPSILON ?
        direction.normalized() : Vector3Dd(0, 0, 1);
    double yaw = std::atan2(d.y(), d.x());
    double pitch = std::asin(java::Math::max(-1.0, java::Math::min(1.0, d.z())));

    inputGizmo.setValue(0, position.x());
    inputGizmo.setValue(1, position.y());
    inputGizmo.setValue(2, position.z());
    inputGizmo.setValue(3, yaw * 180.0 / M_PI);
    inputGizmo.setValue(4, pitch * 180.0 / M_PI);
}

Ray* RayGizmo::computeReflectedRay(const Ray& incomingRay, const Intersection* intersection)
{
    if ( intersection == 0 ) {
        return 0;
    }
    Vector3Dd hitPoint = intersection->point;
    Vector3Dd normal = intersection->normal;
    double normalLen = normal.length();
    if ( normalLen < VSDK::EPSILON ) {
        return 0;
    }
    Vector3Dd n = normal.multiply(1.0 / normalLen);
    Vector3Dd d = incomingRay.getDirection();
    double dot = d.dotProduct(n);
    Vector3Dd r = d.subtract(n.multiply(2.0 * dot));
    if ( r.length() < VSDK::EPSILON ) {
        return 0;
    }
    Vector3Dd origin = hitPoint.add(n.multiply(1e-4));
    return new Ray(origin, r);
}

Matrix4x4d RayGizmo::rotationFromZToDirection(const Vector3Dd& direction)
{
    double len = direction.length();
    if ( len < VSDK::EPSILON ) {
        return Matrix4x4d();
    }

    Vector3Dd d = direction.multiply(1.0 / len);
    Vector3Dd z(0, 0, 1);
    double dot = z.dotProduct(d);

    if ( dot > 1.0 - VSDK::EPSILON ) {
        return Matrix4x4d();
    }
    if ( dot < -1.0 + VSDK::EPSILON ) {
        return Matrix4x4d().axisRotation(M_PI, 1, 0, 0);
    }

    Vector3Dd axis = z.crossProduct(d).normalized();
    double angle = std::acos(dot);
    return Matrix4x4d().axisRotation(angle, axis.x(), axis.y(), axis.z());
}
