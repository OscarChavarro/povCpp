#include "java/lang/Math.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/volume/HeightField.h"
#include "java/util/PriorityQueue.txx"

HeightField::HeightField(long columns, long rows, const Vector3Dd &minBounds,
    const Vector3Dd &maxBounds) :
    solid(new HeightFieldSolid(columns, rows, minBounds, maxBounds))
{
}

HeightField::HeightField(const HeightField &other) :
    solid(new HeightFieldSolid(*other.solid))
{
}

HeightField::~HeightField()
{
    delete solid;
}

void
HeightField::setWaterLevel(double level)
{
    solid->setBounds(solid->getMinimum().withY(level), solid->getMaximum());
    solid->updateBlocks();
}

BoundingVolumeHierarchy *
HeightField::createBoundingVolume() const
{
    return new AabbBoundingVolume(AxisAlignedBoundingBox::fromTransformedCorners(
        solid->getMinimum(), solid->getMaximum(), nullptr));
}

double
HeightField::imageHeight(double height) const
{
    return height <= solid->getMinimum().y() ? HIDDEN_HEIGHT : height;
}

void
HeightField::findHfMinMax(HeightField *hField,
    const IndexedColorImageHDRUncompressed *image, int imageType)
{
    int maxX = image->getXSize();
    if (imageType == HeightField::POT) {
        maxX = maxX / 2;
    }
    const int maxZ = image->getYSize();
    for (int z = 0; z <= maxZ; z++) {
        for (int x = 0; x <= maxX; x++) {
            double height = HIDDEN_HEIGHT;
            if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                const int temp1 = image->getPixel(x, maxZ - z - 1);
                if (imageType == HeightField::POT) {
                    const int temp2 = image->getPixel(x + maxX, maxZ - z - 1);
                    height = hField->imageHeight((double)temp1 + (double)temp2 / 256.0);
                } else {
                    height = hField->imageHeight((double)temp1);
                }
            }
            hField->solid->setHeight(x, z, height);
        }
    }
    hField->solid->updateBlocks();
}

void
HeightField::findHfMinMax(HeightField *hField,
    const RGBAImageHDRUncompressed *image, int imageType)
{
    (void)imageType;
    const int maxX = image->getXSize();
    const int maxZ = image->getYSize();
    for (int z = 0; z <= maxZ; z++) {
        for (int x = 0; x <= maxX; x++) {
            double height = HIDDEN_HEIGHT;
            if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                RGBAPixelHDR pixel;
                image->getPixel(x, maxZ - z - 1, &pixel);
                height = hField->imageHeight((double)pixel.r + (double)pixel.g / 256.0);
            }
            hField->solid->setHeight(x, z, height);
        }
    }
    hField->solid->updateBlocks();
}

int
HeightField::traceCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayHtFieldTests();
    stats.incrementRayBoxTests();
    double depth = 0.0;
    const HeightFieldTraceResult result = solid->traceFirstHit(
        ray->getOrigin(), ray->getDirection(), depth);
    if (result == HeightFieldTraceResult::MissedBounds) {
        return (false);
    }
    stats.incrementRayBoxTestsSucceeded();
    if (result != HeightFieldTraceResult::Hit) {
        return (false);
    }
    IntersectionCandidate localElement;
    localElement.getAttributes().setMaterial(materialOverride);
    if (context != nullptr) {
        localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context->getMaterialUsesObjectLocalPoint());
    }
    localElement.getIntersection().t = depth;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth).add(ray->getOrigin());
    localElement.getAttributes().setHitGeometry(this);
    depthQueue->offer(localElement);
    stats.incrementRayHtFieldTestsSucceeded();
    return (true);
}

int
HeightField::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
HeightField::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, context.getMaterialOverride(), &context);
}

// PovRay 1.0 containment: the integer-truncated plane test of the triangle
// under the point, and only above one unit over the box floor.
int
HeightField::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    const Vector3Dd &test = testPoint;
    int px = (int)test.x();
    int pz = (int)test.z();
    const double x = test.x() - (double)px;
    const double z = test.z() - (double)pz;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;
    if ((x + z) < 1.0) {
        if (px < 0 || pz < 0 || px + 1 >= solid->getColumnCount() || pz + 1 >= solid->getRowCount()) {
            return OUTSIDE;
        }
        const double y1 = solid->getHeight(px, pz);
        const double y2 = solid->getHeight(px + 1, pz);
        const double y3 = solid->getHeight(px, pz + 1);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, 1.0);
    } else {
        px = java::Math::ceil(test.x());
        pz = java::Math::ceil(test.z());
        if (px < 1 || pz < 1 || px >= solid->getColumnCount() || pz >= solid->getRowCount()) {
            return OUTSIDE;
        }
        const double y1 = solid->getHeight(px, pz);
        const double y2 = solid->getHeight(px - 1, pz);
        const double y3 = solid->getHeight(px, pz - 1);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(-1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, -1.0);
    }
    Vector3Dd localNormal = temp2.crossProduct(temp1);
    if (localNormal.y() < 0.0) {
        localNormal = localNormal.multiply(-1.0);
    }
    const int dot1 = (int)test.dotProduct(localNormal);
    const int dot2 = (int)localOrigin.dotProduct(localNormal);
    if ((dot1 < dot2) && (test.y() > solid->getMinimum().y() + 1.0)) {
        return INSIDE;
    }
    return OUTSIDE;
}

void
HeightField::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    Vector3Dd temp1;
    Vector3Dd temp2;
    solid->getSurfaceTangents(*intersectionPoint, temp1, temp2);
    *result = temp2.crossProduct(temp1);
    *result = (*result).normalizedFast();
}

RaySolidQueryStatus
HeightField::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    return solid->classifyRay(ray, query, result);
}

void *
HeightField::copy()
{
    return new HeightField(*this);
}

void
HeightField::invertGeometry()
{
}
