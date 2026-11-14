#include <cfloat>
#include <cmath>

#include "java/lang/String.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
#include "vsdk/toolkit/environment/scene/AffineToleranceMap.h"
#include "vsdk/toolkit/environment/scene/InstanceRayCrossingBuffer.h"
#include "vsdk/toolkit/environment/scene/SceneSolid.h"

SceneSolid::SceneSolid()
    : geometry(0), ownsGeometry(true), geometryIsSphere(false), name(""), modificationVersion(0)
{
}

SceneSolid::~SceneSolid()
{
    if ( geometry != 0 && ownsGeometry ) {
        delete geometry;
    }
    geometry = 0;
}

bool SceneSolid::denotesPlacedGeometry() const { return false; }

bool SceneSolid::isStructuralTraversalSafe() const { return denotesPlacedGeometry(); }

bool SceneSolid::queryDenotedSolid(const Geometry*& intrinsic, bool& inverted) const
{
    intrinsic = nullptr; inverted = false;
    if (!denotesPlacedGeometry() || geometry == nullptr) return false;
    intrinsic = geometry;
    return true;
}

GeometryContainment SceneSolid::orientSide(GeometryContainment side, bool inverted)
{
    if (inverted && side==GeometryContainment::Inside) return GeometryContainment::Outside;
    if (inverted && side==GeometryContainment::Outside) return GeometryContainment::Inside;
    return side;
}

GeometryDistanceStatus SceneSolid::queryDenotedSphere(const Sphere*& sphere, bool& inverted,
    Matrix4x4d& forward, Matrix4x4d& inverse) const
{
    const Geometry* intrinsic=nullptr;
    if (!queryDenotedSolid(intrinsic,inverted)) return GeometryDistanceStatus::Unsupported;
    sphere=dynamic_cast<const Sphere*>(intrinsic);
    if (sphere==nullptr) return GeometryDistanceStatus::Unsupported;
    const AffinePlacementStatus status=queryAffinePlacement(forward,inverse);
    if (status==AffinePlacementStatus::Invalid) return GeometryDistanceStatus::InvalidInput;
    if (status==AffinePlacementStatus::Unsupported) return GeometryDistanceStatus::Unsupported;
    if (status==AffinePlacementStatus::Indeterminate) return GeometryDistanceStatus::Indeterminate;
    return GeometryDistanceStatus::Valid;
}
bool SceneSolid::hasSphereGeometry() const { return geometryIsSphere; }
Material* SceneSolid::getSurfaceMaterial() const { return nullptr; }

void SceneSolid::getWorldBounds(GeometryBounds& result) const
{
    Geometry* shape = getGeometry();
    if (shape == nullptr) { result.setEmpty(); return; }
    GeometryBounds local;
    shape->getBounds(local);
    result = local.transformed(getAffinePlacement());
}

void SceneSolid::getSolidWorldBounds(GeometryBounds& result) const
{
    if (!denotesPlacedGeometry() || dynamic_cast<const Solid*>(getGeometry())==nullptr) {
        result.setUnsupported();
        return;
    }
    getWorldBounds(result);
}

GeometryContainment SceneSolid::classifyWorldBoxCertified(const GeometryBounds& box) const
{
    const Geometry* intrinsic=nullptr; bool inverted=false;
    if (!queryDenotedSolid(intrinsic,inverted)) return GeometryContainment::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement==AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    return classifyPlacedBoxCertified(*intrinsic,inverted,inverse,box);
}

GeometryContainment SceneSolid::classifyPlacedBoxCertified(const Geometry& geometry, bool inverted,
    const Matrix4x4d& inverse, const GeometryBounds& box)
{
    if (box.getKind()==GeometryBounds::Kind::Unsupported) return GeometryContainment::Unsupported;
    if (box.getKind()!=GeometryBounds::Kind::Finite) return GeometryContainment::Indeterminate;
    // The conservative local bounds of the box's preimage, as centre and
    // outward-rounded half widths.
    const GeometryBounds local=box.transformed(inverse);
    if (local.getKind()!=GeometryBounds::Kind::Finite) return GeometryContainment::Indeterminate;
    const double lo[3]={local.getMinimum().x(),local.getMinimum().y(),local.getMinimum().z()};
    const double hi[3]={local.getMaximum().x(),local.getMaximum().y(),local.getMaximum().z()};
    double centre[3], half[3];
    for (int i=0;i<3;++i) {
        centre[i]=lo[i]*0.5+hi[i]*0.5;
        const FloatingInterval below=FloatingInterval::exact(centre[i]).subtract(FloatingInterval::exact(lo[i]));
        const FloatingInterval above=FloatingInterval::exact(hi[i]).subtract(FloatingInterval::exact(centre[i]));
        if (!java::Double::isFinite(centre[i]) || !below.isValid() || !above.isValid())
            return GeometryContainment::Indeterminate;
        half[i]=java::Math::max(java::Math::max(below.getUpper(),above.getUpper()),0.0);
    }
    const GeometryContainment result=geometry.classifyBoxCertified(Vector3Dd(centre[0],centre[1],centre[2]),
        Vector3Dd(half[0],half[1],half[2]));
    if (inverted && result==GeometryContainment::Inside) return GeometryContainment::Outside;
    if (inverted && result==GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}

RayCrossingSupport SceneSolid::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (geometry == nullptr) return RayCrossingSupport::Unsupported;
    Matrix4x4d forward, inverse;
    const AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
    if (status == AffinePlacementStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status == AffinePlacementStatus::Invalid) return RayCrossingSupport::InvalidInput;
    if (status == AffinePlacementStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
    const Vector3Dd origin = inverse.multiply(ray.getOrigin());
    const Vector3Dd direction = inverse.withoutTranslation().multiply(ray.getDirection());
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()))
        return RayCrossingSupport::Indeterminate;
    const Ray local = Ray::fromAffine(origin, direction, ray.getT());
    InstanceRayCrossingBuffer buffer(RayInstanceStep(this,forward,inverse), this, getSurfaceMaterial());
    const RayCrossingSupport result = geometry->emitRayCrossings(local,query,buffer);
    if (result != RayCrossingSupport::Supported) return result;
    if (!buffer.isValid()) return RayCrossingSupport::Indeterminate;
    buffer.deliver(sink);
    return RayCrossingSupport::Supported;
}

RaySolidQueryStatus SceneSolid::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    result.clear();
    if (geometry == nullptr) return RaySolidQueryStatus::Unsupported;
    Matrix4x4d childToParent, parentToChild;
    const AffinePlacementStatus placement = queryAffinePlacement(childToParent, parentToChild);
    if (placement == AffinePlacementStatus::Unsupported) return RaySolidQueryStatus::Unsupported;
    if (placement == AffinePlacementStatus::Invalid) return RaySolidQueryStatus::InvalidInput;
    if (placement == AffinePlacementStatus::Indeterminate) return RaySolidQueryStatus::Indeterminate;
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()))
        return RaySolidQueryStatus::InvalidInput;
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            if (!java::Double::isFinite(childToParent.get(row,column)) ||
                !java::Double::isFinite(parentToChild.get(row,column))) return RaySolidQueryStatus::Indeterminate;
        }
    }
    const Vector3Dd localOrigin = parentToChild.multiply(origin);
    const Vector3Dd localDirection = parentToChild.withoutTranslation().multiply(direction);
    if (!java::Double::isFinite(localOrigin.x()) || !java::Double::isFinite(localOrigin.y()) ||
        !java::Double::isFinite(localOrigin.z()) || !java::Double::isFinite(localDirection.x()) ||
        !java::Double::isFinite(localDirection.y()) || !java::Double::isFinite(localDirection.z()))
        return RaySolidQueryStatus::Indeterminate;
    const Ray localRay = Ray::fromAffine(localOrigin, localDirection, ray.getT());
    const RaySolidQueryStatus status = geometry->classifyRay(localRay, query, result);
    if (status != RaySolidQueryStatus::Supported) { result.clear(); return status; }
    const RayInstanceStep step(this, childToParent, parentToChild);
    for (long i = 0; i < result.boundaries.size(); i++) {
        if (!result.boundaries[i].appendInstance(step)) {
            result.clear();
            return RaySolidQueryStatus::Indeterminate;
        }
    }
    return status;
}

GeometryDistanceStatus SceneSolid::encloseMinimumBoundaryDistance(const Vector3Dd& point,
    double maxWidth, GeometryContainment& side, double& lower, double& upper) const
{
    const Sphere* sphere=nullptr; bool inverted=false;
    Matrix4x4d forward,inverse;
    const GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus::Valid) return availability;
    GeometryContainment candidateSide=GeometryContainment::Indeterminate;
    double candidateLower=0,candidateUpper=0;
    const GeometryDistanceStatus status=AffineEllipsoidMinimumDistance::query(point,sphere->getRadius(),
        forward,inverse,maxWidth,candidateSide,candidateLower,candidateUpper);
    if (status!=GeometryDistanceStatus::Valid) return status;
    side=orientSide(candidateSide,inverted); lower=candidateLower; upper=candidateUpper;
    return GeometryDistanceStatus::Valid;
}

GeometryDistanceStatus SceneSolid::encloseBoundaryDistance(const Vector3Dd& point,
    const Vector3Dd& localDirection, GeometryContainment& side, double& lower, double& upper) const
{
    const Sphere* sphere=nullptr; bool inverted=false;
    Matrix4x4d forward,inverse;
    const GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus::Valid) return availability;
    GeometryContainment candidateSide=GeometryContainment::Indeterminate;
    double candidateLower=0,candidateUpper=0;
    const GeometryDistanceStatus status=AffineEllipsoidDistanceBounds::query(point,sphere->getRadius(),
        forward,localDirection,candidateSide,candidateLower,candidateUpper);
    if (status!=GeometryDistanceStatus::Valid) return status;
    side=orientSide(candidateSide,inverted); lower=candidateLower; upper=candidateUpper;
    return GeometryDistanceStatus::Valid;
}

const Geometry* SceneSolid::getSolidEquivalenceGeometry() const
{
    const Geometry* intrinsic=nullptr; bool inverted=false;
    return queryDenotedSolid(intrinsic,inverted) && !inverted ? intrinsic : nullptr;
}

GeometryContainment SceneSolid::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    if (geometry==nullptr) return GeometryContainment::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement==AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    return classifyPlacedPointCertified(*geometry,false,forward,inverse,point,tolerance);
}

GeometryContainment SceneSolid::classifyPlacedPointCertified(const Geometry& geometry, bool inverted,
    const Matrix4x4d& forward, const Matrix4x4d& inverse, const Vector3Dd& point, double tolerance)
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    const Sphere* sphere=dynamic_cast<const Sphere*>(&geometry);
    const GeometryContainment result=sphere ?
        classifyPlacedSphereCertified(sphere->getRadius(),forward,inverse,point,tolerance,128) :
        classifyPlacedSolidCertified(geometry,forward,point,tolerance);
    if (inverted && result==GeometryContainment::Inside) return GeometryContainment::Outside;
    if (inverted && result==GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}

GeometryContainment SceneSolid::classifyPlacedSolidCertified(const Geometry& geometry,
    const Matrix4x4d& forward, const Vector3Dd& point, double tolerance)
{
    bool identity=true;
    for (int row=0;row<4;++row) for (int column=0;column<4;++column)
        if (forward.get(row,column)!=(row==column ? 1.0 : 0.0)) identity=false;
    if (identity) return geometry.classifyPointCertified(point,tolerance);
    Vector3Dd local, halfWidths;
    double outer=0,inner=-1;
    const AffinePlacementStatus mapped=AffineToleranceMap::map(forward,point,tolerance,local,outer,&inner,&halfWidths);
    if (mapped==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (mapped==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (mapped!=AffinePlacementStatus::Valid) return GeometryContainment::Indeterminate;
    // The axis-aligned local box also contains the preimage and is tighter
    // under anisotropic placement; it can only prove Inside or Outside.
    const GeometryContainment region=geometry.classifyBoxCertified(local,halfWidths);
    if (region==GeometryContainment::Inside || region==GeometryContainment::Outside ||
        region==GeometryContainment::InvalidInput) return region;
    // The outer local ball contains the preimage of the world ball, so a
    // definite side for it is a definite world side. A zero outer radius means
    // the preimage is exactly the local point.
    const GeometryContainment result=geometry.classifyPointCertified(local,outer);
    if (result==GeometryContainment::Inside || result==GeometryContainment::Outside ||
        result==GeometryContainment::InvalidInput || result==GeometryContainment::Unsupported) return result;
    if (result==GeometryContainment::Boundary && outer==0) return result;
    // A boundary inside the inner local ball maps inside the world ball.
    if (inner>0) {
        const GeometryContainment near=geometry.classifyPointCertified(local,inner);
        if (near==GeometryContainment::Boundary || near==GeometryContainment::InvalidInput ||
            near==GeometryContainment::Unsupported) return near;
    }
    if (!(tolerance>0)) return GeometryContainment::Indeterminate;
    // Under anisotropic placement the inner ball is small. Two world points of
    // the (convex) tolerance ball with certified opposite membership prove a
    // boundary point on the segment joining them.
    const GeometryContainment centre=classifyPlacedSolidCertified(geometry,forward,point,0);
    int centreSign=centre==GeometryContainment::Inside ? -1 : centre==GeometryContainment::Outside ? 1 : 0;
    bool foundInside=centreSign<0, foundOutside=centreSign>0;
    const double step=tolerance*(1.0-1.0/1048576.0);
    const double diagonal=step/java::Math::sqrt(3.0)*(1.0-1.0/1048576.0);
    for (int direction=0;direction<14;++direction) {
        Vector3Dd offset;
        if (direction<6) {
            const double sign=direction%2==0 ? 1.0 : -1.0;
            offset=Vector3Dd(direction/2==0 ? sign*step : 0, direction/2==1 ? sign*step : 0,
                direction/2==2 ? sign*step : 0);
        } else {
            const int corner=direction-6;
            offset=Vector3Dd(corner&1 ? -diagonal : diagonal, corner&2 ? -diagonal : diagonal,
                corner&4 ? -diagonal : diagonal);
        }
        const Vector3Dd sample=point.add(offset);
        FloatingInterval squared=FloatingInterval::exact(0);
        squared=squared.add(FloatingInterval::exact(sample.x()).subtract(FloatingInterval::exact(point.x())).square());
        squared=squared.add(FloatingInterval::exact(sample.y()).subtract(FloatingInterval::exact(point.y())).square());
        squared=squared.add(FloatingInterval::exact(sample.z()).subtract(FloatingInterval::exact(point.z())).square());
        const FloatingInterval distance=squared.squareRoot();
        if (!distance.isValid() || distance.getUpper()>tolerance) continue;
        const GeometryContainment side=classifyPlacedSolidCertified(geometry,forward,sample,0);
        if (side==GeometryContainment::Inside) foundInside=true;
        else if (side==GeometryContainment::Outside) foundOutside=true;
        else if (side==GeometryContainment::Boundary) return GeometryContainment::Boundary;
        if (foundInside && foundOutside) return GeometryContainment::Boundary;
    }
    return GeometryContainment::Indeterminate;
}

GeometryContainment SceneSolid::classifyPlacedSphereCertified(double radius,
    const Matrix4x4d& forward, const Matrix4x4d& inverse, const Vector3Dd& point,
    double tolerance, int iterationLimit)
{
    Vector3Dd seed=inverse.multiply(point);
    if (!java::Double::isFinite(seed.x()) || !java::Double::isFinite(seed.y()) ||
        !java::Double::isFinite(seed.z())) return GeometryContainment::Indeterminate;
    const bool center=(point.x()==forward.get(0,3) && point.y()==forward.get(1,3) && point.z()==forward.get(2,3)) ||
        (seed.x()==0 && seed.y()==0 && seed.z()==0);
    const Vector3Dd axes[]={Vector3Dd(1,0,0),Vector3Dd(0,1,0),Vector3Dd(0,0,1)};
    double forwardNormSquared=0;
    for (int i=0;i<3;++i) for (int j=0;j<3;++j)
        forwardNormSquared+=forward.get(i,j)*forward.get(i,j);
    const double lipschitz=radius*radius*forwardNormSquared;
    const int seedCount=center ? 3 : 1;
    for (int candidate=0;candidate<seedCount;++candidate) {
      if (center) seed=axes[candidate];
      for (int iteration=0;iteration<iterationLimit;++iteration) {
        if (iteration==0 || iteration%4==0 || iteration+1==iterationLimit) {
            GeometryContainment side=GeometryContainment::Indeterminate;
            double lower=0,upper=0;
            const GeometryDistanceStatus status=AffineEllipsoidDistanceBounds::query(point,
                radius,forward,seed,side,lower,upper,iteration==0 && candidate==0);
            if (status==GeometryDistanceStatus::InvalidInput) return GeometryContainment::InvalidInput;
            if (status==GeometryDistanceStatus::Unsupported) return GeometryContainment::Unsupported;
            if (status!=GeometryDistanceStatus::Valid) return GeometryContainment::Indeterminate;
            if (upper<=tolerance) return GeometryContainment::Boundary;
            if (lower>tolerance) return side;
        }
        if (!(lipschitz>0) || !java::Double::isFinite(lipschitz)) break;
        const double seedLength=seed.length();
        if (!(seedLength>0) || !java::Double::isFinite(seedLength)) break;
        const Vector3Dd unit=seed.multiply(1.0/seedLength);
        const Vector3Dd delta=forward.multiply(unit.multiply(radius)).subtract(point);
        const double components[3]={delta.x(),delta.y(),delta.z()};
        double gradient[3]={0,0,0};
        for (int i=0;i<3;++i) for (int j=0;j<3;++j)
            gradient[i]+=radius*forward.get(j,i)*components[j];
        const Vector3Dd g(gradient[0],gradient[1],gradient[2]);
        const Vector3Dd tangent=g.subtract(unit.multiply(g.dotProduct(unit)));
        if (tangent.dotProduct(tangent)==0) break;
        seed=unit.subtract(tangent.multiply(1.0/lipschitz));
        if (!java::Double::isFinite(seed.x()) || !java::Double::isFinite(seed.y()) ||
            !java::Double::isFinite(seed.z())) break;
      }
    }
    // If surface-directed search cannot decide, use a separately certified
    // minimum-distance enclosure. The numerical estimate only proposes its
    // witness; interval bounds, never the estimate, determine this result.
    const double requestedWidth=tolerance*1e-3;
    if (requestedWidth>0 && java::Double::isFinite(requestedWidth)) {
        GeometryContainment side=GeometryContainment::Indeterminate;
        double lower=0,upper=0;
        const GeometryDistanceStatus status=AffineEllipsoidMinimumDistance::query(
            point,radius,forward,inverse,requestedWidth,side,lower,upper);
        if (status==GeometryDistanceStatus::InvalidInput) return GeometryContainment::InvalidInput;
        if (status==GeometryDistanceStatus::Unsupported) return GeometryContainment::Unsupported;
        if (status==GeometryDistanceStatus::Valid) {
            if (upper<=tolerance) return GeometryContainment::Boundary;
            if (lower>tolerance) return side;
        }
    }
    return GeometryContainment::Indeterminate;
}

GeometryDistanceStatus SceneSolid::estimateBoundaryDistance(const Vector3Dd& point,
    double& distance, double& estimatedError) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryDistanceStatus::InvalidInput;
    const Sphere* sphere=nullptr; bool inverted=false;
    Matrix4x4d forward,inverse;
    const GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus::Valid) return availability;
    return AffineEllipsoidDistance::query(point,sphere->getRadius(),forward,distance,estimatedError);
}

GeometryContainment SceneSolid::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (geometry == nullptr) return GeometryContainment::Unsupported;
    if (!java::Double::isFinite(tolerance) || tolerance < 0 ||
        !java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryContainment::InvalidInput;
    if (tolerance == 0) {
        Matrix4x4d forward, inverse;
        const AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
        if (status == AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
        if (status == AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
        if (status == AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
        const Vector3Dd local = inverse.multiply(point);
        if (!java::Double::isFinite(local.x()) || !java::Double::isFinite(local.y()) ||
            !java::Double::isFinite(local.z())) return GeometryContainment::Indeterminate;
        return geometry->classifyPoint(local, 0);
    }
    return classifyPointCertified(point, tolerance);
}

void SceneSolid::markModified() { modificationVersion++; }

const java::String& SceneSolid::getName() const { return name; }

long long SceneSolid::getModificationVersion() const { return modificationVersion; }

void SceneSolid::setName(const java::String& n) { name = n; markModified(); }

Geometry* SceneSolid::getGeometry() const { return geometry; }

void SceneSolid::setGeometry(Geometry* g)
{
    if ( geometry != g && geometry != 0 && ownsGeometry ) {
        delete geometry;
    }
    geometry = g;
    ownsGeometry = true;
    geometryIsSphere = dynamic_cast<Sphere*>(geometry) != 0;
    markModified();
}

void SceneSolid::setGeometryReference(Geometry* g)
{
    if ( geometry != g && geometry != 0 && ownsGeometry ) {
        delete geometry;
    }
    geometry = g;
    ownsGeometry = false;
    geometryIsSphere = dynamic_cast<Sphere*>(geometry) != 0;
    markModified();
}
