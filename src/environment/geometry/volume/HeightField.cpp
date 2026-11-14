#include "java/lang/Math.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/trace/HeightFieldTrace.h"
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
HeightField::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return HeightFieldTrace::emitCrossings(*solid, ray, depthQueue, materialOverride, nullptr);
}

int
HeightField::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return HeightFieldTrace::emitCrossings(*solid, ray, depthQueue,
        context.getMaterialOverride(), &context);
}

int
HeightField::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return HeightFieldTrace::containment(*solid, testPoint, distanceTolerance);
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
