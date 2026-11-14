#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/AffineToleranceMap.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointCandidate.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWorkspace.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

GeometryContainment CsgPointClassification::classify(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance)
{
    CsgPointWitness ignored;
    return classifyWithWitness(expression,point,tolerance,ignored);
}

GeometryContainment CsgPointClassification::classifyWithWitness(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, CsgPointWitness& witness)
{
    witness.clear();
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance < 0)
        return GeometryContainment::InvalidInput;
    java::ArrayList<const CsgExpression*> path;
    return classifyExpression(expression, point, tolerance, path, &witness, nullptr, nullptr);
}

GeometryContainment CsgPointClassification::classifyWithProof(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, CsgPointProof& proof)
{
    proof.clear();
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    java::ArrayList<const CsgExpression*> path;
    const GeometryContainment result=classifyExpression(expression,point,tolerance,path,nullptr,&proof,nullptr);
    if (proof.isAvailable() && !proof.getVersions().capture(expression)) proof.clear();
    return result;
}

GeometryContainment CsgPointClassification::classifyWithAttribution(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, CsgPointAttribution& attribution)
{
    attribution.clear();
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    java::ArrayList<const CsgExpression*> path;
    const GeometryContainment result=classifyExpression(expression,point,tolerance,path,nullptr,nullptr,&attribution);
    if (attribution.isComplete() && !attribution.getVersions().capture(expression)) attribution.clear();
    return result;
}

GeometryContainment CsgPointClassification::classifyExpression(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, java::ArrayList<const CsgExpression*>& path,
    CsgPointWitness* witness, CsgPointProof* proof, CsgPointAttribution* attribution)
{
    if (path.contains(&expression)) return GeometryContainment::InvalidInput;
    const long int count = expression.getOperandCount();
    const CsgOperation operation = expression.getOperation();
    if (count <= 0 || (operation != CsgOperation::UNION && operation != CsgOperation::INTERSECTION &&
        operation != CsgOperation::DIFFERENCE)) return GeometryContainment::InvalidInput;
    if (!path.add(&expression)) return GeometryContainment::Indeterminate;
    bool hasInside = false, hasOutside = false;
    long int boundaryCount = 0;
    bool hasUnsupported = false, hasIndeterminate = false, hasInvalid = false;
    GeometryContainment first = GeometryContainment::Indeterminate;
    CsgPointWitness firstWitness, decisiveWitness, boundaryWitness;
    CsgPointProof firstProof, decisiveProof, allProof, selfProof;
    CsgPointAttribution firstAttribution, boundaryAttribution;
    allProof.begin(); boundaryAttribution.begin();
    java::ArrayList<GeometryContainment> priorResults;
    bool subtractsSelf=false, hasConflict=false;
    java::ArrayList<CsgPointCandidate> candidates;
    for (long int i = 0; i < count; ++i) {
        const CsgOperandView* operand = expression.getOperand(i);
        GeometryContainment result = GeometryContainment::InvalidInput;
        CsgPointWitness candidate;
        CsgPointProof candidateProof;
        CsgPointAttribution candidateAttribution;
        if (operand) {
            const CsgExpression* child = operand->getChildExpression();
            if (child && (!operand->isStructuralTraversalSafe() || !child->isStructuralTraversalSafe())) {
                result=GeometryContainment::Unsupported;
            }
            else if (child) {
                Matrix4x4d forward, inverse;
                const AffinePlacementStatus placement = operand->queryAffinePlacement(forward, inverse);
                if (placement == AffinePlacementStatus::Invalid) result = GeometryContainment::InvalidInput;
                else if (placement == AffinePlacementStatus::Unsupported) result = GeometryContainment::Unsupported;
                else if (placement != AffinePlacementStatus::Valid) result = GeometryContainment::Indeterminate;
                else {
                    bool identity = true;
                    for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column)
                        if (forward.get(row,column) != (row == column ? 1.0 : 0.0)) identity = false;
                    if (identity) result=classifyExpression(*child,point,tolerance,path,&candidate,&candidateProof,&candidateAttribution);
                    else {
                        Vector3Dd childPoint;
                        double childTolerance=0, childInnerTolerance=-1;
                        const AffinePlacementStatus mapped=AffineToleranceMap::map(
                            forward,point,tolerance,childPoint,childTolerance,&childInnerTolerance);
                        if (mapped==AffinePlacementStatus::Invalid) result=GeometryContainment::InvalidInput;
                        else if (mapped==AffinePlacementStatus::Unsupported) result=GeometryContainment::Unsupported;
                        else if (mapped!=AffinePlacementStatus::Valid) result=GeometryContainment::Indeterminate;
                        else {
                            result=classifyExpression(*child,childPoint,childTolerance,path,&candidate,&candidateProof,&candidateAttribution);
                            // An outer-ball boundary is insufficient. A boundary
                            // within the certified inner ball is also within the
                            // original parent-space tolerance ball.
                            if (result==GeometryContainment::Boundary || result==GeometryContainment::Indeterminate) {
                                candidate.clear();
                                candidateProof.clear();
                                candidateAttribution.clear();
                                if (childInnerTolerance>0) {
                                    CsgPointWitness innerWitness;
                                    CsgPointProof innerProof;
                                    CsgPointAttribution innerAttribution;
                                    const GeometryContainment inner=classifyExpression(*child,childPoint,
                                        childInnerTolerance,path,&innerWitness,&innerProof,&innerAttribution);
                                    if (inner==GeometryContainment::Boundary) {
                                        result=inner;
                                        candidate=innerWitness;
                                        candidateProof=innerProof;
                                        candidateAttribution=innerAttribution;
                                    }
                                    else if (inner==GeometryContainment::InvalidInput ||
                                        inner==GeometryContainment::Unsupported) result=inner;
                                    else result=GeometryContainment::Indeterminate;
                                }
                                else result=GeometryContainment::Indeterminate;
                            }
                        }
                    }
                }
            }
            else {
                // Conservative culling: a solid whose certified bounds miss the
                // tolerance ball is outside; uncertain bounds never cull.
                GeometryBounds bounds;
                operand->getSolidBounds(bounds);
                result = bounds.excludesBall(point,tolerance) ? GeometryContainment::Outside :
                    operand->classifyPointCertified(point,tolerance);
                if (result==GeometryContainment::Inside || result==GeometryContainment::Outside ||
                    result==GeometryContainment::Boundary) {
                    if (candidate.prepend(operand)) {
                        candidateProof.begin();
                        candidateProof.add(CsgPointEvidence(candidate,result,point,tolerance));
                        if (result==GeometryContainment::Boundary) {
                            candidateAttribution.begin(); candidateAttribution.add(candidate);
                        }
                    }
                }
            }
            if (child && candidate.isAvailable() && !candidate.prepend(operand)) candidate.clear();
            if (child && candidateProof.isAvailable() && !candidateProof.prepend(operand)) candidateProof.clear();
            if (child && candidateAttribution.isComplete() && !candidateAttribution.prepend(operand)) candidateAttribution.clear();
        }
        if (result != GeometryContainment::Inside && result != GeometryContainment::Outside &&
            result != GeometryContainment::Boundary && result != GeometryContainment::Unsupported &&
            result != GeometryContainment::Indeterminate && result != GeometryContainment::InvalidInput)
            result = GeometryContainment::InvalidInput;
        if (result!=GeometryContainment::Inside && result!=GeometryContainment::Outside &&
            result!=GeometryContainment::Boundary) { candidate.clear(); candidateProof.clear(); candidateAttribution.clear(); }
        CsgPointCandidate stored;
        stored.result=result; stored.witness=candidate; stored.proof=candidateProof; stored.attribution=candidateAttribution;
        if (!candidates.add(stored)) { path.remove(path.size()-1); return GeometryContainment::Indeterminate; }
    }
    // A certified sibling can supply a classification for an equivalent view.
    // A composite recipient does not inherit leaf attribution: its owner paths
    // must be independently resolved. Invalid or opaque views are never used.
    for (long int i=0;i<count;++i) {
        CsgPointCandidate& target=candidates[i];
        const CsgOperandView* destination=expression.getOperand(i);
        if ((target.result!=GeometryContainment::Unsupported && target.result!=GeometryContainment::Indeterminate) || !destination) continue;
        const CsgExpression* destinationChild=destination->getChildExpression();
        if (destinationChild ? (!destination->isStructuralTraversalSafe() || !destinationChild->isStructuralTraversalSafe()) : !destination->getGeometry()) continue;
        for (long int j=0;j<count;++j) {
            const CsgOperandView* source=expression.getOperand(j);
            const CsgPointCandidate& known=candidates[j];
            if (i==j || !source || (known.result!=GeometryContainment::Inside && known.result!=GeometryContainment::Outside &&
                 known.result!=GeometryContainment::Boundary) || !known.proof.isAvailable() ||
                !destination->isSameSolid(*source) || !source->isSameSolid(*destination)) continue;
            const CsgExpression* sourceChild=source->getChildExpression();
            if (sourceChild ? (!source->isStructuralTraversalSafe() || !sourceChild->isStructuralTraversalSafe()) : !source->getGeometry()) continue;
            CsgPointWitness sourceView,recipient;
            if (!sourceView.prepend(source) || !recipient.prepend(destination)) break;
            target.proof=known.proof;
            if (!target.proof.add(CsgPointEvidence(sourceView,recipient,point,tolerance))) { target.proof.clear(); break; }
            target.result=known.result;
            if (!destinationChild) {
                target.witness=recipient;
                if (target.result==GeometryContainment::Boundary) {
                    target.attribution.begin(); target.attribution.add(recipient);
                }
            }
            break;
        }
    }
    for (long int i=0;i<count;++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        GeometryContainment result=candidates[i].result;
        CsgPointWitness candidate=candidates[i].witness;
        CsgPointProof candidateProof=candidates[i].proof;
        CsgPointAttribution candidateAttribution=candidates[i].attribution;
        if (i == 0) { first = result; firstWitness=candidate; firstProof=candidateProof; firstAttribution=candidateAttribution; }
        bool samePriorBoundary=false, matchesFirst=false;
        for (long int j=0;j<i;++j) {
            const CsgOperandView* earlier=expression.getOperand(j);
            if (operand && earlier && operand->isSameSolid(*earlier) && earlier->isSameSolid(*operand)) {
                if (priorResults[j]!=result) hasConflict=true;
                if (result==GeometryContainment::Boundary && priorResults[j]==result) samePriorBoundary=true;
                if (operation==CsgOperation::DIFFERENCE && j==0 && priorResults[j]==result &&
                    (result==GeometryContainment::Inside || result==GeometryContainment::Outside ||
                     result==GeometryContainment::Boundary)) { subtractsSelf=true; matchesFirst=true; }
            }
        }
        if (!priorResults.add(result)) { path.remove(path.size()-1); return GeometryContainment::Indeterminate; }
        if (operation == CsgOperation::DIFFERENCE && i > 0) {
            if (result == GeometryContainment::Inside) result = GeometryContainment::Outside;
            else if (result == GeometryContainment::Outside) result = GeometryContainment::Inside;
            candidate.reverseOrientation();
            candidateProof.reverseOrientation();
            candidateAttribution.reverseOrientation();
        }
        if (matchesFirst && firstProof.isAvailable() && candidateProof.isAvailable() && !selfProof.isAvailable()) {
            selfProof=firstProof;
            if (!selfProof.append(candidateProof)) selfProof.clear();
        }
        if (!decisiveProof.isAvailable() && candidateProof.isAvailable() &&
            ((operation==CsgOperation::UNION && result==GeometryContainment::Inside) ||
             (operation!=CsgOperation::UNION && result==GeometryContainment::Outside)))
            decisiveProof=candidateProof;
        if (!allProof.append(candidateProof)) allProof.clear();
        if (!decisiveWitness.isAvailable() && candidate.isAvailable() &&
            ((operation==CsgOperation::UNION && result==GeometryContainment::Inside) ||
             (operation!=CsgOperation::UNION && result==GeometryContainment::Outside)))
            decisiveWitness=candidate;
        hasInside |= result == GeometryContainment::Inside;
        hasOutside |= result == GeometryContainment::Outside;
        if (result == GeometryContainment::Boundary && !boundaryAttribution.append(candidateAttribution))
            boundaryAttribution.clear();
        if (result == GeometryContainment::Boundary && !samePriorBoundary) {
            ++boundaryCount;
            if (boundaryCount==1) boundaryWitness=candidate;
        }
        else if (result == GeometryContainment::Boundary && boundaryCount==1 &&
            !boundaryWitness.isAvailable() && candidate.isAvailable()) boundaryWitness=candidate;
        hasUnsupported |= result == GeometryContainment::Unsupported;
        hasIndeterminate |= result == GeometryContainment::Indeterminate;
        hasInvalid |= result == GeometryContainment::InvalidInput;
    }
    path.remove(path.size()-1);
    if (hasInvalid) return GeometryContainment::InvalidInput;
    if (hasConflict) return GeometryContainment::Indeterminate;
    if (subtractsSelf) { if (proof && selfProof.isAvailable()) *proof=selfProof; return GeometryContainment::Outside; }
    if (count == 1) {
        if (witness) *witness=firstWitness;
        if (proof) *proof=firstProof;
        if (attribution && first==GeometryContainment::Boundary) *attribution=firstAttribution;
        return first;
    }
    // A definite result requires a full tolerance ball in or out of the solid.
    // A boundary of one operand need not be a boundary of the composition.
    if (operation == CsgOperation::UNION && hasInside) {
        if (witness) *witness=decisiveWitness;
        if (proof) *proof=decisiveProof;
        return GeometryContainment::Inside;
    }
    if (operation != CsgOperation::UNION && hasOutside) {
        if (witness) *witness=decisiveWitness;
        if (proof) *proof=decisiveProof;
        return GeometryContainment::Outside;
    }
    if (hasUnsupported) return GeometryContainment::Unsupported;
    if (hasIndeterminate) return GeometryContainment::Indeterminate;
    // With exactly one boundary and every other operand certified on the
    // non-interfering side throughout the tolerance ball, that operand's
    // boundary is exposed in the composite. Multiple boundaries can coincide
    // or hide one another, so their combination remains indeterminate.
    if (boundaryCount==1) {
        if (witness) *witness=boundaryWitness;
        if (proof) *proof=allProof;
        if (attribution) *attribution=boundaryAttribution;
        return GeometryContainment::Boundary;
    }
    if (boundaryCount>1) return GeometryContainment::Indeterminate;
    if (proof) *proof=allProof;
    return operation == CsgOperation::UNION ? GeometryContainment::Outside : GeometryContainment::Inside;
}

GeometryContainment CsgPointClassification::classifyContainment(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace)
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    workspace.path.clear();
    workspace.results.clear();
    return containmentOfExpression(expression,point,tolerance,workspace);
}

GeometryContainment CsgPointClassification::classifyOperandContainment(const CsgOperandView& operand,
    const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace)
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    workspace.path.clear();
    workspace.results.clear();
    return containmentOfOperand(&operand,point,tolerance,workspace);
}

void CsgPointClassification::truncate(CsgPointWorkspace& workspace, long int resultCount)
{
    while (workspace.results.size()>resultCount) workspace.results.remove(workspace.results.size()-1);
}

// Mirrors the operand step of classifyExpression.
GeometryContainment CsgPointClassification::containmentOfOperand(const CsgOperandView* operand,
    const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace)
{
    if (!operand) return GeometryContainment::InvalidInput;
    GeometryContainment result=GeometryContainment::InvalidInput;
    const CsgExpression* child=operand->getChildExpression();
    if (child && (!operand->isStructuralTraversalSafe() || !child->isStructuralTraversalSafe()))
        result=GeometryContainment::Unsupported;
    else if (child) {
        Matrix4x4d forward, inverse;
        const AffinePlacementStatus placement=operand->queryAffinePlacement(forward,inverse);
        if (placement==AffinePlacementStatus::Invalid) result=GeometryContainment::InvalidInput;
        else if (placement==AffinePlacementStatus::Unsupported) result=GeometryContainment::Unsupported;
        else if (placement!=AffinePlacementStatus::Valid) result=GeometryContainment::Indeterminate;
        else {
            bool identity=true;
            for (int row=0;row<4;++row) for (int column=0;column<4;++column)
                if (forward.get(row,column)!=(row==column ? 1.0 : 0.0)) identity=false;
            if (identity) result=containmentOfExpression(*child,point,tolerance,workspace);
            else {
                Vector3Dd childPoint;
                double childTolerance=0, childInnerTolerance=-1;
                const AffinePlacementStatus mapped=AffineToleranceMap::map(
                    forward,point,tolerance,childPoint,childTolerance,&childInnerTolerance);
                if (mapped==AffinePlacementStatus::Invalid) result=GeometryContainment::InvalidInput;
                else if (mapped==AffinePlacementStatus::Unsupported) result=GeometryContainment::Unsupported;
                else if (mapped!=AffinePlacementStatus::Valid) result=GeometryContainment::Indeterminate;
                else {
                    result=containmentOfExpression(*child,childPoint,childTolerance,workspace);
                    // Only a boundary within the certified inner ball is a
                    // boundary within the parent-space tolerance ball.
                    if (result==GeometryContainment::Boundary || result==GeometryContainment::Indeterminate) {
                        if (childInnerTolerance>0) {
                            const GeometryContainment inner=containmentOfExpression(*child,childPoint,
                                childInnerTolerance,workspace);
                            if (inner==GeometryContainment::Boundary || inner==GeometryContainment::InvalidInput ||
                                inner==GeometryContainment::Unsupported) result=inner;
                            else result=GeometryContainment::Indeterminate;
                        }
                        else result=GeometryContainment::Indeterminate;
                    }
                }
            }
        }
    }
    else {
        GeometryBounds bounds;
        operand->getSolidBounds(bounds);
        result=bounds.excludesBall(point,tolerance) ? GeometryContainment::Outside :
            operand->classifyPointCertified(point,tolerance);
    }
    if (result!=GeometryContainment::Inside && result!=GeometryContainment::Outside &&
        result!=GeometryContainment::Boundary && result!=GeometryContainment::Unsupported &&
        result!=GeometryContainment::Indeterminate && result!=GeometryContainment::InvalidInput)
        result=GeometryContainment::InvalidInput;
    return result;
}

// Mirrors classifyExpression; a definite operand result is the condition under
// which classifyExpression has a proof for it.
GeometryContainment CsgPointClassification::containmentOfExpression(const CsgExpression& expression,
    const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace)
{
    if (workspace.path.contains(&expression)) return GeometryContainment::InvalidInput;
    const long int count=expression.getOperandCount();
    const CsgOperation operation=expression.getOperation();
    if (count<=0 || (operation!=CsgOperation::UNION && operation!=CsgOperation::INTERSECTION &&
        operation!=CsgOperation::DIFFERENCE)) return GeometryContainment::InvalidInput;
    if (!workspace.path.add(&expression)) return GeometryContainment::Indeterminate;
    const long int base=workspace.results.size();
    for (long int i=0;i<count;++i) {
        const GeometryContainment result=containmentOfOperand(expression.getOperand(i),point,tolerance,workspace);
        if (!workspace.results.add(result)) {
            truncate(workspace,base);
            workspace.path.remove(workspace.path.size()-1);
            return GeometryContainment::Indeterminate;
        }
    }
    java::ArrayList<GeometryContainment>& results=workspace.results;
    for (long int i=0;i<count;++i) {
        const GeometryContainment target=results[base+i];
        const CsgOperandView* destination=expression.getOperand(i);
        if ((target!=GeometryContainment::Unsupported && target!=GeometryContainment::Indeterminate) || !destination) continue;
        const CsgExpression* destinationChild=destination->getChildExpression();
        if (destinationChild ? (!destination->isStructuralTraversalSafe() || !destinationChild->isStructuralTraversalSafe()) : !destination->getGeometry()) continue;
        for (long int j=0;j<count;++j) {
            const CsgOperandView* source=expression.getOperand(j);
            const GeometryContainment known=results[base+j];
            if (i==j || !source || (known!=GeometryContainment::Inside && known!=GeometryContainment::Outside &&
                 known!=GeometryContainment::Boundary) ||
                !destination->isSameSolid(*source) || !source->isSameSolid(*destination)) continue;
            const CsgExpression* sourceChild=source->getChildExpression();
            if (sourceChild ? (!source->isStructuralTraversalSafe() || !sourceChild->isStructuralTraversalSafe()) : !source->getGeometry()) continue;
            results[base+i]=known;
            break;
        }
    }
    bool hasInside=false, hasOutside=false, hasUnsupported=false, hasIndeterminate=false, hasInvalid=false;
    bool subtractsSelf=false, hasConflict=false;
    long int boundaryCount=0;
    const GeometryContainment first=results[base];
    for (long int i=0;i<count;++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        GeometryContainment result=results[base+i];
        bool samePriorBoundary=false;
        for (long int j=0;j<i;++j) {
            const CsgOperandView* earlier=expression.getOperand(j);
            const GeometryContainment prior=results[base+j];
            if (operand && earlier && operand->isSameSolid(*earlier) && earlier->isSameSolid(*operand)) {
                if (prior!=result) hasConflict=true;
                if (result==GeometryContainment::Boundary && prior==result) samePriorBoundary=true;
                if (operation==CsgOperation::DIFFERENCE && j==0 && prior==result &&
                    (result==GeometryContainment::Inside || result==GeometryContainment::Outside ||
                     result==GeometryContainment::Boundary)) subtractsSelf=true;
            }
        }
        if (operation==CsgOperation::DIFFERENCE && i>0) {
            if (result==GeometryContainment::Inside) result=GeometryContainment::Outside;
            else if (result==GeometryContainment::Outside) result=GeometryContainment::Inside;
        }
        hasInside |= result==GeometryContainment::Inside;
        hasOutside |= result==GeometryContainment::Outside;
        if (result==GeometryContainment::Boundary && !samePriorBoundary) ++boundaryCount;
        hasUnsupported |= result==GeometryContainment::Unsupported;
        hasIndeterminate |= result==GeometryContainment::Indeterminate;
        hasInvalid |= result==GeometryContainment::InvalidInput;
    }
    truncate(workspace,base);
    workspace.path.remove(workspace.path.size()-1);
    if (hasInvalid) return GeometryContainment::InvalidInput;
    if (hasConflict) return GeometryContainment::Indeterminate;
    if (subtractsSelf) return GeometryContainment::Outside;
    if (count==1) return first;
    if (operation==CsgOperation::UNION && hasInside) return GeometryContainment::Inside;
    if (operation!=CsgOperation::UNION && hasOutside) return GeometryContainment::Outside;
    if (hasUnsupported) return GeometryContainment::Unsupported;
    if (hasIndeterminate) return GeometryContainment::Indeterminate;
    if (boundaryCount==1) return GeometryContainment::Boundary;
    if (boundaryCount>1) return GeometryContainment::Indeterminate;
    return operation==CsgOperation::UNION ? GeometryContainment::Outside : GeometryContainment::Inside;
}
