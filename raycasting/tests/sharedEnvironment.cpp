#include <cmath>
#include <cstdio>
#include <vector>
#include <stdexcept>
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingSphere.h"
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

class CrossingCollector : public RayCrossingSink {
public:
    std::vector<RayCrossing> hits;
    void crossing(const RayCrossing& hit) override { hits.push_back(hit); }
};

int main() {
    ConstructiveSolidGeometryByRaySegment shell(BooleanSetOperations::DIFFERENCE);
    shell.addShape(new RayCastingSphere(2.0), nullptr);
    shell.addShape(new RayCastingSphere(1.0), nullptr);
    Geometry* sharedGeometry = &shell;
    CrossingCollector crossings;
    if (sharedGeometry->emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0.001,10), crossings) != RayCrossingSupport::Supported) return 4;
    const double expected[] = {1,2,4,5};
    if (crossings.hits.size() != 4) return 5;
    for (int i=0;i<4;++i) {
        if (std::abs(crossings.hits[i].t-expected[i]) > 1e-10 || !crossings.hits[i].geometry) return 6;
    }
    IntersectionAttributes attributes;
    RayCastingSphere owner(1.0);
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
    auto* body = new RayCastingBody(new RayCastingSphere(1.0), material, nullptr,
        nullptr, false, regions, regions);
    SimpleBody* sharedBody = body;
    if (sharedBody->getSurfaceMaterial()!=material || sharedBody->getMaterial()!=material) return 8;
    CrossingCollector bodyCrossings;
    if (sharedBody->emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0.001,10), bodyCrossings) != RayCrossingSupport::Supported) return 9;
    if (bodyCrossings.hits.size()!=2 || bodyCrossings.hits[0].body!=body ||
        bodyCrossings.hits[0].material!=material) return 10;
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
