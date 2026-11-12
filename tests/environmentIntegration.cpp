#include <cmath>
#include <cstdio>
#include "environment/scene/Scene.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.h"
#include "render/bakedScene/BakedSceneBuilder.h"

static void require(bool valid, const char* message) {
    if (!valid) { std::fprintf(stderr, "%s\n", message); throw message; }
}

int main() {
    try {
        NativeGeometryBinding typedSphere(new Sphere(3.0));
        require(typedSphere.classifyPoint(Vector3Dd(3.1,0,0),0.1)==GeometryContainment::Indeterminate,
            "native sphere silently classified an uncertain tolerance boundary");
        require(typedSphere.classifyPoint(Vector3Dd(),0.1)==GeometryContainment::Inside,
            "native sphere lost its certified interior");
        typedSphere.invertGeometry();
        require(typedSphere.classifyPoint(Vector3Dd(3.1,0,0),0.1)==GeometryContainment::Indeterminate,
            "inverted native sphere did not preserve uncertainty");
        require(typedSphere.classifyPoint(Vector3Dd(),0.1)==GeometryContainment::Outside,
            "inverted native sphere lost its certified exterior");
        java::ArrayList<RayCastingBody*> noRegions;
        RayCastingBody certifiedSphere(new NativeGeometryBinding(new Sphere(1)),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        const Matrix4x4d spherePlacement=Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1));
        const Matrix4x4d sphereInverse=Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0));
        require(certifiedSphere.setAffinePlacement(spherePlacement,sphereInverse)==AffinePlacementStatus::Valid,
            "native sphere rejected its affine placement");
        GeometryContainment certifiedSide=GeometryContainment::Indeterminate;
        double certifiedLower=0,certifiedUpper=0;
        const SimpleBody& commonSphere=certifiedSphere;
        require(commonSphere.encloseMinimumBoundaryDistance(Vector3Dd(5.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid,
            "POV sphere failed to expose the certified minimum distance");
        require(certifiedSide==GeometryContainment::Outside && certifiedLower<0.20000001 &&
            certifiedUpper>0.19999999 && certifiedUpper-certifiedLower<=1e-6,
            "POV sphere returned incorrect certified distance or side");
        require(commonSphere.encloseBoundaryDistance(Vector3Dd(5.2,0,0),Vector3Dd(1,0,0),
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Outside && certifiedLower<0.20000001 && certifiedUpper>0.19999999,
            "POV sphere failed to expose the shared distance enclosure");
        double estimatedDistance=0,estimatedError=0;
        require(commonSphere.estimateBoundaryDistance(Vector3Dd(5.2,0,0),estimatedDistance,estimatedError)
            ==GeometryDistanceStatus::Valid && estimatedDistance>0.19999999 && estimatedDistance<0.20000001,
            "POV sphere failed to expose the shared distance estimate");
        require(commonSphere.classifyPointCertified(Vector3Dd(5.05,0,0),0.1)==GeometryContainment::Boundary,
            "POV sphere lost its certified tolerance boundary");
        require(certifiedSphere.classifyPointCertified(Vector3Dd(5.1,0,0),0.1)==GeometryContainment::Indeterminate,
            "POV sphere silently resolved an uncertain threshold");
        certifiedSphere.invert();
        require(certifiedSphere.encloseMinimumBoundaryDistance(Vector3Dd(5.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Inside,
            "inverted POV sphere failed to reverse the certified side");
        require(commonSphere.encloseBoundaryDistance(Vector3Dd(5.2,0,0),Vector3Dd(1,0,0),
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Inside,
            "inverted POV sphere failed to reverse the enclosure side");
        certifiedSphere.getClippingShapes().add(nullptr);
        certifiedLower=17; certifiedUpper=23; certifiedSide=GeometryContainment::Boundary;
        require(certifiedSphere.encloseMinimumBoundaryDistance(Vector3Dd(5.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Unsupported &&
            certifiedLower==17 && certifiedUpper==23 && certifiedSide==GeometryContainment::Boundary,
            "clipped POV sphere was incorrectly accepted or mutated outputs");
        certifiedSphere.getClippingShapes().clear();
        certifiedSphere.getBoundingShapes().add(nullptr);
        require(certifiedSphere.classifyPointCertified(Vector3Dd(5.2,0,0),0.1)==GeometryContainment::Unsupported,
            "bounded POV sphere was incorrectly accepted as an unbounded leaf");
        RayCastingBody layeredSphere(new NativeGeometryBinding(new Sphere(1)),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        Vector3Dd sphereGeometryScale(2,1,1),sphereOuterTranslation(5,0,0);
        layeredSphere.scaleGeometryLayer(&sphereGeometryScale);
        layeredSphere.translate(&sphereOuterTranslation);
        require(layeredSphere.encloseMinimumBoundaryDistance(Vector3Dd(7.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Outside && certifiedLower<0.20000001 && certifiedUpper>0.19999999,
            "layered POV placement did not reach the shared certified distance query");
        RayCastingBody reflectedSphere(new NativeGeometryBinding(new Sphere(2)),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        const Matrix4x4d reflected=Matrix4x4d().translation(1,-2,3).multiply(Matrix4x4d().scale(-2,1,1));
        const Matrix4x4d reflectedInverse=Matrix4x4d().scale(-0.5,1,1).multiply(Matrix4x4d().translation(-1,2,-3));
        require(reflectedSphere.setAffinePlacement(reflected,reflectedInverse)==AffinePlacementStatus::Valid,
            "reflected POV sphere rejected its placement");
        require(reflectedSphere.encloseMinimumBoundaryDistance(Vector3Dd(-3.2,-2,3),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Outside && certifiedLower<0.20000001 && certifiedUpper>0.19999999,
            "reflected non-unit POV sphere returned incorrect certified bounds");
        Scene scene;
        java::ArrayList<RayCastingBody*> regions;
        auto* body = new RayCastingBody(new NativeGeometryBinding(new Sphere(1.0)), nullptr, nullptr,
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
        require(std::abs(sharedBody->getTransformationMatrix().multiply(Vector3Dd()).x()-5) < 1e-10,
            "shared placement does not describe the body");
        require(!sharedBody->supportsDecomposedPlacement(), "affine body advertises TRS editing");
        sharedBody->setPosition(Vector3Dd(6,0,0));
        require(std::abs(sharedBody->getPosition().x()-6) < 1e-10, "shared translation was ignored");
        auto* snapshot = sharedScene.exportToSimpleSceneSnapshot();
        require(snapshot->getSimpleBodies()[0] == body, "snapshot copied the body graph");
        delete snapshot;
        scene.resetForSceneParse();
        require(sharedScene.getSimpleBodies().size() == 0, "reset retained owned bodies");
        RayCastingBody quadricBody(new Quadric(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1),
            nullptr,nullptr,nullptr,false,regions,regions);
        Vector3Dd priorTranslation(1,0,0);
        quadricBody.translate(&priorTranslation);
        const Matrix4x4d shear = Matrix4x4d().translation(2,0,0).multiply(
            Matrix4x4d::identityMatrix().withVal(0,1,0.5));
        const Matrix4x4d inverse = Matrix4x4d::identityMatrix().withVal(0,1,-0.5).multiply(
            Matrix4x4d().translation(-2,0,0));
        require(quadricBody.setAffinePlacement(shear,inverse) == AffinePlacementStatus::Valid,
            "quadric rejected valid affine placement");
        java::ArrayList<SimpleBody*> bakedBodies;
        bakedBodies.add(&quadricBody);
        BakedScene baked;
        BakedSceneBuilder::build(bakedBodies,baked);
        require(baked.traceableObjects.size()==1 && !baked.traceableObjects[0]->getHasBakedQuadric(),
            "arbitrary affine placement was replaced by stale TRS baking");
        require(baked.traceableObjects[0]->getObjectToWorld().transpose().epsilonEquals(shear,1e-12),
            "baked scene lost the affine placement");
    } catch (...) { return 1; }
    return 0;
}
