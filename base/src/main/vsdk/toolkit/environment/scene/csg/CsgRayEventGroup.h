#ifndef VSDK_CSG_RAY_EVENT_GROUP_H
#define VSDK_CSG_RAY_EVENT_GROUP_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
// Events with exactly equal binary64 ray parameters, with stable leaf order.
class CsgRayEventGroup {
private:
    double t;
    java::ArrayList<CsgRayEventReference> references;
public:
    CsgRayEventGroup();
    explicit CsgRayEventGroup(double t);
    CsgRayEventGroup(const CsgRayEventGroup& other);
    CsgRayEventGroup& operator=(const CsgRayEventGroup& other);
    ~CsgRayEventGroup();
    double getT() const;
    bool add(const CsgRayEventReference& reference);
    long int size() const;
    const CsgRayEventReference* get(long int index) const;
};
#endif
