#include "java/lang/Math.h"
#include "java/lang/UnsupportedOperationException.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "environment/geometry/SurfaceDetailKernel.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/surface/parametric/PatchScratch.h"

void
SurfaceDetailKernel::resolve(const RayWithTracingState &ray, double t, RayCastingHit *hit)
{
    if (!hit->needsNormal()) {
        return;
    }
    Geometry *surface = hit->hitSurface;
    switch (hit->hitSurfaceFamily) {
    case SurfaceDetailFamily::None:
    case SurfaceDetailFamily::Empty:
        return;
    case SurfaceDetailFamily::Quadric: {
        const Vector3Dd gradient = static_cast<const QuadricSurface *>(surface)->gradient(hit->p);
        const double len = java::Math::sqrt(gradient.x() * gradient.x() +
            gradient.y() * gradient.y() + gradient.z() * gradient.z());
        if (len == 0.0) {
            hit->n = Vector3Dd(1.0, 0.0, 0.0);
        } else {
            hit->n = Vector3Dd(gradient.x() / len, gradient.y() / len, gradient.z() / len);
        }
        return;
    }
    case SurfaceDetailFamily::Box: {
        const Box *box = static_cast<const Box *>(surface);
        hit->n = Box::normalForBounds(hit->p, box->getRawBounds()[0], box->getRawBounds()[1],
            GeometryConfig::INTERSECTION_EPSILON);
        if (hit->n.x() == 0 && hit->n.y() == 0 && hit->n.z() == 0) {
            hit->n = Vector3Dd(1, 0, 0);
        }
        return;
    }
    case SurfaceDetailFamily::Plane:
        hit->n = static_cast<const PlanarHalfSpace *>(surface)->getBoundaryPlane().getCoefficients();
        return;
    case SurfaceDetailFamily::Sphere:
        hit->n = static_cast<const Sphere *>(surface)->surfaceNormalAtNonzeroPoint(hit->p);
        return;
    case SurfaceDetailFamily::Mesh:
        if (hit->n.length() > 1.0e-12) {
            return;
        }
        hit->n = Vector3Dd(0.0, 1.0, 0.0);
        return;
    case SurfaceDetailFamily::Polynomial: {
        const Vector3Dd gradient = static_cast<const PolynomialSurface *>(surface)->gradient(hit->p);
        const double length = java::Math::sqrt(gradient.x() * gradient.x() +
            gradient.y() * gradient.y() + gradient.z() * gradient.z());
        if (length > 0.0) {
            hit->n = Vector3Dd(gradient.x() / length, gradient.y() / length, gradient.z() / length);
        } else {
            hit->n = Vector3Dd(1.0, 0.0, 0.0);
        }
        hit->n = hit->n.normalizedFast();
        return;
    }
    case SurfaceDetailFamily::Blob: {
        const Vector3Dd gradient = static_cast<const BlobSolid *>(surface)->gradient(hit->p);
        double squared = gradient.x() * gradient.x() + gradient.y() * gradient.y() +
            gradient.z() * gradient.z();
        if (squared < GeometryConfig::INTERSECTION_EPSILON) {
            hit->n = Vector3Dd(1.0, 0.0, 0.0);
        } else {
            squared = 1.0 / java::Math::sqrt(squared);
            hit->n = gradient.multiply(squared);
        }
        hit->n = hit->n.normalizedFast();
        return;
    }
    case SurfaceDetailFamily::HeightField: {
        Vector3Dd first;
        Vector3Dd second;
        static_cast<const HeightFieldSolid *>(surface)->getSurfaceTangents(hit->p, first, second);
        hit->n = second.crossProduct(first).normalizedFast();
        return;
    }
    case SurfaceDetailFamily::BicubicPatch: {
        if (hit->n.length() > GeometryConfig::PARAMETRIC_CURVE_EPSILON) {
            hit->n = hit->n.normalizedFast();
            return;
        }
        PatchScratch &scratch =
            PatchScratch::forPatch(static_cast<const SubdividedBezierPatch *>(surface));
        const int intersectionCount = scratch.getIntersectionCount();
        for (int i = 0; i < intersectionCount; i++) {
            if (hit->p.subtract(scratch.getIntersectionPointAt(i)).length() <= 1.0e-6) {
                hit->n = scratch.getNormalVectorAt(i);
                return;
            }
        }
        hit->n = Vector3Dd(1.0, 0.0, 0.0);
        return;
    }
    case SurfaceDetailFamily::Generic: {
        const Ray local = Ray::fromAffine(ray.getOrigin(), ray.getDirection());
        RayHit detail(RayHit::DETAIL_NORMAL, false);
        if (hit->surfaceIndex >= 0) {
            const RayBoundary boundary(t, surface, false, hit->surfaceIndex);
            if (surface->querySurfaceDetail(local, boundary, &detail) != RayFirstHitStatus::Hit) {
                throw java::UnsupportedOperationException(
                    "Geometry did not resolve the selected surface");
            }
        } else {
            const Ray pointRay = Ray::fromAffine(hit->p, ray.getDirection());
            surface->doExtraInformation(pointRay, 0, &detail);
        }
        if (!detail.isDetailAvailabilityKnown() ||
            !(detail.getAvailableDetailMask() & RayHit::DETAIL_NORMAL)) {
            throw java::UnsupportedOperationException("Geometry did not provide a normal");
        }
        hit->n = detail.normal;
        return;
    }
    }
}
