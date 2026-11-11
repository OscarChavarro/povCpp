#include "java/util/ArrayList.txx"
#include "GeometryWorkspaceTestWorker.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
GeometryWorkspaceTestWorker::GeometryWorkspaceTestWorker(const Sphere* value)
    : sphere(value), workspace(), success(true) {}
void GeometryWorkspaceTestWorker::run()
{
    const Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    for (int i = 0; i < 500; i++) {
        workspace.clear();
        workspace.hitResult().resetForDistanceOnly();
        if (sphere->queryFirstHit(ray, RayCrossingQuery(0,4), &workspace.hitResult()) != RayFirstHitStatus::Hit ||
            workspace.hitResult().getHitDistance() != 1 ||
            sphere->classifyRay(ray, RayCrossingQuery(0,4), workspace.solidResult()) != RaySolidQueryStatus::Supported ||
            workspace.solidResult().boundaries.size() != 2 ||
            workspace.solidResult().boundaries[1].t != 2) {
            success = false;
            return;
        }
    }
}
bool GeometryWorkspaceTestWorker::passed() const { return success; }
const RaySolidBoundary* GeometryWorkspaceTestWorker::boundaryStorage()
{
    return workspace.solidResult().boundaries.data();
}
