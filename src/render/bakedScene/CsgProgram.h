#ifndef __CSG_PROGRAM__
#define __CSG_PROGRAM__

#include "render/bakedScene/BakedSceneKinds.h"
#include "render/bakedScene/CsgOperandRecord.h"
#include "render/bakedScene/OperandCullBins.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"

// One compiled CSG node: algorithm/specialization chosen once at build
// time, plus the operand-kind bucket arrays the fused-plan kernels
// iterate (planes, direct primitives, nested CSGs, transformed
// primitives) instead of re-deriving them per ray.
//
// `operands` stores its CsgOperandRecord entries by value in one contiguous
// array (not individually heap-allocated behind pointers - see
// CsgOperandRecord.h for why that distinction matters on the per-ray hot
// path). Recompiling this program (transform pushdown, execution-plan
// rebuild) means constructing a fresh CsgProgram and replacing the owning
// pointer in BakedScene::csgPrograms, never mutating this object in place.
class CsgProgram {
  public:
    CsgProgram(
        BakedSceneCsgAlgorithm algorithm,
        BakedSceneCsgPlanKind planKind,
        BooleanSetOperations geometryType,
        bool topLevel,
        bool specializationValid,
        int specializationCoreOperandIndex,
        const java::ArrayList<int> &planeOperandIndices,
        const java::ArrayList<int> &nestedOperandIndices,
        const java::ArrayList<int> &transformedPrimitiveOperandIndices,
        const java::ArrayList<int> &directPrimitiveOperandIndices,
        const java::ArrayList<CsgOperandRecord> &operands,
        const OperandCullBins *directPrimitiveCullBins,
        const OperandCullBins *transformedPrimitiveCullBins) :
        algorithm(algorithm),
        planKind(planKind),
        geometryType(geometryType),
        topLevel(topLevel),
        specializationValid(specializationValid),
        specializationCoreOperandIndex(specializationCoreOperandIndex),
        planeOperandIndices(planeOperandIndices),
        nestedOperandIndices(nestedOperandIndices),
        transformedPrimitiveOperandIndices(transformedPrimitiveOperandIndices),
        directPrimitiveOperandIndices(directPrimitiveOperandIndices),
        operands(operands),
        directPrimitiveCullBins(directPrimitiveCullBins),
        transformedPrimitiveCullBins(transformedPrimitiveCullBins)
    {}

    CsgProgram(const CsgProgram &) = delete;
    CsgProgram &operator=(const CsgProgram &) = delete;

    BakedSceneCsgAlgorithm getAlgorithm() const { return algorithm; }
    BakedSceneCsgPlanKind getPlanKind() const { return planKind; }
    BooleanSetOperations getGeometryType() const { return geometryType; }
    bool getTopLevel() const { return topLevel; }
    bool getSpecializationValid() const { return specializationValid; }
    int getSpecializationCoreOperandIndex() const { return specializationCoreOperandIndex; }
    const java::ArrayList<int> &getPlaneOperandIndices() const { return planeOperandIndices; }
    const java::ArrayList<int> &getNestedOperandIndices() const { return nestedOperandIndices; }
    const java::ArrayList<int> &getTransformedPrimitiveOperandIndices() const
    {
        return transformedPrimitiveOperandIndices;
    }
    const java::ArrayList<int> &getDirectPrimitiveOperandIndices() const
    {
        return directPrimitiveOperandIndices;
    }
    const java::ArrayList<CsgOperandRecord> &getOperands() const { return operands; }
    const OperandCullBins *getDirectPrimitiveCullBins() const { return directPrimitiveCullBins; }
    const OperandCullBins *getTransformedPrimitiveCullBins() const { return transformedPrimitiveCullBins; }

  private:
    BakedSceneCsgAlgorithm algorithm;
    BakedSceneCsgPlanKind planKind;
    BooleanSetOperations geometryType;
    bool topLevel;
    bool specializationValid;
    int specializationCoreOperandIndex;
    java::ArrayList<int> planeOperandIndices;
    java::ArrayList<int> nestedOperandIndices;
    java::ArrayList<int> transformedPrimitiveOperandIndices;
    java::ArrayList<int> directPrimitiveOperandIndices;
    java::ArrayList<CsgOperandRecord> operands;
    const OperandCullBins *directPrimitiveCullBins;
    const OperandCullBins *transformedPrimitiveCullBins;
};

#endif
