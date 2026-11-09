#ifndef __SHAPE_DESCRIPTOR__
#define __SHAPE_DESCRIPTOR__

#include "java/lang/String.h"
#include "vsdk/toolkit/media/MediaEntity.h"
class ShapeDescriptor : public MediaEntity {

protected:
    java::String* label;

public:
    ShapeDescriptor(const java::String* label_);
    virtual ~ShapeDescriptor();

    java::String* getLabel() const;
    void setLabel(const java::String* label_);

    virtual double* getFeatureVector() const {
        return nullptr;
    }

    virtual void setFeatureVector(double* vector) {
    }
};

#endif
