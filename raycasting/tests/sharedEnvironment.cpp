#include <cmath>
#include <cstdio>
#include <stdexcept>
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingComposite.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/background/SimpleBackground.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/render/raytracing/SimpleRaytracer.h"
#include "vsdk/toolkit/render/raytracing/ParallelRaytracer.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/RayCastingQueryAdapter.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingBox.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/raycasting/geometry/surface/RayCastingInfinitePlane.h"
#include "vsdk/toolkit/environment/raycasting/scene/ProjectedTriangleTraversal.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "java/lang/UnsupportedOperationException.h"
#include "java/lang/IllegalArgumentException.h"
#include "TestSlabGeometry.h"
#include "TestCrossingCollector.h"

int main() {
    NativeGeometryBinding slab(new TestSlabGeometry());
    const Ray slabRay = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    Quadric sharedQuadric(Vector3Dd(1,1,1), Vector3Dd(), Vector3Dd(), -1);
    GeometryBounds sharedQuadricBounds;
    sharedQuadric.getBounds(sharedQuadricBounds);
    BoundingVolumeHierarchy* sharedQuadricCulling = sharedQuadric.createBoundingVolume();
    const AxisAlignedBoundingBox sharedQuadricBox = sharedQuadricCulling->axisAlignedExtent();
    delete sharedQuadricCulling;
    if (sharedQuadricBounds.getKind() != GeometryBounds::Kind::Finite ||
        sharedQuadricBounds.getMinimum().x() != -1 || sharedQuadricBox.getMin().x() != -1 ||
        sharedQuadricBounds.getMaximum().x() != 1 || sharedQuadricBox.getMax().x() != 1) return 115;
    RaySolidClassification quadricSegments;
    if (dynamic_cast<const QuadricSurface*>(sharedQuadric.getIntrinsicGeometry()) == nullptr ||
        sharedQuadric.classifyRay(slabRay, RayCrossingQuery(0,10), quadricSegments) != RaySolidQueryStatus::Supported ||
        quadricSegments.boundaries.size() != 2 ||
        quadricSegments.boundaries[0].geometry != sharedQuadric.getIntrinsicGeometry() ||
        quadricSegments.boundaries[0].t != 2 || quadricSegments.boundaries[1].t != 4) return 113;
    sharedQuadric.setConstantCached(true);
    sharedQuadric.invertGeometry();
    sharedQuadric.getBounds(sharedQuadricBounds);
    if (sharedQuadricBounds.getKind() != GeometryBounds::Kind::Unbounded) return 116;
    if (sharedQuadric.isConstantCached() ||
        sharedQuadric.classifyPoint(Vector3Dd(),0) != GeometryContainment::Outside ||
        sharedQuadric.classifyRay(slabRay, RayCrossingQuery(0,10), quadricSegments) != RaySolidQueryStatus::Supported ||
        !quadricSegments.insideSegments[0] || quadricSegments.insideSegments[1] ||
        !quadricSegments.insideSegments[2]) return 114;
    Geometry* slabGeometry = &slab;
    TestCrossingCollector slabCrossings;
    if (slabGeometry->emitRayCrossings(slabRay, RayCrossingQuery(0,10), slabCrossings) !=
            RayCrossingSupport::Supported || slabCrossings.hits.size() != 2 ||
        slabCrossings.hits[0]->t != 2 || slabCrossings.hits[1]->t != 4 ||
        slabCrossings.hits[0]->geometry != slab.getIntrinsicGeometry()) return 29;
    RayCastingQueryAdapter slabContext(slabRay);
    IntersectionCandidate slabHit;
    if (!slab.doIntersectionFirstHit(&slabContext.ray, slabHit) ||
        slabHit.getIntersection().t != 2 ||
        slab.doContainmentTest(Vector3Dd(0,0,0), 0) != Geometry::INSIDE) return 30;
    RayHit slabDetail(RayHit::DETAIL_NORMAL, false);
    if (!slabGeometry->doIntersectionFirstHit(slabRay, &slabDetail) ||
        slabDetail.normal.x() != -1) return 31;
    NativeGeometryBinding slabCopy(slab);
    if (slabCopy.getIntrinsicGeometry() == slab.getIntrinsicGeometry()) return 32;
    slab.invertGeometry();
    if (slab.doContainmentTest(Vector3Dd(0,0,0), 0) != Geometry::OUTSIDE) return 33;
    const Matrix4x4d outerColumn = Matrix4x4d().translation(3,0,0).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.5));
    const Matrix4x4d outerInverse = Matrix4x4d::identityMatrix().withVal(0,1,-0.5).multiply(
        Matrix4x4d().translation(-3,0,0));
    java::ArrayList<RayCastingBody*> affineRegions;
    NativeGeometryBinding* affineSphere = new NativeGeometryBinding(new Sphere(1));
    RayCastingBody affineBody(affineSphere, nullptr, nullptr, nullptr, false, affineRegions, affineRegions,
        new Matrix4x4d(outerColumn.transpose()), new Matrix4x4d(outerInverse.transpose()),
        new Matrix4x4d(Matrix4x4d().scale(2,1,1).transpose()),
        new Matrix4x4d(Matrix4x4d().scale(0.5,1,1).transpose()));
    Matrix4x4d placement, inversePlacement;
    if (affineBody.queryAffinePlacement(placement,inversePlacement) != AffinePlacementStatus::Valid) return 122;
    const Vector3Dd localOrigin(-3,0.5,0), localDirection(2,0,0);
    const Ray affineRay=Ray::fromAffine(placement.multiply(localOrigin),
        placement.withoutTranslation().multiply(localDirection));
    RaySolidClassification affineSegments;
    const double expectedOffset=java::Math::sqrt(0.75);
    if (affineBody.classifyRay(affineRay,RayCrossingQuery(0,10),affineSegments) != RaySolidQueryStatus::Supported ||
        affineSegments.boundaries.size()!=2 ||
        java::Math::abs(affineSegments.boundaries[0].t-(3-expectedOffset)/2)>1e-10 ||
        java::Math::abs(affineSegments.boundaries[1].t-(3+expectedOffset)/2)>1e-10 ||
        affineSegments.boundaries[0].geometry != affineSphere->getIntrinsicGeometry() ||
        affineSegments.boundaries[0].getInstanceCount()!=1 ||
        affineBody.classifyPoint(placement.multiply(Vector3Dd()),0) != GeometryContainment::Inside ||
        affineBody.classifyPoint(placement.multiply(Vector3Dd()),0.1) != GeometryContainment::Indeterminate) return 123;
    TestCrossingCollector affineEvents;
    if (affineBody.emitRayCrossings(affineRay,RayCrossingQuery(0,10),affineEvents) != RayCrossingSupport::Supported ||
        affineEvents.hits.size()!=2 || affineEvents.hits[0]->geometry != affineSphere->getIntrinsicGeometry() ||
        affineEvents.hits[0]->getInstanceCount()!=1 || affineEvents.hits[0]->body != &affineBody ||
        affineEvents.hits[0]->kind != RayCrossing::Kind::Entering ||
        java::Math::abs(affineEvents.hits[0]->t-(3-expectedOffset)/2)>1e-10) return 127;
    affineSphere->invertGeometry();
    if (affineBody.classifyRay(affineRay,RayCrossingQuery(0,10),affineSegments) != RaySolidQueryStatus::Supported ||
        !affineSegments.insideSegments[0] || affineSegments.insideSegments[1] ||
        !affineSegments.insideSegments[2] || !affineSegments.boundaries[0].normalReversed ||
        affineBody.classifyPoint(placement.multiply(Vector3Dd()),0) != GeometryContainment::Outside) return 124;
    TestCrossingCollector invertedAffineEvents;
    if (affineBody.emitRayCrossings(affineRay,RayCrossingQuery(0,10),invertedAffineEvents) != RayCrossingSupport::Supported ||
        invertedAffineEvents.hits.size()!=2 || !invertedAffineEvents.hits[0]->normalReversed ||
        invertedAffineEvents.hits[0]->kind != RayCrossing::Kind::Leaving) return 128;
    const Matrix4x4d editedForward = Matrix4x4d().translation(-2,1,0).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.25)).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d editedInverse = Matrix4x4d().scale(0.5,1,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,-0.25)).multiply(Matrix4x4d().translation(2,-1,0));
    if (affineBody.setAffinePlacement(editedForward,editedInverse) != AffinePlacementStatus::Valid ||
        !affineBody.hasArbitraryAffineTransform() ||
        affineBody.queryAffinePlacement(placement,inversePlacement) != AffinePlacementStatus::Valid ||
        !placement.epsilonEquals(editedForward,1e-12) ||
        !affineBody.getTransformationMatrix().epsilonEquals(editedForward,1e-12) ||
        affineBody.getPosition().x() != -2) return 131;
    if (affineBody.setAffinePlacement(Matrix4x4d().scale(0,1,1),Matrix4x4d()) != AffinePlacementStatus::Invalid ||
        !affineBody.getAffinePlacement().epsilonEquals(editedForward,1e-12)) return 132;
    affineBody.resetAffinePlacement();
    if (!affineBody.getAffinePlacement().epsilonEquals(Matrix4x4d(),1e-12)) return 133;
    RayCastingBody singularBody(new NativeGeometryBinding(new Sphere(1)), nullptr, nullptr, nullptr,
        false, affineRegions, affineRegions, new Matrix4x4d(Matrix4x4d().scale(0,1,1)),
        new Matrix4x4d(Matrix4x4d()));
    if (singularBody.classifyRay(affineRay,RayCrossingQuery(0,10),affineSegments) != RaySolidQueryStatus::InvalidInput ||
        singularBody.classifyPoint(Vector3Dd(),0) != GeometryContainment::InvalidInput) return 125;
    RayCastingBody inconsistentBody(new NativeGeometryBinding(new Sphere(1)), nullptr, nullptr, nullptr,
        false, affineRegions, affineRegions, new Matrix4x4d(Matrix4x4d().scale(2,1,1)),
        new Matrix4x4d(Matrix4x4d()));
    if (inconsistentBody.classifyRay(affineRay,RayCrossingQuery(0,10),affineSegments) != RaySolidQueryStatus::Indeterminate ||
        inconsistentBody.classifyPoint(Vector3Dd(),0) != GeometryContainment::Indeterminate) return 126;
    NativeGeometryBinding nativeBox(new Box(2,2,2));
    GeometryBounds boxLocalBounds;
    nativeBox.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != -1 || boxLocalBounds.getMaximum().x() != 1) return 95;
    NativeGeometryBinding inverseBounds(new Sphere(1));
    inverseBounds.invertGeometry();
    inverseBounds.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unbounded) return 96;
    const Matrix4x4d shear = Matrix4x4d::identityMatrix().withVal(1,0,2).withVal(3,0,10);
    const AxisAlignedBoundingBox placed = AxisAlignedBoundingBox::fromTransformedCorners(
        Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1), &shear);
    if (placed.getMin().x() != 7 || placed.getMax().x() != 13 ||
        placed.getMin().y() != -1 || placed.getMax().y() != 1) return 97;
    const Matrix4x4d invalidPlacement = shear.withVal(0,3,1);
    if (!AxisAlignedBoundingBox::fromTransformedCorners(
        Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1), &invalidPlacement).isUnbounded()) return 98;
    java::ArrayList<RayCastingBody*> noRegions;
    RayCastingBody placedBody(new NativeGeometryBinding(new Sphere(2)), nullptr, nullptr,
        nullptr, false, noRegions, noRegions);
    placedBody.setPosition(Vector3Dd(5,0,0));
    placedBody.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != 3 || boxLocalBounds.getMaximum().x() != 7) return 99;
    Vector3Dd geometryScale(2,1,1);
    placedBody.scaleGeometryLayer(&geometryScale);
    placedBody.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != 1 || boxLocalBounds.getMaximum().x() != 9 ||
        placedBody.getAffinePlacement().multiply(Vector3Dd(1,0,0)).x() != 7) return 100;
    const AxisAlignedBoundingBox placedCulling = placedBody.getAABB();
    if (placedCulling.getMin().x() != boxLocalBounds.getMinimum().x() ||
        placedCulling.getMax().x() != boxLocalBounds.getMaximum().x()) return 101;
    java::ArrayList<RayCastingBody*> compositeChildren;
    compositeChildren.add(new RayCastingBody(new NativeGeometryBinding(new Sphere(1)),
        nullptr, nullptr, nullptr, false, noRegions, noRegions));
    RayCastingBody* shiftedChild = new RayCastingBody(new NativeGeometryBinding(new Sphere(2)),
        nullptr, nullptr, nullptr, false, noRegions, noRegions);
    shiftedChild->setPosition(Vector3Dd(4,0,0));
    compositeChildren.add(shiftedChild);
    RayCastingComposite composite(nullptr, nullptr, nullptr, nullptr, false,
        noRegions, noRegions, compositeChildren);
    composite.setPosition(Vector3Dd(10,0,0));
    composite.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != 9 || boxLocalBounds.getMaximum().x() != 16) return 102;
    composite.getSimpleBodies().add(new RayCastingBody(new NativeGeometryBinding(new TestSlabGeometry()),
        nullptr, nullptr, nullptr, false, noRegions, noRegions));
    composite.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unbounded) return 103;
    composite.getSimpleBodies().add(new RayCastingBody(new NativeGeometryBinding(new Torus(2,1)),
        nullptr, nullptr, nullptr, false, noRegions, noRegions));
    composite.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unsupported) return 104;
    java::ArrayList<RayCastingBody*> nestedChildren;
    nestedChildren.add(new RayCastingBody(new NativeGeometryBinding(new Sphere(1)),
        nullptr, nullptr, nullptr, false, noRegions, noRegions));
    RayCastingComposite* nested = new RayCastingComposite(nullptr, nullptr, nullptr, nullptr,
        false, noRegions, noRegions, nestedChildren);
    nested->setPosition(Vector3Dd(2,0,0));
    java::ArrayList<RayCastingBody*> outerChildren;
    outerChildren.add(nested);
    RayCastingComposite outerComposite(nullptr, nullptr, nullptr, nullptr, false,
        noRegions, noRegions, outerChildren);
    outerComposite.setPosition(Vector3Dd(5,0,0));
    outerComposite.getWorldBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != 6 || boxLocalBounds.getMaximum().x() != 8) return 105;
    TestCrossingCollector boxCrossings;
    if (static_cast<Geometry*>(&nativeBox)->emitRayCrossings(
            slabRay, RayCrossingQuery(0,10), boxCrossings) != RayCrossingSupport::Supported ||
        boxCrossings.hits.size() != 2 || boxCrossings.hits[0]->t != 2 ||
        boxCrossings.hits[1]->t != 4 ||
        boxCrossings.hits[0]->geometry != nativeBox.getIntrinsicGeometry()) return 36;
    RayHit boxDetail(RayHit::DETAIL_NORMAL, false);
    if (!static_cast<Geometry*>(&nativeBox)->doIntersectionFirstHit(slabRay, &boxDetail) ||
        boxDetail.normal.x() != -1) return 37;
    NativeGeometryBinding boxCopy(nativeBox);
    if (boxCopy.getIntrinsicGeometry() == nativeBox.getIntrinsicGeometry() ||
        boxCopy.doContainmentTest(Vector3Dd(0,0,0), 0) != Geometry::INSIDE) return 38;
    RayCastingBox arbitraryBox(Vector3Dd(-2,-1,-3), Vector3Dd(4,3,1));
    const Ray boxRay=Ray::fromAffine(Vector3Dd(-4,0,0),Vector3Dd(1,0,0));
    RayBoundary boxBoundary;
    RaySolidClassification arbitrarySegments;
    if (dynamic_cast<const Box*>(arbitraryBox.getIntrinsicGeometry()) == nullptr ||
        arbitraryBox.queryFirstHit(boxRay,RayCrossingQuery(0,10),nullptr,&boxBoundary) != RayFirstHitStatus::Hit ||
        boxBoundary.geometry != arbitraryBox.getIntrinsicGeometry() || boxBoundary.t != 2 ||
        arbitraryBox.classifyRay(boxRay,RayCrossingQuery(0,10),arbitrarySegments) != RaySolidQueryStatus::Supported ||
        arbitrarySegments.boundaries.size() != 2 || arbitrarySegments.boundaries[1].t != 8) return 117;
    arbitraryBox.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != -2 || boxLocalBounds.getMaximum().x() != 4) return 106;
    const Vector3Dd boxMin(-2,-1,-3), boxMax(4,3,1);
    if (!Box::containsBounds(Vector3Dd(-2,0,0), boxMin, boxMax) ||
        Box::containsBounds(Vector3Dd(-2.1,0,0), boxMin, boxMax) ||
        Box::normalForBounds(Vector3Dd(4,3,1), boxMin, boxMax, 1e-8).x() != 1 ||
        Box::normalForBounds(Vector3Dd(0,-1,0), boxMin, boxMax, 1e-8).y() != -1 ||
        arbitraryBox.doContainmentTest(Vector3Dd(-2,0,0), 0) != Geometry::INSIDE) return 93;
    arbitraryBox.invertGeometry();
    if (arbitraryBox.classifyRay(boxRay,RayCrossingQuery(0,10),arbitrarySegments) != RaySolidQueryStatus::Supported ||
        arbitrarySegments.insideSegments.size() != 3 || !arbitrarySegments.insideSegments[0] ||
        arbitrarySegments.insideSegments[1] || !arbitrarySegments.insideSegments[2] ||
        !arbitrarySegments.boundaries[0].normalReversed) return 118;
    TestCrossingCollector invertedBoxEvents;
    if (arbitraryBox.emitRayCrossings(boxRay,RayCrossingQuery(0,10),invertedBoxEvents) != RayCrossingSupport::Supported ||
        invertedBoxEvents.hits.size() != 2 || invertedBoxEvents.hits[0]->kind != RayCrossing::Kind::Leaving ||
        !invertedBoxEvents.hits[0]->normalReversed) return 119;
    arbitraryBox.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unbounded) return 107;
    if (arbitraryBox.doContainmentTest(Vector3Dd(-2,0,0), 0) != Geometry::OUTSIDE ||
        arbitraryBox.doContainmentTest(Vector3Dd(-2.1,0,0), 0) != Geometry::INSIDE) return 94;
    arbitraryBox.invertGeometry();
    const Vector3Dd origins[] = {Vector3Dd(-4,0,0), Vector3Dd(5,0,0),
        Vector3Dd(0,0,0), Vector3Dd(5,0,0)};
    const Vector3Dd directions[] = {Vector3Dd(1,0,0), Vector3Dd(-1,0,0),
        Vector3Dd(1,0,0), Vector3Dd(0,1,0)};
    const bool expectedHit[] = {true, true, true, false};
    const double expectedNear[] = {2,1,4,0};
    const double expectedFar[] = {8,7,4,0};
    for (int i = 0; i < 4; ++i) {
        RayCastingQueryAdapter context(Ray::fromAffine(origins[i], directions[i]));
        double nearT = 0, farT = 0;
        const bool hit = RayCastingBox::intersectBoxx(&context.ray, &arbitraryBox, &nearT, &farT);
        if (hit != expectedHit[i] || (hit &&
            (nearT != expectedNear[i] || farT != expectedFar[i]))) return 39;
    }
    NativeGeometryBinding nativePlane(new InfinitePlane(1,0,0,0));
    TestCrossingCollector planeCrossings;
    if (static_cast<Geometry*>(&nativePlane)->emitRayCrossings(
            slabRay, RayCrossingQuery(0,10), planeCrossings) != RayCrossingSupport::Supported ||
        planeCrossings.hits.size() != 1 || planeCrossings.hits[0]->t != 3 ||
        planeCrossings.hits[0]->geometry != nativePlane.getIntrinsicGeometry()) return 40;
    RayHit planeDetail(RayHit::DETAIL_NORMAL, false);
    if (!static_cast<Geometry*>(&nativePlane)->doIntersectionFirstHit(slabRay, &planeDetail) ||
        planeDetail.normal.x() != 1) return 41;
    RayCastingInfinitePlane legacyPlane(Vector3Dd(1,0,0), 0);
    legacyPlane.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unbounded) return 108;
    RayBoundary sharedPlaneBoundary;
    RayHit sharedPlaneHit(RayHit::DETAIL_NONE, false);
    TestCrossingCollector sharedPlaneEvents;
    RaySolidClassification sharedPlaneSegments;
    if (legacyPlane.queryFirstHit(slabRay, RayCrossingQuery(0,10), &sharedPlaneHit,
            &sharedPlaneBoundary) != RayFirstHitStatus::Hit ||
        sharedPlaneBoundary.geometry != legacyPlane.getIntrinsicGeometry() ||
        sharedPlaneBoundary.t != 3 ||
        legacyPlane.emitRayCrossings(slabRay, RayCrossingQuery(0,10), sharedPlaneEvents) !=
            RayCrossingSupport::Supported || sharedPlaneEvents.hits.size() != 1 ||
        sharedPlaneEvents.hits[0]->geometry != legacyPlane.getIntrinsicGeometry() ||
        legacyPlane.classifyPoint(Vector3Dd(-1,0,0), 0) != GeometryContainment::Inside ||
        legacyPlane.classifyRay(slabRay, RayCrossingQuery(0,10), sharedPlaneSegments) !=
            RaySolidQueryStatus::Supported || sharedPlaneSegments.boundaries.size() != 1 ||
        sharedPlaneSegments.boundaries[0].geometry != legacyPlane.getIntrinsicGeometry()) return 109;
    RayCastingQueryAdapter planeContext(slabRay);
    double planeDepth = 0;
    if (!RayCastingInfinitePlane::intersectPlane(&planeContext.ray, &legacyPlane, &planeDepth) ||
        planeDepth != 3) return 42;
    planeContext.ray.setPrimaryRay(true);
    if (!RayCastingInfinitePlane::intersectPlane(&planeContext.ray, &legacyPlane, &planeDepth) ||
        planeDepth != 3 || !legacyPlane.isVpCached()) return 43;
    RayCastingInfinitePlane scaledPlane(Vector3Dd(2,0,0), -4);
    if (scaledPlane.getHalfSpace().getBoundaryPlane().getCoefficients().x() != 2 ||
        scaledPlane.getHalfSpace().classifyPoint(Vector3Dd(1,0,0), 0) != GeometryContainment::Inside ||
        !RayCastingInfinitePlane::intersectPlane(&planeContext.ray, &scaledPlane, &planeDepth) ||
        planeDepth != 5) return 90;
    scaledPlane.invertGeometry();
    if (scaledPlane.isVpCached() || scaledPlane.getNormalVector().x() != -2 ||
        scaledPlane.getDistance() != 4 ||
        scaledPlane.getHalfSpace().classifyPoint(Vector3Dd(1,0,0), 0) != GeometryContainment::Outside ||
        !RayCastingInfinitePlane::intersectPlane(&planeContext.ray, &scaledPlane, &planeDepth) ||
        planeDepth != 5) return 91;
    RayCastingInfinitePlane independentPlane(scaledPlane);
    independentPlane.setDistance(6);
    if (independentPlane.isVpCached() || scaledPlane.getDistance() != 4 ||
        independentPlane.getDistance() != 6) return 92;
    NativeGeometryBinding halfSpace(new PlanarHalfSpace(1,0,0,0));
    TestCrossingCollector halfCrossings;
    if (static_cast<Geometry*>(&halfSpace)->emitRayCrossings(
            slabRay, RayCrossingQuery(0,10), halfCrossings) != RayCrossingSupport::Supported ||
        halfCrossings.hits.size() != 1 || halfCrossings.hits[0]->t != 3 ||
        halfCrossings.hits[0]->geometry != halfSpace.getIntrinsicGeometry() ||
        halfSpace.doContainmentTest(Vector3Dd(-1,0,0), 0) != Geometry::INSIDE ||
        halfSpace.doContainmentTest(Vector3Dd(1,0,0), 0) != Geometry::OUTSIDE) return 44;
    NativeGeometryBinding triangleMesh(new TriangleMesh());
    triangleMesh.enableProjectedTriangleTraversal();
    triangleMesh.getProjectedTriangleTraversal()->addTriangle(
        Vector3Dd(0,0,0), Vector3Dd(2,0,0), Vector3Dd(0,2,0));
    const Ray triangleRay = Ray::fromAffine(Vector3Dd(0.5,0.5,4), Vector3Dd(0,0,-2));
    TestCrossingCollector triangleCrossings;
    if (static_cast<Geometry*>(&triangleMesh)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), triangleCrossings) != RayCrossingSupport::Supported ||
        triangleCrossings.hits.size() != 1 || triangleCrossings.hits[0]->t != 2 ||
        triangleCrossings.hits[0]->geometry != triangleMesh.getIntrinsicGeometry() ||
        triangleCrossings.hits[0]->surfaceIndex != 0) return 45;
    const TriangleMesh* sharedTriangleMesh = dynamic_cast<const TriangleMesh*>(triangleMesh.getIntrinsicGeometry());
    if (sharedTriangleMesh == nullptr || sharedTriangleMesh->getNumTriangles() != 1) return 49;
    GeometryBounds triangleBounds;
    triangleMesh.getBounds(triangleBounds);
    if (triangleBounds.getKind() != GeometryBounds::Kind::Finite) return 129;
    sharedTriangleMesh->getBounds(triangleBounds);
    if (triangleBounds.getKind() != GeometryBounds::Kind::Finite ||
        triangleBounds.getMinimum().x() != 0 || triangleBounds.getMaximum().x() != 2 ||
        triangleBounds.getMaximum().y() != 2) return 50;
    NativeGeometryBinding* placedTriangle = new NativeGeometryBinding(new TriangleMesh());
    placedTriangle->enableProjectedTriangleTraversal();
    placedTriangle->getProjectedTriangleTraversal()->addTriangle(
        Vector3Dd(0,0,0),Vector3Dd(2,0,0),Vector3Dd(0,2,0));
    RayCastingBody triangleBody(placedTriangle,nullptr,nullptr,nullptr,false,noRegions,noRegions);
    triangleBody.setPosition(Vector3Dd(0,0,5));
    TestCrossingCollector placedTriangleEvents;
    if (triangleBody.emitRayCrossings(Ray::fromAffine(Vector3Dd(0.5,0.5,9),Vector3Dd(0,0,-2)),
            RayCrossingQuery(0,10),placedTriangleEvents) != RayCrossingSupport::Supported ||
        placedTriangleEvents.hits.size()!=1 || placedTriangleEvents.hits[0]->t!=2 ||
        placedTriangleEvents.hits[0]->geometry!=placedTriangle->getIntrinsicGeometry() ||
        placedTriangleEvents.hits[0]->surfaceIndex!=0 || placedTriangleEvents.hits[0]->getInstanceCount()!=1 ||
        placedTriangleEvents.hits[0]->body!=&triangleBody) return 134;
    NativeGeometryBinding normalTriangleCopy(triangleMesh);
    if (normalTriangleCopy.getIntrinsicGeometry() == triangleMesh.getIntrinsicGeometry()) return 51;
    TriangleMesh* nativeMesh = new TriangleMesh();
    nativeMesh->initVertexPositionsArray(3);
    java::ArrayList<double>& nativePositions = nativeMesh->getVertexPositions();
    const double nativeVertices[] = {0,0,0, 2,0,0, 0,2,0};
    for (int i = 0; i < 9; ++i) nativePositions[i] = nativeVertices[i];
    nativeMesh->initTriangleArrays(1);
    nativeMesh->setTriangleAt(0, Triangle(0,1,2));
    NativeGeometryBinding genericTriangle(nativeMesh);
    TestCrossingCollector genericTriangleCrossings;
    if (static_cast<Geometry*>(&genericTriangle)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), genericTriangleCrossings) != RayCrossingSupport::Supported ||
        genericTriangleCrossings.hits.size() != 1 || genericTriangleCrossings.hits[0]->surfaceIndex != 0 ||
        genericTriangleCrossings.hits[0]->geometry != nativeMesh) return 53;
    RayHit genericTriangleDetail(RayHit::DETAIL_NORMAL, false);
    if (!static_cast<Geometry*>(&genericTriangle)->doIntersectionFirstHit(triangleRay, &genericTriangleDetail) ||
        genericTriangleDetail.normal.z() != 1) return 54;
    java::ArrayList<double>& nativeNormals = nativeMesh->getTriangleNormals();
    nativeNormals[0] = 0.25; nativeNormals[1] = 0.5; nativeNormals[2] = 0.75;
    const int originalFirst = nativeMesh->getTriangleIndexes()[0];
    const int originalSecond = nativeMesh->getTriangleIndexes()[1];
    genericTriangle.enableProjectedTriangleTraversal();
    if (nativeMesh->getTriangleIndexes()[0] != originalFirst ||
        nativeMesh->getTriangleIndexes()[1] != originalSecond ||
        nativeNormals[0] != 0.25 || nativeNormals[1] != 0.5 || nativeNormals[2] != 0.75) return 57;
    TestCrossingCollector projectedNativeCrossings;
    if (static_cast<Geometry*>(&genericTriangle)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), projectedNativeCrossings) != RayCrossingSupport::Supported ||
        projectedNativeCrossings.hits.size() != 1 || projectedNativeCrossings.hits[0]->t != 2 ||
        projectedNativeCrossings.hits[0]->geometry != nativeMesh ||
        projectedNativeCrossings.hits[0]->surfaceIndex != 0) return 58;
    TriangleMesh* badMesh = new TriangleMesh();
    badMesh->initVertexPositionsArray(3);
    badMesh->initTriangleArrays(1);
    badMesh->setTriangleAt(0, Triangle(0,1,99));
    NativeGeometryBinding invalidTriangle(badMesh);
    bool rejectedInvalidMesh = false;
    try { invalidTriangle.enableProjectedTriangleTraversal(); }
    catch (const java::IllegalArgumentException&) { rejectedInvalidMesh = true; }
    if (!rejectedInvalidMesh) return 59;
    RayCastingQueryAdapter triangleContext(triangleRay);
    triangleContext.ray.setPrimaryRay(true);
    for (int i = 0; i < 2; ++i) {
        java::PriorityQueue<IntersectionCandidate>* queue = triangleContext.pool.pop(8);
        const int hit = triangleMesh.doIntersectionForAllRayCrossings(&triangleContext.ray, queue);
        const bool correct = hit && queue->size() == 1 && queue->peek().getIntersection().t == 2;
        triangleContext.pool.push(queue);
        if (!correct) return 46;
    }
    NativeGeometryBinding cachedTriangleCopy(triangleMesh);
    RayCastingQueryAdapter shiftedTriangleContext(
        Ray::fromAffine(Vector3Dd(0.5,0.5,6), Vector3Dd(0,0,-2)));
    shiftedTriangleContext.ray.setPrimaryRay(true);
    java::PriorityQueue<IntersectionCandidate>* shiftedQueue = shiftedTriangleContext.pool.pop(8);
    const int shiftedHit = cachedTriangleCopy.doIntersectionForAllRayCrossings(
        &shiftedTriangleContext.ray, shiftedQueue);
    const bool shiftedCorrect = shiftedHit && shiftedQueue->size() == 1 &&
        shiftedQueue->peek().getIntersection().t == 3;
    shiftedTriangleContext.pool.push(shiftedQueue);
    if (!shiftedCorrect) return 55;
    NativeGeometryBinding* csgTriangle = new NativeGeometryBinding(normalTriangleCopy);
    const Geometry* csgLeaf = csgTriangle->getIntrinsicGeometry();
    ConstructiveSolidGeometryByMorganRules triangleUnion(BooleanSetOperations::UNION);
    triangleUnion.addShape(csgTriangle, nullptr);
    TestCrossingCollector csgTriangleCrossings;
    if (static_cast<Geometry*>(&triangleUnion)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), csgTriangleCrossings) != RayCrossingSupport::Supported ||
        csgTriangleCrossings.hits.size() != 1 || csgTriangleCrossings.hits[0]->geometry != csgLeaf ||
        csgTriangleCrossings.hits[0]->surfaceIndex != 0) return 56;
    triangleMesh.invertGeometry();
    triangleMesh.getBounds(triangleBounds);
    BoundingVolumeHierarchy* invertedTriangleBounds=triangleMesh.createBoundingVolume();
    const bool triangleIsUnbounded=invertedTriangleBounds->isUnbounded();
    delete invertedTriangleBounds;
    if (triangleBounds.getKind() != GeometryBounds::Kind::Unbounded || !triangleIsUnbounded) return 130;
    TestCrossingCollector invertedInterior;
    if (static_cast<Geometry*>(&triangleMesh)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), invertedInterior) != RayCrossingSupport::Supported ||
        invertedInterior.hits.size() != 0) return 47;
    TestCrossingCollector invertedExterior;
    if (static_cast<Geometry*>(&triangleMesh)->emitRayCrossings(
            Ray::fromAffine(Vector3Dd(3,3,4), Vector3Dd(0,0,-2)),
            RayCrossingQuery(0,10), invertedExterior) != RayCrossingSupport::Supported ||
        invertedExterior.hits.size() != 1 || invertedExterior.hits[0]->t != 2) return 48;
    NativeGeometryBinding combinedTriangles(new TriangleMesh());
    combinedTriangles.enableProjectedTriangleTraversal();
    combinedTriangles.getProjectedTriangleTraversal()->appendFrom(
        *normalTriangleCopy.getProjectedTriangleTraversal(), 0);
    combinedTriangles.getProjectedTriangleTraversal()->appendFrom(
        *triangleMesh.getProjectedTriangleTraversal(), 0);
    TestCrossingCollector mergedInterior, mergedExterior;
    if (static_cast<Geometry*>(&combinedTriangles)->emitRayCrossings(
            triangleRay, RayCrossingQuery(0,10), mergedInterior) != RayCrossingSupport::Supported ||
        mergedInterior.hits.size() != 1 ||
        static_cast<Geometry*>(&combinedTriangles)->emitRayCrossings(
            Ray::fromAffine(Vector3Dd(3,3,4), Vector3Dd(0,0,-2)),
            RayCrossingQuery(0,10), mergedExterior) != RayCrossingSupport::Supported ||
        mergedExterior.hits.size() != 1 || mergedInterior.hits[0]->surfaceIndex != 0 ||
        mergedExterior.hits[0]->surfaceIndex != 1) return 52;
    NativeGeometryBinding unsupported(new Torus(2,1));
    bool rejectedUnsupported = false;
    try { unsupported.doIntersectionFirstHit(&slabContext.ray, slabHit); }
    catch (const java::UnsupportedOperationException&) { rejectedUnsupported = true; }
    if (!rejectedUnsupported) return 34;
    NativeGeometryBinding reversedBoundary(new TestSlabGeometry(true));
    TestCrossingCollector preservedProvenance;
    if (static_cast<Geometry*>(&reversedBoundary)->emitRayCrossings(
            slabRay, RayCrossingQuery(0,10), preservedProvenance) != RayCrossingSupport::Supported ||
        preservedProvenance.hits.size() != 2 || !preservedProvenance.hits[0]->normalReversed) return 35;
    Sphere* intrinsicPointer = new Sphere(2.0);
    NativeGeometryBinding bridge(intrinsicPointer);
    const Sphere& intrinsic = *intrinsicPointer;
    intrinsicPointer->setRadius(3.0);
    if (intrinsic.getRadius() != 3.0 || intrinsic.implicitValue(Vector3Dd(3,0,0)) != 0) return 21;
    intrinsicPointer->setRadius(2.0);
    if (bridge.doContainmentTest(Vector3Dd(2,0,0), 0.0) != Geometry::OUTSIDE ||
        bridge.doContainmentTest(Vector3Dd(2,0,0), 0.01) != Geometry::INSIDE) return 24;
    bridge.invertGeometry();
    if (bridge.doContainmentTest(Vector3Dd(3,0,0), 0.0) != Geometry::INSIDE) return 25;
    bridge.invertGeometry();
    RayHit nativeHit(RayHit::DETAIL_NONE, false);
    const Ray nativeRay = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    if (intrinsic.queryFirstHit(nativeRay, RayCrossingQuery(0,10), &nativeHit) != RayFirstHitStatus::Hit ||
        nativeHit.getHitDistance() != 1.0) return 22;
    TestCrossingCollector nativeCrossings;
    if (intrinsic.emitRayCrossings(nativeRay, RayCrossingQuery(0,10), nativeCrossings) !=
            RayCrossingSupport::Supported || nativeCrossings.hits.size() != 2 ||
        nativeCrossings.hits[0]->t != 1.0 || nativeCrossings.hits[1]->t != 5.0) return 26;
    NativeGeometryBinding bridgeCopy(bridge);
    intrinsicPointer->setRadius(1.0);
    const Sphere* copiedSphere = dynamic_cast<const Sphere*>(bridgeCopy.getIntrinsicGeometry());
    if (copiedSphere == nullptr || copiedSphere->getRadius() != 2.0 || intrinsic.getRadius() != 1.0) return 23;
    TestCrossingCollector bridgeCrossings;
    if (bridgeCopy.emitRayCrossings(nativeRay, RayCrossingQuery(0,10), bridgeCrossings) !=
            RayCrossingSupport::Supported || bridgeCrossings.hits.size() != 2 ||
        bridgeCrossings.hits[0]->geometry != copiedSphere ||
        bridgeCrossings.hits[1]->geometry != copiedSphere) return 27;
    ConstructiveSolidGeometryByRaySegment shell(BooleanSetOperations::DIFFERENCE);
    NativeGeometryBinding* outer = new NativeGeometryBinding(new Sphere(2.0));
    NativeGeometryBinding* inner = new NativeGeometryBinding(new Sphere(1.0));
    shell.addShape(outer, nullptr);
    shell.addShape(inner, nullptr);
    const CsgExpression* commonExpression = &shell;
    if (commonExpression->getOperation() != CsgOperation::DIFFERENCE ||
        commonExpression->getOperandCount() != 2 ||
        commonExpression->getOperand(0)->getGeometry() != outer ||
        commonExpression->getOperand(1)->getGeometry() != inner ||
        commonExpression->getOperand(0)->getChildExpression() != nullptr ||
        commonExpression->getOperand(-1) != nullptr || commonExpression->getOperand(2) != nullptr) return 120;
    ConstructiveSolidGeometryByRaySegment commonParent(BooleanSetOperations::UNION);
    ConstructiveSolidGeometryByRaySegment* commonChild = new ConstructiveSolidGeometryByRaySegment(BooleanSetOperations::INTERSECTION);
    commonChild->addShape(new NativeGeometryBinding(new Sphere(1)), nullptr);
    commonParent.addShape(commonChild, nullptr,
        new Matrix4x4d(Matrix4x4d().translation(5,0,0).transpose()),
        new Matrix4x4d(Matrix4x4d().translation(-5,0,0).transpose()));
    const CsgExpression* commonParentView = &commonParent;
    Matrix4x4d operandForward, operandInverse;
    if (commonParentView->getOperand(0)->queryAffinePlacement(operandForward,operandInverse) != AffinePlacementStatus::Valid ||
        operandInverse.multiply(Vector3Dd()).x() != -5 ||
        commonParentView->getOperand(0)->getChildExpression() != static_cast<CsgExpression*>(commonChild) ||
        commonParentView->getOperand(0)->getAffinePlacement().multiply(Vector3Dd()).x() != 5 ||
        commonParentView->getOperand(0)->getChildExpression()->getOperation() != CsgOperation::INTERSECTION) return 121;
    shell.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != -2 || boxLocalBounds.getMaximum().x() != 2) return 110;
    ConstructiveSolidGeometryByRaySegment placedCsg(BooleanSetOperations::UNION);
    placedCsg.addShape(new NativeGeometryBinding(new Sphere(1)), nullptr,
        new Matrix4x4d(Matrix4x4d().translation(5,0,0).transpose()),
        new Matrix4x4d(Matrix4x4d().translation(-5,0,0).transpose()));
    placedCsg.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Finite ||
        boxLocalBounds.getMinimum().x() != 4 || boxLocalBounds.getMaximum().x() != 6) return 111;
    placedCsg.invertGeometry();
    placedCsg.getBounds(boxLocalBounds);
    if (boxLocalBounds.getKind() != GeometryBounds::Kind::Unbounded) return 112;
    Geometry* sharedGeometry = &shell;
    TestCrossingCollector crossings;
    if (sharedGeometry->emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0.001,10), crossings) != RayCrossingSupport::Supported) return 4;
    const double expected[] = {1,2,4,5};
    if (crossings.hits.size() != 4) return 5;
    for (int i=0;i<4;++i) {
        if (std::abs(crossings.hits[i]->t-expected[i]) > 1e-10 || !crossings.hits[i]->geometry) return 6;
    }
    if (crossings.hits[0]->geometry != outer->getIntrinsicGeometry() ||
        crossings.hits[1]->geometry != inner->getIntrinsicGeometry() ||
        crossings.hits[2]->geometry != inner->getIntrinsicGeometry() ||
        crossings.hits[3]->geometry != outer->getIntrinsicGeometry()) return 28;
    IntersectionAttributes attributes;
    NativeGeometryBinding owner(new Sphere(1.0));
    for (int i=0;i<8;++i) attributes.pushDetailOwner(&owner);
    bool rejectedOverflow = false;
    try { attributes.pushDetailOwner(&owner); }
    catch (const std::overflow_error&) { rejectedOverflow = true; }
    if (!rejectedOverflow || attributes.getDetailOwnerCount()!=8) return 7;

    const int width = 41, height = 29;
    SimpleScene scene;
    auto* camera = new Camera();
    camera->setPosition(Vector3Dd(-5,-5,5));
    camera->setRotation(Matrix4x4d().eulerAnglesRotation(M_PI/4, -35*M_PI/180, 0));
    camera->updateViewportResize(width,height);
    auto* background = new SimpleBackground();
    background->setColor(0.2,0.3,0.4);
    java::ArrayList<RayCastingBody*> regions;
    auto* material = new SimpleMaterial(SimpleMaterial().withDiffuse(ColorRgb(0.8,0.5,0.3)));
    auto* bodySphere = new NativeGeometryBinding(new Sphere(1.0));
    auto* body = new RayCastingBody(bodySphere, material, nullptr,
        nullptr, false, regions, regions);
    SimpleBody* sharedBody = body;
    if (sharedBody->getSurfaceMaterial()!=material || sharedBody->getMaterial()!=material) return 8;
    TestCrossingCollector bodyCrossings;
    if (sharedBody->emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0.001,10), bodyCrossings) != RayCrossingSupport::Supported) return 9;
    if (bodyCrossings.hits.size()!=2 || bodyCrossings.hits[0]->body!=body ||
        bodyCrossings.hits[0]->material!=material ||
        bodyCrossings.hits[0]->geometry!=bodySphere->getIntrinsicGeometry() ||
        bodyCrossings.hits[1]->geometry!=bodySphere->getIntrinsicGeometry()) return 10;
    scene.addBody(body);
    scene.addCamera(camera);
    scene.addBackground(background);
    scene.addLight(new PointLight(Vector3Dd(3,-3,4), ColorRgb(1,1,1)));
    auto* snapshot = scene.exportToSimpleSceneSnapshot(width,height);
    RGBImageUncompressed serial, parallel;
    if (!serial.init(width,height) || !parallel.init(width,height)) return 1;
    RendererConfiguration quality;
    SimpleRaytracer renderer;
    renderer.execute(&serial, &quality, snapshot, nullptr);
    ParallelRaytracer concurrent;
    concurrent.execute(&parallel, &quality, snapshot, false);
    concurrent.dispose();
    int differentFromCorner = 0;
    RGBPixel corner;
    serial.getPixelRgb(0,0,&corner);
    for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
        RGBPixel a,b;
        serial.getPixelRgb(x,y,&a);
        parallel.getPixelRgb(x,y,&b);
        if (a.r!=b.r || a.g!=b.g || a.b!=b.b) return 2;
        if (a.r!=corner.r || a.g!=corner.g || a.b!=corner.b) ++differentFromCorner;
    }
    delete snapshot;
    if (!differentFromCorner) { std::fprintf(stderr,"Shared sphere was not rendered\n"); return 3; }
    return 0;
}
