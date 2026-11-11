#ifndef VSDK_GEOMETRY_WORKSPACE_TEST_WORKER_H
#define VSDK_GEOMETRY_WORKSPACE_TEST_WORKER_H
#include "java/lang/Runnable.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryQueryWorkspace.h"
class Sphere;
class GeometryWorkspaceTestWorker : public java::Runnable {
private:
    const Sphere* sphere;
    GeometryQueryWorkspace workspace;
    bool success;
public:
    explicit GeometryWorkspaceTestWorker(const Sphere* sphere);
    void run() override;
    bool passed() const;
    const RaySolidBoundary* boundaryStorage();
};
#endif
