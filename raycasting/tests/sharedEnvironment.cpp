#include <cmath>
#include <cstdio>
#include <stdexcept>
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
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
#include "java/lang/UnsupportedOperationException.h"
#include "TestSlabGeometry.h"
#include "TestCrossingCollector.h"

int main() {
    NativeGeometryBinding slab(new TestSlabGeometry());
    const Ray slabRay = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
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
    NativeGeometryBinding unsupported(new Box(1,1,1));
    bool rejectedUnsupported = false;
    try { unsupported.doIntersectionFirstHit(&slabContext.ray, slabHit); }
    catch (const java::UnsupportedOperationException&) { rejectedUnsupported = true; }
    if (!rejectedUnsupported) return 34;
    NativeGeometryBinding reversedBoundary(new TestSlabGeometry(true));
    bool rejectedProvenance = false;
    try {
        TestCrossingCollector ignored;
        static_cast<Geometry*>(&reversedBoundary)->emitRayCrossings(
            slabRay, RayCrossingQuery(0,10), ignored);
    }
    catch (const java::UnsupportedOperationException&) { rejectedProvenance = true; }
    if (!rejectedProvenance) return 35;
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
