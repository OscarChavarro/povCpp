#include "java/lang/String.h"
#include "vsdk/toolkit/media/ShapeDescriptor.h"
ShapeDescriptor::ShapeDescriptor(const java::String* label_) {
    if (label_ != nullptr) {
        label = new java::String(*label_);
    } else {
        label = nullptr;
    }
}

ShapeDescriptor::~ShapeDescriptor() {
    if (label != nullptr) {
        delete label;
        label = nullptr;
    }
}

java::String* ShapeDescriptor::getLabel() const {
    if (label != nullptr) {
        return new java::String(*label);
    }
    return nullptr;
}

void ShapeDescriptor::setLabel(const java::String* label_) {
    if (label != nullptr) {
        delete label;
    }
    if (label_ != nullptr) {
        label = new java::String(*label_);
    } else {
        label = nullptr;
    }
}
