#ifndef VSDK_CSG_RAY_LEAF_H
#define VSDK_CSG_RAY_LEAF_H
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayPlacement.h"
#include "java/util/ArrayList.h"
class CsgOperandView;
class Matrix4x4d;
// Raw leaf membership in its parent ray's affine parameter; the path carries
// the difference parity. Boundary normals are oriented for that parity.
class CsgRayLeaf {
private:
    CsgPointWitness path;
    RaySolidClassification classification;
    java::ArrayList<CsgRayPlacement> placements;
public:
    CsgRayLeaf();
    CsgRayLeaf(const CsgRayLeaf& other);
    CsgRayLeaf& operator=(const CsgRayLeaf& other);
    ~CsgRayLeaf();
    // Empties the leaf and keeps its storage for reuse.
    void reset();
    CsgPointWitness& getPath();
    const CsgPointWitness& getPath() const;
    RaySolidClassification& getClassification();
    const RaySolidClassification& getClassification() const;
    bool prependOperand(const CsgOperandView* operand);
    // Parent placements are ordered from the leaf outwards.
    bool appendPlacement(const CsgOperandView* operand, const Matrix4x4d& forward, const Matrix4x4d& inverse);
    long int getPlacementCount() const;
    const CsgRayPlacement* getPlacement(long int index) const;
    void reverseOrientation();
};
#endif
