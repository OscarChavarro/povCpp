#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuarticSolver.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"

BlobSolid::BlobSolid(double thresholdValue, bool useSturmSequences) :
    components(), threshold(thresholdValue), sturm(useSturmSequences)
{
}

Geometry* BlobSolid::copyGeometry() const { return new BlobSolid(*this); }

void BlobSolid::addComponent(const BlobComponent& component)
{
    components.add(component);
}

/**
Sorted list of the distances along the ray where the blob components start
and stop adding their influence. It would take a very complex blob (with many
components along the current ray) to warrant a faster sort technique.
*/
long int BlobSolid::determineInfluences(const Vector3Dd& p, const Vector3Dd& d,
    double influenceStart, java::ArrayList<BlobInfluenceInterval>& intervals) const
{
    intervals.clear();
    for (long int i = 0; i < components.size(); i++) {
        // Standard sphere intersection with the volume of influence.
        const Vector3Dd v = components[i].getCenter().subtract(p);
        const double b = v.dotProduct(d);
        const double t = v.dotProduct(v);
        double disc = b * b - t + components[i].getRadiusSquared();
        if (disc < GRAZING_LIMIT) {
            continue;
        }
        disc = java::Math::sqrt(disc);
        double t1 = b + disc;
        if (t1 < influenceStart) {
            t1 = 0.0;
        }
        double t0 = b - disc;
        if (t0 < influenceStart) {
            t0 = 0.0;
        }
        if (t1 == t0) {
            continue;
        }
        if (t1 < t0) {
            disc = t0;
            t0 = t1;
            t1 = disc;
        }
        long int k;
        for (k = 0; k < intervals.size() && t0 > intervals[k].getBound(); k++) {
        }
        if (k < intervals.size()) {
            // Smaller than an existing bound: insert it here.
            intervals.add(k, BlobInfluenceInterval(true, i, t0));
            for (k = k + 1; k < intervals.size() && t1 > intervals[k].getBound(); k++) {
            }
            if (k < intervals.size()) {
                intervals.add(k, BlobInfluenceInterval(false, i, t1));
            } else {
                intervals.add(BlobInfluenceInterval(false, i, t1));
            }
        } else {
            intervals.add(BlobInfluenceInterval(true, i, t0));
            intervals.add(BlobInfluenceInterval(false, i, t1));
        }
    }
    return intervals.size();
}

double BlobSolid::evaluateDensity(const Vector3Dd& pos) const
{
    double density = 0.0;
    for (long int i = 0; i < components.size(); i++) {
        const BlobComponent& component = components[i];
        const Vector3Dd v = component.getCenter().subtract(pos);
        const double len = v.dotProduct(v);
        if (len < component.getRadiusSquared()) {
            // Inside the radius of influence of this component.
            density += len * (len * component.getCoefficients()[0] +
                component.getCoefficients()[1]) + component.getCoefficients()[2];
        }
    }
    return density;
}

Vector3Dd BlobSolid::gradient(const Vector3Dd& point) const
{
    Vector3Dd result(0.0, 0.0, 0.0);
    for (long int i = 0; i < components.size(); i++) {
        const BlobComponent& component = components[i];
        const Vector3Dd v(point.x() - component.getCenter().x(),
            point.y() - component.getCenter().y(), point.z() - component.getCenter().z());
        const double dist = v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
        if (dist <= component.getRadiusSquared()) {
            const double val = -2.0 * (2.0 * component.getCoefficients()[0] * dist +
                component.getCoefficients()[1]);
            result = Vector3Dd(result.x() + val * v.x(), result.y() + val * v.y(),
                result.z() + val * v.z());
        }
    }
    return result;
}

/**
With r^2 = t^2 + 2 t t1 + t0 along the ray, where t0 = (P - C).(P - C) and
t1 = D.(P - C) for the unit direction D, each component density
c0 r^4 + c1 r^2 + c2 is the quartic
    t^4 c0 + t^3 4 c0 t1 + t^2 (c1 + 2 c0 t0 + 4 c0 t1^2)
    + t 2 (c1 t1 + 2 c0 t0 t1) + c2 + c1 t0 + c0 t0^2,
summed over the components whose influence intervals overlap.
*/
void BlobSolid::intersect(const Vector3Dd& p, const Vector3Dd& d, double minimumDistance,
    double influenceStart, java::ArrayList<double>& distances) const
{
    java::ArrayList<BlobInfluenceInterval> intervals(2 * components.size());
    const long int cnt = determineInfluences(p, d, influenceStart, intervals);
    if (cnt == 0) {
        return;
    }
    java::ArrayList<double> componentTerms(components.size() * 5);
    for (long int i = 0; i < components.size() * 5; i++) componentTerms.add(0.0);
    double coefficients[5];
    double roots[4];
    for (int i = 0; i < 4; i++) {
        coefficients[i] = 0.0;
    }
    coefficients[4] = -threshold;
    int inFlag = 0;
    for (long int i = 0; i < cnt; i++) {
        double* terms = &componentTerms[intervals[i].getComponent() * 5];
        if (intervals[i].isStart()) {
            // A component starts influencing the ray: add its terms.
            inFlag++;
            const BlobComponent& component = components[intervals[i].getComponent()];
            const Vector3Dd v = p.subtract(component.getCenter());
            const double c0 = component.getCoefficients()[0];
            const double c1 = component.getCoefficients()[1];
            const double c2 = component.getCoefficients()[2];
            const double t0 = v.dotProduct(v);
            const double t1 = v.dotProduct(d);
            terms[0] = c0;
            terms[1] = 4.0 * c0 * t1;
            terms[2] = 2.0 * c0 * (2.0 * t1 * t1 + t0) + c1;
            terms[3] = 2.0 * t1 * (2.0 * c0 * t0 + c1);
            terms[4] = c0 * t0 * t0 + c1 * t0 + c2;
            for (int j = 0; j < 5; j++) {
                coefficients[j] += terms[j];
            }
        } else {
            // A component stops influencing the ray: subtract its terms.
            for (int j = 0; j < 5; j++) {
                coefficients[j] -= terms[j];
            }
            if (--inFlag == 0) {
                // No component affects the ray until the next start.
                continue;
            }
        }
        int rootCount;
        if (!sturm) {
            rootCount = QuarticSolver::solve(coefficients, &roots[0], minimumDistance, SOLVER_EPSILON);
        } else if (java::Math::abs(coefficients[0]) < COEFFICIENT_LIMIT) {
            if (java::Math::abs(coefficients[1]) < COEFFICIENT_LIMIT) {
                rootCount = QuadraticSolver::solve(&coefficients[2], &roots[0]);
            } else {
                PolynomialSolver cubicSolver(3, &coefficients[1]);
                rootCount = cubicSolver.solve(&roots[0], minimumDistance, SOLVER_EPSILON);
            }
        } else {
            PolynomialSolver quarticSolver(4, coefficients);
            rootCount = quarticSolver.solve(&roots[0], minimumDistance, SOLVER_EPSILON);
        }
        for (int j = 0; j < rootCount; j++) {
            const double dist = roots[j];
            // Keep roots inside the interval of the active components whose
            // gradient shows a real crossing rather than numerical noise.
            if (dist >= intervals[i].getBound() && dist <= intervals[i + 1].getBound()) {
                const Vector3Dd point = d.multiply(dist).add(p);
                const Vector3Dd n = gradient(point);
                if (n.dotProduct(n) >= GRAZING_LIMIT) {
                    distances.add(dist);
                }
            }
        }
    }
}

FloatingInterval BlobSolid::clampedFalloff(long int index, const FloatingInterval& x,
    const FloatingInterval& y, const FloatingInterval& z) const
{
    const BlobComponent& component = components[index];
    const FloatingInterval dx = x.subtract(FloatingInterval::exact(component.getCenter().x()));
    const FloatingInterval dy = y.subtract(FloatingInterval::exact(component.getCenter().y()));
    const FloatingInterval dz = z.subtract(FloatingInterval::exact(component.getCenter().z()));
    const FloatingInterval q = dx.square().add(dy.square()).add(dz.square()).divide(
        FloatingInterval::exact(component.getRadius()).square());
    if (!q.isValid()) return q;
    if (q.getLower() >= 1.0) return FloatingInterval::exact(0.0);
    // 1 - min(q, 1): the exact falloff is continuous and zero beyond the radius.
    return FloatingInterval::exact(1.0).subtract(FloatingInterval::spanning(
        java::Math::max(q.getLower(), 0.0), java::Math::min(q.getUpper(), 1.0)));
}

FloatingInterval BlobSolid::encloseField(const FloatingInterval& x, const FloatingInterval& y,
    const FloatingInterval& z) const
{
    if (!x.isValid() || !y.isValid() || !z.isValid() || !java::Double::isFinite(threshold))
        return FloatingInterval();
    FloatingInterval result = FloatingInterval::exact(threshold);
    for (long int i = 0; i < components.size(); i++) {
        if (components[i].isDegenerate()) return FloatingInterval();
        const FloatingInterval e = clampedFalloff(i, x, y, z);
        result = result.subtract(FloatingInterval::exact(components[i].getStrength()).multiply(e.square()));
    }
    return result;
}

bool BlobSolid::encloseFieldGradient(const FloatingInterval& x, const FloatingInterval& y,
    const FloatingInterval& z, FloatingInterval& gx, FloatingInterval& gy, FloatingInterval& gz) const
{
    if (!x.isValid() || !y.isValid() || !z.isValid()) return false;
    gx = gy = gz = FloatingInterval::exact(0.0);
    for (long int i = 0; i < components.size(); i++) {
        const BlobComponent& component = components[i];
        if (component.isDegenerate()) return false;
        // grad (threshold - s (1 - q)^2) = 4 s (1 - q) (p - c) / radius^2
        const FloatingInterval scale = FloatingInterval::exact(4.0 * component.getStrength()).multiply(
            clampedFalloff(i, x, y, z)).divide(FloatingInterval::exact(component.getRadius()).square());
        gx = gx.add(scale.multiply(x.subtract(FloatingInterval::exact(component.getCenter().x()))));
        gy = gy.add(scale.multiply(y.subtract(FloatingInterval::exact(component.getCenter().y()))));
        gz = gz.add(scale.multiply(z.subtract(FloatingInterval::exact(component.getCenter().z()))));
    }
    return gx.isValid() && gy.isValid() && gz.isValid();
}

bool BlobSolid::findRayRoots(const Vector3Dd& origin, const Vector3Dd& unitDirection,
    double, java::ArrayList<double>& distances) const
{
    for (long int i = 0; i < components.size(); i++)
        if (components[i].isDegenerate()) return false;
    if (!java::Double::isFinite(threshold)) return false;
    intersect(origin, unitDirection, 0.0, 0.0, distances);
    return true;
}

void BlobSolid::getBounds(GeometryBounds& result) const
{
    if (!java::Double::isFinite(threshold)) { result.setIndeterminate(); return; }
    for (long int i = 0; i < components.size(); i++)
        if (components[i].isDegenerate()) { result.setIndeterminate(); return; }
    // Density above a nonnegative threshold needs a positive component.
    if (threshold < 0) { result.setUnbounded(); return; }
    bool any = false;
    Vector3Dd minimum, maximum;
    for (long int i = 0; i < components.size(); i++) {
        const BlobComponent& component = components[i];
        if (component.getStrength() <= 0) continue;
        const FloatingInterval r = FloatingInterval::exact(component.getRadius());
        const double c[3] = {component.getCenter().x(), component.getCenter().y(), component.getCenter().z()};
        double low[3], high[3];
        for (int k = 0; k < 3; k++) {
            low[k] = FloatingInterval::exact(c[k]).subtract(r).getLower();
            high[k] = FloatingInterval::exact(c[k]).add(r).getUpper();
        }
        const Vector3Dd a(low[0], low[1], low[2]), b(high[0], high[1], high[2]);
        if (!any) { minimum = a; maximum = b; any = true; continue; }
        minimum = Vector3Dd(java::Math::min(minimum.x(), a.x()), java::Math::min(minimum.y(), a.y()),
            java::Math::min(minimum.z(), a.z()));
        maximum = Vector3Dd(java::Math::max(maximum.x(), b.x()), java::Math::max(maximum.y(), b.y()),
            java::Math::max(maximum.z(), b.z()));
    }
    if (!any) { result.setEmpty(); return; }
    if (!result.setFinite(minimum, maximum)) result.setIndeterminate();
}
