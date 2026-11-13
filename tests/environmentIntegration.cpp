#include <cmath>
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include <cstdio>
#include "environment/scene/Scene.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingBox.h"
#include "vsdk/toolkit/environment/raycasting/geometry/surface/RayCastingInfinitePlane.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.h"
#include "render/bakedScene/BakedSceneBuilder.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "java/util/ArrayList.txx"

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
        SimpleBody equivalentNative;
        equivalentNative.setGeometry(new Sphere(1));
        require(equivalentNative.setAffinePlacement(spherePlacement,sphereInverse)==AffinePlacementStatus::Valid,
            "shared sphere rejected its affine placement");
        CsgBodyOperand povIdentity(certifiedSphere), nativeIdentity(equivalentNative);
        require(povIdentity.isSameSolid(nativeIdentity) && nativeIdentity.isSameSolid(povIdentity),
            "equivalent POV and VITRAL spheres were not recognized");
        CsgTree identicalUnion(CsgOperation::UNION);
        require(identicalUnion.addOperand(povIdentity) && identicalUnion.addOperand(nativeIdentity),
            "could not create equivalent POV and VITRAL composition");
        CsgPointAttribution owners;
        require(CsgPointClassification::classifyWithAttribution(identicalUnion,Vector3Dd(5.05,0,0),0.1,owners)
            ==GeometryContainment::Boundary && owners.isComplete() && owners.getCount()==2 &&
            owners.getOwner(0)->getOperand(0)==&povIdentity && owners.getOwner(1)->getOperand(0)==&nativeIdentity,
            "coincident POV and VITRAL boundary owners were not preserved");

        SimpleBody nativeBox; nativeBox.setGeometry(new Box(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)));
        RayCastingBody povBox(new RayCastingBox(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        CsgBodyOperand nativeBoxOperand(nativeBox), povBoxOperand(povBox);
        require(nativeBoxOperand.isSameSolid(povBoxOperand) && povBoxOperand.isSameSolid(nativeBoxOperand),
            "equivalent POV and VITRAL boxes were not recognized");
        SimpleBody nativeHalfSpace; nativeHalfSpace.setGeometry(new PlanarHalfSpace(1,2,0,-3));
        RayCastingBody povHalfSpace(new RayCastingInfinitePlane(Vector3Dd(2,4,0),-6),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        CsgBodyOperand nativeHalfOperand(nativeHalfSpace), povHalfOperand(povHalfSpace);
        require(nativeHalfOperand.isSameSolid(povHalfOperand) && povHalfOperand.isSameSolid(nativeHalfOperand),
            "equivalent POV and VITRAL half-spaces were not recognized");
        RayCastingBody povQuadric(new Quadric(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1),nullptr,nullptr,
            nullptr,false,noRegions,noRegions);
        // Stage F-1: POV box and plane leaves use the shared certificate under
        // nonuniform, reflected and sheared placement, including binding inversion.
        {
            RayCastingBody placedBox(new RayCastingBox(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)),nullptr,nullptr,
                nullptr,false,noRegions,noRegions);
            const Matrix4x4d boxForward=Matrix4x4d().translation(10,0,0).multiply(Matrix4x4d().scale(-2,3,1));
            require(placedBox.setAffinePlacement(boxForward,boxForward.inverse())==AffinePlacementStatus::Valid,
                "POV box rejected a reflected nonuniform placement");
            // World x in [8,12], y in [-3,3], z in [-1,1].
            require(placedBox.classifyPointCertified(Vector3Dd(10,0,0),0.1)==GeometryContainment::Inside &&
                placedBox.classifyPointCertified(Vector3Dd(8.05,0,0),0.1)==GeometryContainment::Boundary &&
                placedBox.classifyPointCertified(Vector3Dd(7.85,0,0),0.1)==GeometryContainment::Outside &&
                placedBox.classifyPointCertified(Vector3Dd(10,2.95,0),0.1)==GeometryContainment::Boundary,
                "placed POV box classification differs from its world-space distances");
            RayCastingBody invertedBox(new RayCastingBox(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1),true),nullptr,nullptr,
                nullptr,false,noRegions,noRegions);
            require(invertedBox.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Outside &&
                invertedBox.classifyPointCertified(Vector3Dd(2,0,0),0.1)==GeometryContainment::Inside &&
                invertedBox.classifyPointCertified(Vector3Dd(1.05,0,0),0.1)==GeometryContainment::Boundary,
                "inverted POV box did not complement the certified classification");
            RayCastingBody shearedPlane(new RayCastingInfinitePlane(Vector3Dd(1,0,0),-1),nullptr,nullptr,
                nullptr,false,noRegions,noRegions);
            const Matrix4x4d planeForward=Matrix4x4d::identityMatrix().withVal(0,1,0.5);
            require(shearedPlane.setAffinePlacement(planeForward,planeForward.inverse())==AffinePlacementStatus::Valid,
                "POV plane rejected a sheared placement");
            // The world solid is x - 0.5 y <= 1, with unit normal (1,-0.5,0)/sqrt(1.25).
            const double unit=1.0/std::sqrt(1.25);
            require(shearedPlane.classifyPointCertified(Vector3Dd(1+0.05/unit,0,0),0.1)==GeometryContainment::Boundary &&
                shearedPlane.classifyPointCertified(Vector3Dd(1+0.15/unit,0,0),0.1)==GeometryContainment::Outside &&
                shearedPlane.classifyPointCertified(Vector3Dd(1,1,0),0.1)==GeometryContainment::Inside,
                "sheared POV plane classification differs from its world-space distances");
            SimpleBody nativeBlock; nativeBlock.setGeometry(new Box(Vector3Dd(7,-1,-1),Vector3Dd(13,1,1)));
            CsgBodyOperand blockView(nativeBlock), placedBoxView(placedBox);
            CsgTree carved(CsgOperation::DIFFERENCE);
            require(carved.addOperand(blockView) && carved.addOperand(placedBoxView),
                "could not create mixed certified difference");
            CsgPointWitness carvedWitness;
            require(CsgPointClassification::classify(carved,Vector3Dd(10,0,0),0.1)==GeometryContainment::Outside &&
                CsgPointClassification::classify(carved,Vector3Dd(7.5,0,0),0.1)==GeometryContainment::Inside &&
                CsgPointClassification::classifyWithWitness(carved,Vector3Dd(12.05,0,0),0.06,carvedWitness)
                    ==GeometryContainment::Boundary && carvedWitness.getLength()==1 &&
                carvedWitness.getOperand(0)==&placedBoxView && carvedWitness.isReversed(),
                "mixed certified difference lost classification or orientation");
            // Stage F-3: solid bounds of POV leaves for conservative culling.
            GeometryBounds solidBounds;
            placedBox.getSolidWorldBounds(solidBounds);
            require(solidBounds.getKind()==GeometryBounds::Kind::Finite && solidBounds.getMinimum().x()<=8 &&
                solidBounds.getMaximum().x()>=12 && solidBounds.getMaximum().y()>=3,
                "placed POV box lost its solid bounds");
            invertedBox.getSolidWorldBounds(solidBounds);
            require(solidBounds.getKind()==GeometryBounds::Kind::Unbounded,
                "inverted POV box reported finite solid bounds");
            CsgTree farUnion(CsgOperation::UNION);
            require(farUnion.addOperand(placedBoxView),"could not create culling composition");
            require(CsgPointClassification::classify(farUnion,Vector3Dd(100,0,0),0.1)==GeometryContainment::Outside,
                "distant POV box was not culled");
        }
        SimpleBody nativeUnitSphere; nativeUnitSphere.setGeometry(new Sphere(1));
        CsgBodyOperand nativeUnitOperand(nativeUnitSphere), povQuadricOperand(povQuadric);
        require(nativeUnitOperand.isSameSolid(povQuadricOperand) && povQuadricOperand.isSameSolid(nativeUnitOperand),
            "equivalent POV quadric and VITRAL sphere were not recognized");
        {
            const Matrix4x4d dyadicLinear=Matrix4x4d::identityMatrix().withVal(0,1,0.1).withVal(1,2,0.1);
            const Matrix4x4d dyadicInverse=Matrix4x4d::identityMatrix().withVal(0,1,-0.1).withVal(0,2,0.01).withVal(1,2,-0.1);
            SimpleBody shearedSphere; shearedSphere.setGeometry(new Sphere(1));
            RayCastingBody shiftedQuadric(new Quadric(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(0,0,2),0),
                nullptr,nullptr,nullptr,false,noRegions,noRegions);
            require(shearedSphere.setAffinePlacement(dyadicLinear,dyadicInverse)==AffinePlacementStatus::Valid &&
                shiftedQuadric.setAffinePlacement(Matrix4x4d().translation(0,0.1,1).multiply(dyadicLinear),
                    dyadicInverse.multiply(Matrix4x4d().translation(0,-0.1,-1)))==AffinePlacementStatus::Valid,
                "could not set non-representable intermediate placements");
            CsgBodyOperand sphereView(shearedSphere), quadricView(shiftedQuadric);
            require(sphereView.isSameSolid(quadricView) && quadricView.isSameSolid(sphereView),
                "dyadic certificate did not connect equivalent VITRAL and POV solids");
            CsgTree dyadicUnion(CsgOperation::UNION);
            require(dyadicUnion.addOperand(sphereView) && dyadicUnion.addOperand(quadricView),
                "could not build dyadic equivalence composition");
            CsgPointProof dyadicProof;
            require(CsgPointClassification::classifyWithProof(dyadicUnion,Vector3Dd(),0.1,dyadicProof)
                ==GeometryContainment::Inside && dyadicProof.isAvailable(),
                "dyadic equivalence could not support shared classification");
            CsgPointAttribution dyadicOwners;
            require(CsgPointClassification::classifyWithAttribution(dyadicUnion,Vector3Dd(1,0,0),0.01,dyadicOwners)
                ==GeometryContainment::Boundary && dyadicOwners.getCount()==2 &&
                dyadicOwners.getOwner(0)->getOperand(0)==&sphereView &&
                dyadicOwners.getOwner(1)->getOperand(0)==&quadricView,
                "dyadic equivalence lost coincident boundary attribution");
        }

        {
            CsgTree crossFamily(CsgOperation::UNION);
            require(crossFamily.addOperand(nativeUnitOperand) && crossFamily.addOperand(povQuadricOperand),
                "could not create equivalent cross-family composition");
            CsgPointAttribution crossOwners;
            require(CsgPointClassification::classifyWithAttribution(crossFamily,Vector3Dd(1,0,0),0.01,crossOwners)
                ==GeometryContainment::Boundary && crossOwners.getCount()==2 &&
                crossOwners.getOwner(0)->getOperand(0)==&nativeUnitOperand &&
                crossOwners.getOwner(1)->getOperand(0)==&povQuadricOperand,
                "equivalent POV quadric did not inherit the certified boundary or attribution");
            CsgPointProof crossProof;
            require(CsgPointClassification::classifyWithProof(crossFamily,Vector3Dd(1,0,0),0.01,crossProof)
                ==GeometryContainment::Boundary && crossProof.isAvailable(),"cross-family classification lost its proof");
            // Both leaves certify their own boundary through the shared routine,
            // so the proof needs no equivalence transfer.
            require(povQuadricOperand.classifyPointCertified(Vector3Dd(1,0,0),0.01)==GeometryContainment::Boundary &&
                povQuadricOperand.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Inside &&
                povQuadricOperand.classifyPointCertified(Vector3Dd(1.2,0,0),0.1)==GeometryContainment::Outside,
                "POV quadric leaf did not use the shared certified classification");
            bool hasEquivalence=false;
            for (long i=0;i<crossProof.getCount();++i)
                if (crossProof.getEvidence(i)->isEquivalence()) hasEquivalence=true;
            require(!hasEquivalence && crossProof.getCount()==2,
                "directly certified cross-family leaves used an equivalence transfer");
            CsgTree nativeBranch(CsgOperation::UNION), povBranch(CsgOperation::UNION), branchRoot(CsgOperation::UNION);
            require(nativeBranch.addOperand(nativeUnitOperand) && povBranch.addOperand(povQuadricOperand) &&
                branchRoot.addOperand(povBranch) && branchRoot.addOperand(nativeBranch),
                "could not create equivalent mixed-project subtrees");
            require(nativeBranch.isSameSolid(povBranch) && povBranch.isSameSolid(nativeBranch),
                "equivalent POV and VITRAL subtrees were not recognized");
            CsgPointProof branchProof; CsgPointAttribution branchOwners;
            require(CsgPointClassification::classifyWithProof(branchRoot,Vector3Dd(1,0,0),0.01,branchProof)
                ==GeometryContainment::Boundary && branchProof.isAvailable(),
                "equivalent POV subtree lost its certified proof");
            // The POV subtree resolves its own leaf, so the owner set is complete.
            require(CsgPointClassification::classifyWithAttribution(branchRoot,Vector3Dd(1,0,0),0.01,branchOwners)
                ==GeometryContainment::Boundary && branchOwners.isComplete() && branchOwners.getCount()==2,
                "equivalent POV and VITRAL subtrees did not attribute both certified owners");


            CsgRayLeafBatch crossLeaves; CsgRayEventBatch crossEvents;
            require(CsgRayLeafQuery::query(crossFamily,Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
                RayCrossingQuery(0,10),crossLeaves)==RaySolidQueryStatus::Supported &&
                CsgRayEventQuery::group(crossLeaves,crossEvents)==RaySolidQueryStatus::Supported &&
                crossEvents.size()==2 && crossEvents.get(0)->size()==2 && crossEvents.get(1)->size()==2,
                "cross-family coincident ray events were not preserved");
        }
        static_cast<RayCastingBox*>(povBox.getGeometry())->setInverted(true);
        require(!nativeBoxOperand.isSameSolid(povBoxOperand),"inverted box was incorrectly equated to a solid box");
        static_cast<RayCastingBox*>(povBox.getGeometry())->setInverted(false);
        povBox.getClippingShapes().add(nullptr);
        require(!nativeBoxOperand.isSameSolid(povBoxOperand),"clipped box was incorrectly equated to a solid box");
        povBox.getClippingShapes().clear();
        povBox.getBoundingShapes().add(nullptr);
        require(!nativeBoxOperand.isSameSolid(povBoxOperand),"bounded POV box was incorrectly equated to a solid box");
        povBox.getBoundingShapes().clear();


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
        SimpleBody localSphere;
        localSphere.setGeometry(new Sphere(1));
        CsgBodyOperand povOperand(certifiedSphere), localOperand(localSphere);
        CsgTree commonUnion(CsgOperation::UNION);
        require(commonUnion.addOperand(povOperand) && commonUnion.addOperand(localOperand),
            "could not create common POV and VITRAL composition");
        require(CsgPointClassification::classify(commonUnion,Vector3Dd(3,0,0),0.1)==GeometryContainment::Inside,
            "common composition lost POV certified interior");
        require(CsgPointClassification::classify(commonUnion,Vector3Dd(5.1,0,0),0.1)==GeometryContainment::Indeterminate,
            "common composition lost POV uncertainty");
        CsgPlacedExpression placedCommon(commonUnion);
        const Matrix4x4d parentForward=Matrix4x4d().translation(10,0,0).multiply(Matrix4x4d().scale(2,1,1));
        const Matrix4x4d parentInverse=Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-10,0,0));
        require(placedCommon.setAffinePlacement(parentForward,parentInverse)==AffinePlacementStatus::Valid,
            "mixed composition rejected parent placement");
        CsgTree placedRoot(CsgOperation::UNION);
        require(placedRoot.addOperand(placedCommon),"could not attach placed composition");
        CsgPointWitness mixedWitness;
        require(CsgPointClassification::classifyWithWitness(placedRoot,Vector3Dd(16,0,0),0.1,mixedWitness)
            ==GeometryContainment::Inside && mixedWitness.getLength()==2 &&
            mixedWitness.getOperand(0)==&placedCommon && mixedWitness.getOperand(1)==&povOperand,
            "placed mixed composition lost its POV point witness");
        require(CsgPointClassification::classify(placedRoot,Vector3Dd(20.1,0,0),0.1)
            ==GeometryContainment::Indeterminate,
            "placed mixed composition forced an uncertain tolerance result");
        CsgRayLeafBatch mixedRayLeaves;
        const Ray mixedRay=Ray::fromAffine(Vector3Dd(),Vector3Dd(1,0,0));
        const RayCrossingQuery mixedRayRange(0,30);
        require(CsgRayLeafQuery::query(placedRoot,mixedRay,mixedRayRange,mixedRayLeaves)
            ==RaySolidQueryStatus::Supported && mixedRayLeaves.size()==2,
            "mixed composition failed to collect typed ray leaves");
        require(mixedRayLeaves.get(0)->getClassification().boundaries.size()==2 &&
            mixedRayLeaves.get(1)->getClassification().boundaries.size()==2 &&
            mixedRayLeaves.get(0)->getClassification().boundaries[0].t==12 &&
            mixedRayLeaves.get(0)->getClassification().boundaries[1].t==20 &&
            mixedRayLeaves.get(1)->getClassification().boundaries[0].t==8 &&
            mixedRayLeaves.get(1)->getClassification().boundaries[1].t==12,
            "mixed composition lost a placed or coincident crossing");
        CsgRayEventBatch mixedEvents;
        require(CsgRayEventQuery::group(mixedRayLeaves,mixedEvents)==RaySolidQueryStatus::Supported &&
            mixedEvents.size()==3 && mixedEvents.get(0)->getT()==8 && mixedEvents.get(1)->getT()==12 &&
            mixedEvents.get(2)->getT()==20 && mixedEvents.get(1)->size()==2 &&
            mixedEvents.get(1)->get(0)->leafIndex==0 && mixedEvents.get(1)->get(1)->leafIndex==1,
            "mixed composition lost coincident POV and VITRAL ray events");
        certifiedSphere.invert();
        require(!povIdentity.isSameSolid(nativeIdentity),"inverted POV body was incorrectly equated to a non-inverted sphere");
        require(certifiedSphere.encloseMinimumBoundaryDistance(Vector3Dd(5.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Inside,
            "inverted POV sphere failed to reverse the certified side");
        require(commonSphere.encloseBoundaryDistance(Vector3Dd(5.2,0,0),Vector3Dd(1,0,0),
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Valid &&
            certifiedSide==GeometryContainment::Inside,
            "inverted POV sphere failed to reverse the enclosure side");
        require(CsgPointClassification::classify(commonUnion,Vector3Dd(5.2,0,0),0.1)==GeometryContainment::Inside,
            "common composition lost the inverted POV operand orientation");
        require(CsgRayLeafQuery::query(placedRoot,mixedRay,mixedRayRange,mixedRayLeaves)
            ==RaySolidQueryStatus::Supported && mixedRayLeaves.size()==2 &&
            mixedRayLeaves.get(0)->getClassification().insideSegments[0] &&
            mixedRayLeaves.get(0)->getClassification().boundaries[0].normalReversed,
            "inverted POV leaf lost typed ray membership or normal orientation");
        certifiedSphere.getClippingShapes().add(nullptr);
        GeometryBounds clippedBounds;
        certifiedSphere.getSolidWorldBounds(clippedBounds);
        require(clippedBounds.getKind()==GeometryBounds::Kind::Unsupported,
            "clipped POV body exposed unclipped solid bounds before its region policy exists");
        require(CsgPointClassification::classify(identicalUnion,Vector3Dd(5.05,0,0),0.1)
            ==GeometryContainment::Unsupported,"clipped POV body inherited a result from a non-equivalent sphere");

        require(!povIdentity.isSameSolid(nativeIdentity),"clipped POV body was incorrectly equated to an unclipped sphere");
        certifiedLower=17; certifiedUpper=23; certifiedSide=GeometryContainment::Boundary;
        require(certifiedSphere.encloseMinimumBoundaryDistance(Vector3Dd(5.2,0,0),1e-6,
            certifiedSide,certifiedLower,certifiedUpper)==GeometryDistanceStatus::Unsupported &&
            certifiedLower==17 && certifiedUpper==23 && certifiedSide==GeometryContainment::Boundary,
            "clipped POV sphere was incorrectly accepted or mutated outputs");
        require(CsgPointClassification::classify(commonUnion,Vector3Dd(5.2,0,0),0.1)==GeometryContainment::Unsupported,
            "common composition silently accepted the clipped POV operand");
        require(CsgRayLeafQuery::query(placedRoot,mixedRay,mixedRayRange,mixedRayLeaves)
            ==RaySolidQueryStatus::Unsupported && mixedRayLeaves.size()==0,
            "typed ray composition silently ignored POV clipping or retained partial results");
        certifiedSphere.getClippingShapes().clear();
        certifiedSphere.getBoundingShapes().add(nullptr);
        require(certifiedSphere.classifyPointCertified(Vector3Dd(5.2,0,0),0.1)==GeometryContainment::Unsupported,
            "bounded POV sphere was incorrectly accepted as an unbounded leaf");
        require(CsgRayLeafQuery::query(placedRoot,mixedRay,mixedRayRange,mixedRayLeaves)
            ==RaySolidQueryStatus::Unsupported && mixedRayLeaves.size()==0,
            "typed ray composition silently ignored POV bounds or retained partial results");
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
        // Stage F-4: typed POV region policies. clipped_by is the shared
        // conservative intersection with the clip regions; bounded_by is only a
        // hint that must be certified to contain the solid.
        {
            java::ArrayList<RayCastingBody*> none;
            RayCastingBody clippedSphere(new NativeGeometryBinding(new Sphere(2)),nullptr,nullptr,
                nullptr,false,none,none);
            clippedSphere.getClippingShapes().add(new RayCastingBody(
                new RayCastingBox(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)),nullptr,nullptr,nullptr,false,none,none));
            require(clippedSphere.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Inside &&
                clippedSphere.classifyPointCertified(Vector3Dd(0.9,0.9,0.9),0.05)==GeometryContainment::Inside &&
                clippedSphere.classifyPointCertified(Vector3Dd(1.05,0,0),0.1)==GeometryContainment::Boundary &&
                clippedSphere.classifyPointCertified(Vector3Dd(1.5,0,0),0.1)==GeometryContainment::Outside &&
                clippedSphere.classifyPointCertified(Vector3Dd(3,0,0),0.1)==GeometryContainment::Outside,
                "clipped POV sphere does not classify as its intersection with the clip region");
            GeometryBounds clippedBounds;
            clippedSphere.getSolidWorldBounds(clippedBounds);
            require(clippedBounds.getKind()==GeometryBounds::Kind::Finite && clippedBounds.getMaximum().x()<1.001 &&
                clippedBounds.getMinimum().x()>-1.001,"clipped POV sphere bounds ignore the clip region");
            RayCastingBody hollowSphere(new NativeGeometryBinding(new Sphere(2)),nullptr,nullptr,
                nullptr,false,none,none);
            hollowSphere.getClippingShapes().add(new RayCastingBody(
                new RayCastingBox(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1),true),nullptr,nullptr,nullptr,false,none,none));
            require(hollowSphere.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Outside &&
                hollowSphere.classifyPointCertified(Vector3Dd(1.5,0,0),0.1)==GeometryContainment::Inside &&
                hollowSphere.classifyPointCertified(Vector3Dd(0.95,0,0),0.1)==GeometryContainment::Boundary,
                "inverted POV clip region was not subtracted");
            RayCastingBody boundedSphere(new NativeGeometryBinding(new Sphere(1)),nullptr,nullptr,
                nullptr,false,none,none);
            boundedSphere.getBoundingShapes().add(new RayCastingBody(
                new RayCastingBox(Vector3Dd(-2,-2,-2),Vector3Dd(2,2,2)),nullptr,nullptr,nullptr,false,none,none));
            require(boundedSphere.classifyPointCertified(Vector3Dd(0.95,0,0),0.1)==GeometryContainment::Boundary &&
                boundedSphere.classifyPointCertified(Vector3Dd(1.5,0,0),0.1)==GeometryContainment::Outside,
                "verified bounded_by hint changed the POV sphere classification");
            RayCastingBody tightBound(new NativeGeometryBinding(new Sphere(1)),nullptr,nullptr,
                nullptr,false,none,none);
            tightBound.getBoundingShapes().add(new RayCastingBody(
                new RayCastingBox(Vector3Dd(-0.5,-0.5,-0.5),Vector3Dd(0.5,0.5,0.5)),nullptr,nullptr,nullptr,false,none,none));
            GeometryBounds tightBounds;
            tightBound.getSolidWorldBounds(tightBounds);
            require(tightBound.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Unsupported &&
                tightBounds.getKind()==GeometryBounds::Kind::Unsupported,
                "an unverified bounded_by hint was given a typed solid");
            RayCastingBody sphereBound(new NativeGeometryBinding(new Sphere(1)),nullptr,nullptr,
                nullptr,false,none,none);
            sphereBound.getBoundingShapes().add(new RayCastingBody(
                new NativeGeometryBinding(new Sphere(1.8)),nullptr,nullptr,nullptr,false,none,none));
            require(sphereBound.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Inside,
                "a spherical bounded_by region could not be certified");
            RaySolidClassification clippedRay;
            require(clippedSphere.classifyRay(Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(1,0,0)),
                RayCrossingQuery(0,10),clippedRay)==RaySolidQueryStatus::Unsupported,
                "clipped ray classification claimed a Boolean combination that belongs to stage G");
            // Stage F-5: arbitrary affine edits move owned regions with the object.
            const Matrix4x4d moved=Matrix4x4d().translation(5,0,0).multiply(
                Matrix4x4d::identityMatrix().withVal(0,1,0.5));
            require(clippedSphere.setAffinePlacement(moved,moved.inverse())==AffinePlacementStatus::Valid,
                "clipped POV sphere rejected a valid affine edit");
            require(clippedSphere.classifyPointCertified(Vector3Dd(5,0,0),0.1)==GeometryContainment::Inside &&
                clippedSphere.classifyPointCertified(Vector3Dd(0,0,0),0.1)==GeometryContainment::Outside &&
                clippedSphere.classifyPointCertified(moved.multiply(Vector3Dd(1.05,0,0)),0.06)==GeometryContainment::Boundary,
                "clip region did not follow the object's affine edit");
            require(clippedSphere.getClippingShapes()[0]->getAffinePlacement().epsilonEquals(moved,1e-12),
                "clip region placement differs from the applied object-level change");
            require(clippedSphere.setAffinePlacement(Matrix4x4d::identityMatrix(),Matrix4x4d::identityMatrix())
                ==AffinePlacementStatus::Valid &&
                clippedSphere.getClippingShapes()[0]->getAffinePlacement().epsilonEquals(Matrix4x4d::identityMatrix(),1e-12),
                "returning the clipped body to identity left its clip region displaced");
            // A region that cannot move atomically rejects the whole edit.
            RayCastingBody* brokenRegion=new RayCastingBody(
                new RayCastingBox(Vector3Dd(-3,-3,-3),Vector3Dd(3,3,3)),nullptr,nullptr,nullptr,false,none,none);
            brokenRegion->getClippingShapes().add(nullptr);
            hollowSphere.getBoundingShapes().add(brokenRegion);
            const Matrix4x4d beforeEdit=hollowSphere.getAffinePlacement();
            const Matrix4x4d clipBefore=hollowSphere.getClippingShapes()[0]->getAffinePlacement();
            require(hollowSphere.setAffinePlacement(moved,moved.inverse())==AffinePlacementStatus::Unsupported &&
                hollowSphere.getAffinePlacement().epsilonEquals(beforeEdit,0) &&
                hollowSphere.getClippingShapes()[0]->getAffinePlacement().epsilonEquals(clipBefore,0),
                "a rejected region edit changed part of the POV object");
            hollowSphere.getBoundingShapes()[0]->getClippingShapes().clear();
        }
    } catch (...) { return 1; }
    return 0;
}
