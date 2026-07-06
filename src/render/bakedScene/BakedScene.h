#ifndef __BAKED_SCENE__
#define __BAKED_SCENE__

#include "render/bakedScene/BakedSceneKinds.h"
#include "render/bakedScene/CsgOperandRecord.h"
#include "render/bakedScene/OperandCullBins.h"
#include "render/bakedScene/CsgProgram.h"
#include "render/bakedScene/TraceableObject.h"
#include "render/bakedScene/CompositeRecord.h"
#include "render/bakedScene/BakedSceneStatistics.h"

class BakedScene {
  public:
    using TraceKind = BakedSceneTraceKind;
    using CsgAlgorithm = BakedSceneCsgAlgorithm;
    using CsgPlanKind = BakedSceneCsgPlanKind;
    using CsgOperandKind = BakedSceneCsgOperandKind;

    BakedScene() = default;
    BakedScene(const BakedScene &) = delete;
    BakedScene &operator=(const BakedScene &) = delete;
    ~BakedScene()
    {
        clearOwnedContents();
        delete statistics;
    }

    void clearOwnedContents()
    {
        for (long int i = 0; i < traceableObjects.size(); i++) {
            delete traceableObjects[i];
        }
        traceableObjects.clear();
        for (long int i = 0; i < csgPrograms.size(); i++) {
            delete csgPrograms[i];
        }
        csgPrograms.clear();
        for (long int i = 0; i < operandCullBinsStorage.size(); i++) {
            delete operandCullBinsStorage[i];
        }
        operandCullBinsStorage.clear();
        for (long int i = 0; i < composites.size(); i++) {
            delete composites[i];
        }
        composites.clear();
    }

    java::ArrayList<TraceableObject *> traceableObjects;
    java::ArrayList<CsgProgram *> csgPrograms;
    java::ArrayList<OperandCullBins *> operandCullBinsStorage;
    java::ArrayList<CompositeRecord *> composites;
    java::ArrayList<int> topLevelObjectIndices;
    java::ArrayList<int> boundedObjectIndices;
    java::ArrayList<int> unboundedObjectIndices;
    java::ArrayList<int> shadowCastingObjectIndices;
    java::ArrayList<int> boundedShadowCastingObjectIndices;
    java::ArrayList<int> unboundedShadowCastingObjectIndices;
    BakedSceneStatistics *statistics = nullptr;
};

#endif
