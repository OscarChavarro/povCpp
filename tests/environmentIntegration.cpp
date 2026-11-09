#include <cmath>
#include <cstdio>
#include "environment/scene/Scene.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingSphere.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"

static void require(bool valid, const char* message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); throw message; }
}

int main() {
    try {
        Scene scene;
        java::ArrayList<RayCastingBody*> regions;
        auto* body = new RayCastingBody(new RayCastingSphere(1.0), nullptr, nullptr,
            nullptr, false, regions, regions);
        Geometry* geometry = body->getGeometry();
        RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
        require(geometry->doIntersectionFirstHit(Ray(Vector3Dd(-3,0,0), Vector3Dd(1,0,0)), &hit),
            "shared Geometry query missed sphere");
        require(std::abs(hit.getHitDistance()-2.0) < 1e-10, "wrong sphere distance");
        require(std::abs(hit.normal.x()+1.0) < 1e-10, "wrong sphere normal");
        Vector3Dd translation(5,0,0);
        body->translate(&translation);
        java::ArrayList<RayCastingBody*> bodies;
        bodies.add(body);
        scene.setObjects(bodies);
        SimpleScene& sharedScene = scene;
        require(sharedScene.getSimpleBodies()[0] == body, "scene copied the body graph");
        SimpleBody* sharedBody = sharedScene.getSimpleBodies()[0];
        hit.reset(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
        require(sharedBody->doIntersectionFirstHit(Ray(Vector3Dd(0,0,0), Vector3Dd(1,0,0)), &hit),
            "shared body query missed transformed sphere");
        require(std::abs(hit.getHitDistance()-4.0) < 1e-10, "body lost its transform");
        require(std::abs(sharedBody->getTransformationMatrix().transformPoint(Vector3Dd()).x()-5) < 1e-10,
            "shared placement does not describe the body");
        require(!sharedBody->supportsDecomposedPlacement(), "affine body advertises TRS editing");
        sharedBody->setPosition(Vector3Dd(6,0,0));
        require(std::abs(sharedBody->getPosition().x()-6) < 1e-10, "shared translation was ignored");
        auto* snapshot = sharedScene.exportToSimpleSceneSnapshot();
        require(snapshot->getSimpleBodies()[0] == body, "snapshot copied the body graph");
        delete snapshot;
        scene.resetForSceneParse();
        require(sharedScene.getSimpleBodies().size() == 0, "reset retained owned bodies");
    } catch (...) { return 1; }
    return 0;
}
