#include <typeinfo>
#include "java/lang/Double.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactPlane.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactQuadric.h"
#include "vsdk/toolkit/environment/scene/csg/CsgDyadicSolid.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSolidEquivalence.h"

bool CsgSolidEquivalence::sameEllipsoidLinear(const Matrix4x4d& a, double radiusA,
    const Matrix4x4d& b, double radiusB)
{
    double x[3][3], y[3][3];
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        x[i][j]=a.get(i,j)*radiusA; y[i][j]=b.get(i,j)*radiusB;
        if (!java::Double::isFinite(x[i][j]) || !java::Double::isFinite(y[i][j]) ||
            !CsgExactArithmetic::equalProducts(a.get(i,j),radiusA,x[i][j],1) ||
            !CsgExactArithmetic::equalProducts(b.get(i,j),radiusB,y[i][j],1))
            return CsgDyadicSolid::sameEllipsoids(radiusA,a,radiusB,b);
    }
    // Equal Gram matrices describe the same affine image of a sphere.
    for (int i=0;i<3;++i) for (int j=i;j<3;++j)
        if (!CsgExactArithmetic::equalSumProducts(x[i],x[j],y[i],y[j],3))
            return CsgDyadicSolid::sameEllipsoids(radiusA,a,radiusB,b);
    return true;
}

bool CsgSolidEquivalence::sameTranslation(const Matrix4x4d& a, const Matrix4x4d& b)
{ for (int i=0;i<3;++i) if (a.get(i,3)!=b.get(i,3)) return false; return true; }

bool CsgSolidEquivalence::signedPermutation(const Matrix4x4d& a, const Matrix4x4d& b,
    int* axes, int* signs)
{
    if (!sameTranslation(a,b)) return false;
    bool used[3]={false,false,false};
    for (int j=0;j<3;++j) {
        bool found=false;
        for (int k=0;k<3 && !found;++k) if (!used[k]) {
            bool same=true, reverse=true;
            for (int i=0;i<3;++i) {
                if (a.get(i,k)!=b.get(i,j)) same=false;
                if (a.get(i,k)!=-b.get(i,j)) reverse=false;
            }
            if (same || reverse) { axes[j]=k; signs[j]=same?1:-1; used[k]=true; found=true; }
        }
        if (!found) return false;
    }
    return true;
}

bool CsgSolidEquivalence::proportional(const double* a, const double* b, int count)
{
    int pivot=-1;
    for (int i=0;i<count;++i) {
        if (!java::Double::isFinite(a[i]) || !java::Double::isFinite(b[i])) return false;
        if (pivot<0 && (a[i]!=0 || b[i]!=0)) pivot=i;
    }
    if (pivot<0 || a[pivot]==0 || b[pivot]==0 || (a[pivot]<0)!=(b[pivot]<0)) return false;
    for (int i=0;i<count;++i)
        if (!CsgExactArithmetic::equalProducts(a[i],b[pivot],b[i],a[pivot])) return false;
    return true;
}

bool CsgSolidEquivalence::sameBox(const Box& a, const Matrix4x4d& pa, const Box& b, const Matrix4x4d& pb)
{
    const Vector3Dd* ab=a.getRawBounds(); const Vector3Dd* bb=b.getRawBounds();
    const double amin[3]={ab[0].x(),ab[0].y(),ab[0].z()}, amax[3]={ab[1].x(),ab[1].y(),ab[1].z()};
    const double bmin[3]={bb[0].x(),bb[0].y(),bb[0].z()}, bmax[3]={bb[1].x(),bb[1].y(),bb[1].z()};
    for (int i=0;i<3;++i)
        if (!java::Double::isFinite(amin[i]) || !java::Double::isFinite(amax[i]) ||
            !java::Double::isFinite(bmin[i]) || !java::Double::isFinite(bmax[i]) ||
            !(amin[i]<amax[i]) || !(bmin[i]<bmax[i])) return false;
    // Axis-aligned world boxes: compare exact dyadic affine endpoints, including
    // translations, scales, reflections and permutations without rounded sums.
    int ai[3],bi[3]; bool usedA[3]={false,false,false}, usedB[3]={false,false,false};
    bool axisAligned=true;
    for (int i=0;i<3;++i) {
        ai[i]=-1; bi[i]=-1;
        for (int j=0;j<3;++j) {
            if (pa.get(i,j)!=0) { if (ai[i]>=0) axisAligned=false; ai[i]=j; }
            if (pb.get(i,j)!=0) { if (bi[i]>=0) axisAligned=false; bi[i]=j; }
        }
        if (ai[i]<0 || bi[i]<0) axisAligned=false;
        else { if (usedA[ai[i]] || usedB[bi[i]]) axisAligned=false; usedA[ai[i]]=true; usedB[bi[i]]=true; }
    }
    if (axisAligned) {
        for (int i=0;i<3;++i) {
            const double sa=pa.get(i,ai[i]), sb=pb.get(i,bi[i]);
            if (!CsgExactArithmetic::equalAffine(sa,sa>0?amin[ai[i]]:amax[ai[i]],pa.get(i,3),
                sb,sb>0?bmin[bi[i]]:bmax[bi[i]],pb.get(i,3)) ||
                !CsgExactArithmetic::equalAffine(sa,sa>0?amax[ai[i]]:amin[ai[i]],pa.get(i,3),
                sb,sb>0?bmax[bi[i]]:bmin[bi[i]],pb.get(i,3))) return false;
        }
        return true;
    }
    // For general affine boxes, match parallel edge directions and compare
    // their exact edge vectors and a corresponding world-space corner.
    bool used[3]={false,false,false};
    bool reversed[3]={false,false,false};
    for (int j=0;j<3;++j) {
        bool found=false;
        for (int k=0;k<3 && !found;++k) if (!used[k]) {
            int pivot=-1;
            for (int i=0;i<3;++i) if (pa.get(i,k)!=0) { pivot=i; break; }
            if (pivot<0 || pb.get(pivot,j)==0) continue;
            bool same=true;
            for (int i=0;i<3;++i)
                if (!CsgExactArithmetic::equalProducts(pa.get(i,k),pb.get(pivot,j),pb.get(i,j),pa.get(pivot,k))) same=false;
            const bool reverse=(pa.get(pivot,k)<0)!=(pb.get(pivot,j)<0);
            for (int i=0;i<3 && same;++i) {
                const double ac[2]={pa.get(i,k),-pa.get(i,k)};
                const double av[2]={amax[k],amin[k]};
                const double bc[2]={pb.get(i,j),-pb.get(i,j)};
                const double bv[2]={reverse?bmin[j]:bmax[j],reverse?bmax[j]:bmin[j]};
                if (!CsgExactArithmetic::equalSumProducts(ac,av,bc,bv,2)) same=false;
            }
            if (same) { used[k]=true; reversed[j]=reverse; found=true; }
        }
        if (!found) return false;
    }
    for (int i=0;i<3;++i) {
        const double ac[4]={pa.get(i,0),pa.get(i,1),pa.get(i,2),pa.get(i,3)};
        const double av[4]={amin[0],amin[1],amin[2],1};
        const double bc[4]={pb.get(i,0),pb.get(i,1),pb.get(i,2),pb.get(i,3)};
        const double bv[4]={reversed[0]?bmax[0]:bmin[0],reversed[1]?bmax[1]:bmin[1],
            reversed[2]?bmax[2]:bmin[2],1};
        if (!CsgExactArithmetic::equalSumProducts(ac,av,bc,bv,4)) return false;
    }
    return true;
}

bool CsgSolidEquivalence::axisPlane(const double* coefficients, const Matrix4x4d& placement,
    int& axis, double& scale, double& offset, double& root, bool& positive)
{
    int local=-1;
    for (int i=0;i<4;++i) if (!java::Double::isFinite(coefficients[i])) return false;
    for (int i=0;i<3;++i) if (coefficients[i]!=0) { if (local>=0) return false; local=i; }
    if (local<0) return false;
    axis=-1;
    for (int row=0;row<3;++row) {
        if (placement.get(row,local)==0) continue;
        bool single=true;
        for (int column=0;column<3;++column)
            if (column!=local && placement.get(row,column)!=0) single=false;
        if (single) { if (axis>=0) return false; axis=row; }
    }
    if (axis<0) return false;
    const double candidate=-coefficients[3]/coefficients[local];
    if (!java::Double::isFinite(candidate) ||
        !CsgExactArithmetic::equalProducts(candidate,coefficients[local],-coefficients[3],1)) return false;
    scale=placement.get(axis,local); offset=placement.get(axis,3); root=candidate;
    positive=(coefficients[local]<0)==(scale<0);
    return true;
}

bool CsgSolidEquivalence::sameOrientedPlane(const double* a, const Matrix4x4d& pa,
    const double* b, const Matrix4x4d& pb)
{
    int axisA=-1,axisB=-1; double scaleA=0,scaleB=0,offsetA=0,offsetB=0,rootA=0,rootB=0;
    bool positiveA=false,positiveB=false;
    if (axisPlane(a,pa,axisA,scaleA,offsetA,rootA,positiveA) &&
        axisPlane(b,pb,axisB,scaleB,offsetB,rootB,positiveB))
        return axisA==axisB && positiveA==positiveB &&
            CsgExactArithmetic::equalAffine(scaleA,rootA,offsetA,scaleB,rootB,offsetB);
    double worldA[4],worldB[4];
    if (CsgExactPlane::world(a,pa,worldA) && CsgExactPlane::world(b,pb,worldB))
        return proportional(worldA,worldB,4);
    int axes[3],signs[3];
    if (!signedPermutation(pa,pb,axes,signs)) return CsgDyadicSolid::samePlanes(a,pa,b,pb);
    const double transformed[4]={signs[0]*a[axes[0]],signs[1]*a[axes[1]],signs[2]*a[axes[2]],a[3]};
    return proportional(transformed,b,4);
}

bool CsgSolidEquivalence::sameHalfSpace(const PlanarHalfSpace& a, const Matrix4x4d& pa,
    const PlanarHalfSpace& b, const Matrix4x4d& pb)
{
    const InfinitePlane& x=a.getBoundaryPlane(); const InfinitePlane& y=b.getBoundaryPlane();
    const Vector3Dd nx=x.getCoefficients(), ny=y.getCoefficients();
    const double av[4]={nx.x(),nx.y(),nx.z(),x.getConstantCoefficient()};
    const double bv[4]={ny.x(),ny.y(),ny.z(),y.getConstantCoefficient()};
    if ((av[0]==0 && av[1]==0 && av[2]==0) || (bv[0]==0 && bv[1]==0 && bv[2]==0)) return false;
    return sameOrientedPlane(av,pa,bv,pb);
}

bool CsgSolidEquivalence::sphereQuadric(const Sphere& sphere, const Matrix4x4d& ps,
    const QuadricSurface& quadric, const Matrix4x4d& pq)
{
    const double sphereRadius=sphere.getRadius();
    if (!java::Double::isFinite(sphereRadius) || !(sphereRadius>0)) return false;
    const double squared=sphereRadius*sphereRadius;
    if (java::Double::isFinite(squared) && CsgExactArithmetic::equalProducts(sphereRadius,sphereRadius,squared,1)) {
        const double local[10]={1,1,1,0,0,0,0,0,0,-squared};
        double worldA[10],worldB[10];
        if (CsgExactQuadric::world(local,ps,worldA) && worldQuadric(quadric,pq,worldB) &&
            proportional(worldA,worldB,10)) return true;
    }
    const Vector3Dd sq=quadric.squareCoefficients(), mixed=quadric.mixedCoefficients(), lin=quadric.linearCoefficients();
    const double coefficients[10]={sq.x(),sq.y(),sq.z(),mixed.x(),mixed.y(),mixed.z(),
        lin.x(),lin.y(),lin.z(),quadric.constantCoefficient()};
    if (CsgDyadicSolid::sphereQuadric(sphereRadius,ps,coefficients,pq)) return true;
    int axes[3],signs[3];
    if (!sameTranslation(ps,pq) ||
        (!signedPermutation(ps,pq,axes,signs) && !sameEllipsoidLinear(ps,1,pq,1))) return false;
    const double q=sq.x(), radius=sphere.getRadius(), constant=quadric.constantCoefficient();
    if (!java::Double::isFinite(q) || !java::Double::isFinite(radius) || !java::Double::isFinite(constant) ||
        !(q>0) || !(radius>0) || sq.y()!=q || sq.z()!=q || mixed.x()!=0 || mixed.y()!=0 || mixed.z()!=0 ||
        lin.x()!=0 || lin.y()!=0 || lin.z()!=0) return false;
    // The intermediate must itself be exact; otherwise conservatively decline.
    const double intermediate=q*radius;
    if (!java::Double::isFinite(intermediate) || !CsgExactArithmetic::equalProducts(q,radius,intermediate,1)) return false;
    return CsgExactArithmetic::equalProducts(intermediate,radius,-constant,1);
}

bool CsgSolidEquivalence::halfSpaceQuadric(const PlanarHalfSpace& plane, const Matrix4x4d& pp,
    const QuadricSurface& quadric, const Matrix4x4d& pq)
{
    const Vector3Dd sq=quadric.squareCoefficients(), mixed=quadric.mixedCoefficients(), lin=quadric.linearCoefficients();
    if (sq.x()!=0 || sq.y()!=0 || sq.z()!=0 || mixed.x()!=0 || mixed.y()!=0 || mixed.z()!=0) return false;
    const InfinitePlane& boundary=plane.getBoundaryPlane();
    const Vector3Dd n=boundary.getCoefficients();
    if ((n.x()==0 && n.y()==0 && n.z()==0) || (lin.x()==0 && lin.y()==0 && lin.z()==0)) return false;
    const double a[4]={n.x(),n.y(),n.z(),boundary.getConstantCoefficient()};
    const double b[4]={lin.x(),lin.y(),lin.z(),quadric.constantCoefficient()};
    return sameOrientedPlane(a,pp,b,pq);
}

bool CsgSolidEquivalence::worldQuadric(const QuadricSurface& quadric, const Matrix4x4d& placement, double* result)
{
    const Vector3Dd sq=quadric.squareCoefficients(), mixed=quadric.mixedCoefficients(), lin=quadric.linearCoefficients();
    const double local[10]={sq.x(),sq.y(),sq.z(),mixed.x(),mixed.y(),mixed.z(),lin.x(),lin.y(),lin.z(),quadric.constantCoefficient()};
    return CsgExactQuadric::world(local,placement,result);
}

bool CsgSolidEquivalence::sameQuadric(const QuadricSurface& a, const Matrix4x4d& pa,
    const QuadricSurface& b, const Matrix4x4d& pb)
{
    double worldA[10],worldB[10];
    if (worldQuadric(a,pa,worldA) && worldQuadric(b,pb,worldB)) return proportional(worldA,worldB,10);
    const Vector3Dd as=a.squareCoefficients(), am=a.mixedCoefficients(), al=a.linearCoefficients();
    const Vector3Dd bs=b.squareCoefficients(), bm=b.mixedCoefficients(), bl=b.linearCoefficients();
    const double localA[10]={as.x(),as.y(),as.z(),am.x(),am.y(),am.z(),al.x(),al.y(),al.z(),a.constantCoefficient()};
    const double localB[10]={bs.x(),bs.y(),bs.z(),bm.x(),bm.y(),bm.z(),bl.x(),bl.y(),bl.z(),b.constantCoefficient()};
    if (CsgDyadicSolid::sameQuadrics(localA,pa,localB,pb)) return true;
    int axes[3],signs[3]; if (!signedPermutation(pa,pb,axes,signs)) return false;
    const double sq[3]={as.x(),as.y(),as.z()}, lin[3]={al.x(),al.y(),al.z()};
    const double mixed[3][3]={{0,am.x(),am.y()},{am.x(),0,am.z()},{am.y(),am.z(),0}};
    const double av[10]={sq[axes[0]],sq[axes[1]],sq[axes[2]],
        signs[0]*signs[1]*mixed[axes[0]][axes[1]],signs[0]*signs[2]*mixed[axes[0]][axes[2]],
        signs[1]*signs[2]*mixed[axes[1]][axes[2]],signs[0]*lin[axes[0]],signs[1]*lin[axes[1]],
        signs[2]*lin[axes[2]],a.constantCoefficient()};
    const double bv[10]={bs.x(),bs.y(),bs.z(),bm.x(),bm.y(),bm.z(),bl.x(),bl.y(),bl.z(),b.constantCoefficient()};
    return proportional(av,bv,10);
}

bool CsgSolidEquivalence::same(const SceneSolid& a, const SceneSolid& b)
{
    if (&a==&b) return true;
    // Each scene specialization explicitly exposes its unmodified core solid.
    const Geometry* ga=a.getSolidEquivalenceGeometry();
    const Geometry* gb=b.getSolidEquivalenceGeometry();
    if (!ga || !gb) return false;
    Matrix4x4d pa,ia,pb,ib;
    if (a.queryAffinePlacement(pa,ia)!=AffinePlacementStatus::Valid ||
        b.queryAffinePlacement(pb,ib)!=AffinePlacementStatus::Valid) return false;
    const Sphere* sa=dynamic_cast<const Sphere*>(ga); const Sphere* sb=dynamic_cast<const Sphere*>(gb);
    if (sa && sb && typeid(*sa)==typeid(Sphere) && typeid(*sb)==typeid(Sphere)) {
        const double ra=sa->getRadius(), rb=sb->getRadius();
        if (!java::Double::isFinite(ra) || !java::Double::isFinite(rb) || ra<=0 || rb<=0 || !sameTranslation(pa,pb)) return false;
        bool used[3]={false,false,false};
        for (int j=0;j<3;++j) {
            bool found=false;
            for (int k=0;k<3 && !found;++k) if (!used[k]) {
                bool same=true, reverse=true;
                for (int i=0;i<3;++i) {
                    if (!CsgExactArithmetic::equalProducts(pa.get(i,k),ra,pb.get(i,j),rb)) same=false;
                    if (!CsgExactArithmetic::equalProducts(pa.get(i,k),ra,-pb.get(i,j),rb)) reverse=false;
                }
                if (same || reverse) { used[k]=true; found=true; }
            }
            if (!found) return sameEllipsoidLinear(pa,ra,pb,rb);
        }
        return true;
    }
    const Box* ba=dynamic_cast<const Box*>(ga); const Box* bb=dynamic_cast<const Box*>(gb);
    if (ba && bb && typeid(*ba)==typeid(Box) && typeid(*bb)==typeid(Box)) return sameBox(*ba,pa,*bb,pb);
    const PlanarHalfSpace* ha=dynamic_cast<const PlanarHalfSpace*>(ga); const PlanarHalfSpace* hb=dynamic_cast<const PlanarHalfSpace*>(gb);
    if (ha && hb && typeid(*ha)==typeid(PlanarHalfSpace) && typeid(*hb)==typeid(PlanarHalfSpace)) return sameHalfSpace(*ha,pa,*hb,pb);
    const QuadricSurface* qa=dynamic_cast<const QuadricSurface*>(ga); const QuadricSurface* qb=dynamic_cast<const QuadricSurface*>(gb);
    if (qa && qb && typeid(*qa)==typeid(QuadricSurface) && typeid(*qb)==typeid(QuadricSurface)) return sameQuadric(*qa,pa,*qb,pb);
    if (sa && qb && typeid(*sa)==typeid(Sphere) && typeid(*qb)==typeid(QuadricSurface)) return sphereQuadric(*sa,pa,*qb,pb);
    if (sb && qa && typeid(*sb)==typeid(Sphere) && typeid(*qa)==typeid(QuadricSurface)) return sphereQuadric(*sb,pb,*qa,pa);
    if (ha && qb && typeid(*ha)==typeid(PlanarHalfSpace) && typeid(*qb)==typeid(QuadricSurface)) return halfSpaceQuadric(*ha,pa,*qb,pb);
    if (hb && qa && typeid(*hb)==typeid(PlanarHalfSpace) && typeid(*qa)==typeid(QuadricSurface)) return halfSpaceQuadric(*hb,pb,*qa,pa);
    return false;
}
