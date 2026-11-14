#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "java/util/ArrayList.txx"
#include "java/lang/IllegalStateException.h"
#include "environment/scene/constructiveSolidGeometry/CsgMorganRulesEvaluator.h"
#include "environment/scene/constructiveSolidGeometry/CsgRaySegmentEvaluator.h"

ConstructiveSolidGeometry::ConstructiveSolidGeometry(CsgOperation initialGeometryType, CsgBodyEvaluator evaluator) :
    structure(initialGeometryType),
    evaluator(evaluator)
{
}

// The POV-Ray 1.0 copy appends the operands in reverse order; the Roth copy
// keeps their order.
ConstructiveSolidGeometry::ConstructiveSolidGeometry(const ConstructiveSolidGeometry &other) :
    CsgExpression(),
    structure(other.getGeometryType()),
    evaluator(other.evaluator),
    topLevel(other.topLevel)
{
    const long int count = other.operands.size();
    for (long int k = 0; k < count; k++) {
        const long int i = evaluator == CsgBodyEvaluator::ROTH ? k : count - 1 - k;
        addOperand(static_cast<CsgOperand *>(other.operands[i]->copy()));
    }
}

ConstructiveSolidGeometry *
ConstructiveSolidGeometry::copy() const
{
    return new ConstructiveSolidGeometry(*this);
}

int
ConstructiveSolidGeometry::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (evaluator == CsgBodyEvaluator::ROTH) {
        return CsgRaySegmentEvaluator::allRayCrossings(
            getGeometryType(), operands, topLevel, ray, depthQueue, materialOverride);
    }
    return CsgMorganRulesEvaluator::allRayCrossings(
        getGeometryType(), operands, ray, depthQueue, materialOverride);
}

int
ConstructiveSolidGeometry::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    bool inside;
    if (evaluator == CsgBodyEvaluator::ROTH) {
        inside = CsgRaySegmentEvaluator::contains(getGeometryType(), operands, point, distanceTolerance);
    } else {
        inside = CsgMorganRulesEvaluator::contains(getGeometryType(), operands, point);
    }
    return inside ? RayCastingContainment::INSIDE : RayCastingContainment::OUTSIDE;
}

void
ConstructiveSolidGeometry::applyAnnotatedEmissionContext(
    IntersectionCandidate &candidate,
    const GeometryIntersectionEmissionContext &context)
{
    IntersectionAttributes &attributes = candidate.getAttributes();
    attributes.pushDetailOwner(context.getDetailOwner());
    attributes.setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
}

int
ConstructiveSolidGeometry::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    if (depthQueue->size() == 0) {
        const int found =
            doIntersectionForAllRayCrossings(ray, depthQueue, context.getMaterialOverride());
        if (!found || depthQueue->size() == 0) {
            return found;
        }
        for (IntersectionCandidate &candidate : *depthQueue) {
            applyAnnotatedEmissionContext(candidate, context);
        }
        return found;
    }

    java::PriorityQueue<IntersectionCandidate> * const freshQueue =
        ray->getIntersectionQueuePool()->pop(128);
    const int found =
        doIntersectionForAllRayCrossings(ray, freshQueue, context.getMaterialOverride());
    if (found && freshQueue->size() > 0) {
        for (IntersectionCandidate &candidate : *freshQueue) {
            applyAnnotatedEmissionContext(candidate, context);
            depthQueue->offer(candidate);
        }
    }
    ray->getIntersectionQueuePool()->push(freshQueue);
    return found;
}

void
ConstructiveSolidGeometry::invert()
{
    if (evaluator == CsgBodyEvaluator::ROTH) {
        setGeometryType(CsgRaySegmentEvaluator::invert(getGeometryType(), operands));
    } else {
        setGeometryType(CsgMorganRulesEvaluator::invert(getGeometryType(), operands));
    }
}

ConstructiveSolidGeometry::~ConstructiveSolidGeometry()
{
    for (long int i = 0; i < views.size(); i++) {
        delete views[i];
    }
    for (long int i = 0; i < operands.size(); i++) {
        delete operands[i];
    }
}

void
ConstructiveSolidGeometry::addShape(
    Geometry *shape,
    Material *material,
    Matrix4x4d *transformation,
    Matrix4x4d *transformationInverse,
    const java::ArrayList<TransformStep> &steps)
{
    addOperand(new CsgOperand(shape, material, transformation, transformationInverse, steps));
}

void
ConstructiveSolidGeometry::addNested(
    ConstructiveSolidGeometry *child,
    Material *material,
    Matrix4x4d *transformation,
    Matrix4x4d *transformationInverse,
    const java::ArrayList<TransformStep> &steps)
{
    CsgOperand *operand = new CsgOperand(nullptr, material, transformation, transformationInverse, steps);
    operand->setCsg(child);
    addOperand(operand);
}

void
ConstructiveSolidGeometry::addOperand(CsgOperand *operand)
{
    CsgOperandBodyView *view = new CsgOperandBodyView(*operand);
    if (!structure.addOperand(*view)) {
        delete view;
        delete operand;
        throw java::IllegalStateException("CSG operand would create a cycle");
    }
    operands.add(operand);
    views.add(view);
}

void
ConstructiveSolidGeometry::translate(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->translate(vector);
    }
}

void
ConstructiveSolidGeometry::rotate(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->rotate(vector);
    }
}

void
ConstructiveSolidGeometry::scale(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->scale(vector);
    }
}


void ConstructiveSolidGeometry::getBounds(GeometryBounds& result) const
{
    if (operands.size() == 0) { result.setUnsupported(); return; }
    result.setEmpty();
    for (long int i = 0; i < operands.size(); ++i) {
        const CsgOperand* operand = operands[i];
        if (operand == nullptr || !operand->hasShape()) {
            result.setIndeterminate(); return;
        }
        GeometryBounds child;
        operand->getSurfaceBounds(child);
        if (operand->getTransformation() != nullptr)
            child = child.transformed(operand->getTransformation()->transpose());
        result = result.enclosing(child);
    }
}

BoundingVolumeHierarchy *
ConstructiveSolidGeometry::createBoundingVolume() const
{
    AxisAlignedBoundingBox result = AxisAlignedBoundingBox::empty();
    for (long int i = 0; i < operands.size(); i++) {
        BoundingVolumeHierarchy *operandBounds = operands[i]->createBoundingVolume();
        result = result.enclosing(operandBounds->axisAlignedExtent());
        delete operandBounds;
    }
    return new AabbBoundingVolume(result);
}

