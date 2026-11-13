#include <typeinfo>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpressionEquivalence.h"
bool CsgExpressionEquivalence::same(const CsgOperandView& a, const CsgOperandView& b)
{
    java::ArrayList<const CsgExpression*> pathA,pathB;
    return sameView(a,b,pathA,pathB);
}
bool CsgExpressionEquivalence::sameView(const CsgOperandView& a, const CsgOperandView& b,
    java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB)
{
    const CsgExpression* childA=a.getChildExpression();
    const CsgExpression* childB=b.getChildExpression();
    if (!childA || !childB) return !childA && !childB && a.isSameSolid(b) && b.isSameSolid(a);
    // Arbitrary application views may add inversion, clipping or other
    // semantics not represented by their child and placement. Only the known
    // shared structural views can be compared by unpacking their children.
    const bool structuralA=typeid(a)==typeid(CsgTree) || typeid(a)==typeid(CsgPlacedExpression);
    const bool structuralB=typeid(b)==typeid(CsgTree) || typeid(b)==typeid(CsgPlacedExpression);
    if (!structuralA || !structuralB) return false;
    Matrix4x4d forwardA,inverseA,forwardB,inverseB;
    if (a.queryAffinePlacement(forwardA,inverseA)!=AffinePlacementStatus::Valid ||
        b.queryAffinePlacement(forwardB,inverseB)!=AffinePlacementStatus::Valid) return false;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        if (forwardA.get(i,j)!=forwardB.get(i,j)) return false;
    return sameExpression(*childA,*childB,pathA,pathB);
}
bool CsgExpressionEquivalence::collectOperand(const CsgOperandView& view, CsgOperation operation,
    java::ArrayList<const CsgExpression*>& path, java::ArrayList<const CsgOperandView*>& output)
{
    const CsgExpression* child=view.getChildExpression();
    const bool knownView=typeid(view)==typeid(CsgTree) || typeid(view)==typeid(CsgPlacedExpression);
    if (!child || !knownView || typeid(*child)!=typeid(CsgTree) || child->getOperation()!=operation)
        return output.add(&view);
    Matrix4x4d forward,inverse;
    if (view.queryAffinePlacement(forward,inverse)!=AffinePlacementStatus::Valid) return false;
    bool identity=true;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        if (forward.get(i,j)!=(i==j ? 1.0 : 0.0)) identity=false;
    if (!identity) return output.add(&view);
    if (path.contains(child) || child->getOperandCount()<=0 || !path.add(child)) return false;
    bool valid=true;
    for (long int i=0;i<child->getOperandCount() && valid;++i) {
        const CsgOperandView* item=child->getOperand(i);
        valid=item && collectOperand(*item,operation,path,output);
    }
    path.remove(path.size()-1);
    return valid;
}
bool CsgExpressionEquivalence::sameSets(const java::ArrayList<const CsgOperandView*>& a,
    const java::ArrayList<const CsgOperandView*>& b,
    java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB)
{
    // Union and intersection are idempotent as solid-set operations. Check
    // both directions without consuming matches so duplicates are harmless.
    for (int direction=0;direction<2;++direction) {
        const java::ArrayList<const CsgOperandView*>& from=direction==0 ? a : b;
        const java::ArrayList<const CsgOperandView*>& to=direction==0 ? b : a;
        for (long int i=0;i<from.size();++i) {
            bool found=false;
            for (long int j=0;j<to.size() && !found;++j)
                found=direction==0 ? sameView(*from[i],*to[j],pathA,pathB) :
                    sameView(*to[j],*from[i],pathA,pathB);
            if (!found) return false;
        }
    }
    return true;
}
bool CsgExpressionEquivalence::sameExpression(const CsgExpression& a, const CsgExpression& b,
    java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB)
{
    if (pathA.contains(&a) || pathB.contains(&b)) return false;
    if (typeid(a)!=typeid(CsgTree) || typeid(b)!=typeid(CsgTree)) return false;
    const CsgOperation op=a.getOperation();
    if (a.getOperandCount()<=0 || b.getOperandCount()<=0 || op!=b.getOperation() ||
        (op!=CsgOperation::UNION && op!=CsgOperation::INTERSECTION && op!=CsgOperation::DIFFERENCE)) return false;
    if (!pathA.add(&a)) return false;
    if (!pathB.add(&b)) { pathA.remove(pathA.size()-1); return false; }
    bool equal=true;
    long int start=0;
    if (op==CsgOperation::DIFFERENCE) {
        const CsgOperandView* firstA=a.getOperand(0); const CsgOperandView* firstB=b.getOperand(0);
        equal=firstA && firstB && sameView(*firstA,*firstB,pathA,pathB);
        start=1;
    }
    java::ArrayList<const CsgOperandView*> itemsA,itemsB;
    const CsgOperation associative=op==CsgOperation::DIFFERENCE ? CsgOperation::UNION : op;
    for (long int i=start;equal && i<a.getOperandCount();++i) {
        const CsgOperandView* item=a.getOperand(i);
        equal=item && collectOperand(*item,associative,pathA,itemsA);
    }
    for (long int i=start;equal && i<b.getOperandCount();++i) {
        const CsgOperandView* item=b.getOperand(i);
        equal=item && collectOperand(*item,associative,pathB,itemsB);
    }
    if (equal) equal=sameSets(itemsA,itemsB,pathA,pathB);
    pathA.remove(pathA.size()-1); pathB.remove(pathB.size()-1);
    return equal;
}
