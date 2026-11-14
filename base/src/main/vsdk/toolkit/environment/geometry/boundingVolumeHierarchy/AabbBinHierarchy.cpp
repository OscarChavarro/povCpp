#include <cmath>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBinHierarchy.h"

AabbBinHierarchy::AabbBinHierarchy()
{
}

void AabbBinHierarchy::build(const java::ArrayList<AxisAlignedBoundingBox> &boxes,
    const java::ArrayList<int> &ids)
{
    binBounds.clear();
    binMemberStart.clear();
    binMemberCount.clear();
    memberIds.clear();
    memberBounds.clear();
    const long int total = boxes.size();
    if (total == 0) {
        return;
    }

    Vector3Dd lo(1e30, 1e30, 1e30);
    Vector3Dd hi(-1e30, -1e30, -1e30);
    for (long int i = 0; i < total; i++) {
        const Vector3Dd c = boxes[i].centroid();
        lo = Vector3Dd(std::fmin(lo.x(), c.x()), std::fmin(lo.y(), c.y()), std::fmin(lo.z(), c.z()));
        hi = Vector3Dd(std::fmax(hi.x(), c.x()), std::fmax(hi.y(), c.y()), std::fmax(hi.z(), c.z()));
    }
    const double spreadX = hi.x() - lo.x();
    const double spreadY = hi.y() - lo.y();
    const double spreadZ = hi.z() - lo.z();
    int axis = 0;
    if (spreadY > spreadX && spreadY >= spreadZ) {
        axis = 1;
    } else if (spreadZ > spreadX && spreadZ > spreadY) {
        axis = 2;
    }

    // Stable insertion sort of the primitives by centroid coordinate.
    java::ArrayList<double> keys(total);
    java::ArrayList<int> order(total);
    for (long int i = 0; i < total; i++) {
        const Vector3Dd c = boxes[i].centroid();
        const double key = axis == 0 ? c.x() : (axis == 1 ? c.y() : c.z());
        long int j = i - 1;
        keys.add(key);
        order.add((int)i);
        while (j >= 0 && keys[j] > key) {
            keys[j + 1] = keys[j];
            order[j + 1] = order[j];
            j--;
        }
        keys[j + 1] = key;
        order[j + 1] = (int)i;
    }

    int binCount = (int)std::sqrt((double)total);
    if (binCount < 1) {
        binCount = 1;
    }
    if (binCount > MAXIMUM_BIN_COUNT) {
        binCount = MAXIMUM_BIN_COUNT;
    }
    const int base = (int)total / binCount;
    const int remainder = (int)total % binCount;
    memberIds.reserve(total);
    memberBounds.reserve(total);
    long int cursor = 0;
    for (int b = 0; b < binCount; b++) {
        const int count = base + (b < remainder ? 1 : 0);
        AxisAlignedBoundingBox aggregate = AxisAlignedBoundingBox::empty();
        const int start = (int)memberIds.size();
        for (int k = 0; k < count; k++) {
            const int primitive = order[cursor++];
            memberIds.add(ids[primitive]);
            memberBounds.add(boxes[primitive]);
            aggregate = aggregate.enclosing(boxes[primitive]);
        }
        binBounds.add(aggregate);
        binMemberStart.add(start);
        binMemberCount.add(count);
    }
}

long int AabbBinHierarchy::getBinCount() const
{
    return binBounds.size();
}

long int AabbBinHierarchy::getMemberCount() const
{
    return memberIds.size();
}

const AxisAlignedBoundingBox &AabbBinHierarchy::getBinBounds(long int bin) const
{
    return binBounds[bin];
}
