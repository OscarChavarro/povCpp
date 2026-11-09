#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/PrimitiveCountShapeDescriptor.h"
PrimitiveCountShapeDescriptor::PrimitiveCountShapeDescriptor(const java::String* label) :
    ShapeDescriptor(label) {
    featureVector = new double[numberOfElements];
    for (int i = 0; i < numberOfElements; i++) {
        featureVector[i] = 0.0;
    }
}

PrimitiveCountShapeDescriptor::~PrimitiveCountShapeDescriptor() {
    if (featureVector != nullptr) {
        delete[] featureVector;
        featureVector = nullptr;
    }
}

void PrimitiveCountShapeDescriptor::setFeature(int primitiveType, long count) {
    if (primitiveType < 0 || primitiveType >= numberOfElements) {
        return;
    }
    featureVector[primitiveType] = (double)count;
}

double* PrimitiveCountShapeDescriptor::getFeatureVector() const {
    if (featureVector == nullptr) {
        return nullptr;
    }
    double* copy = new double[numberOfElements];
    for (int i = 0; i < numberOfElements; i++) {
        copy[i] = featureVector[i];
    }
    return copy;
}

void PrimitiveCountShapeDescriptor::setFeatureVector(double* vector) {
    if (vector == nullptr) {
        return;
    }
    for (int i = 0; i < numberOfElements; i++) {
        featureVector[i] = vector[i];
    }
}

java::String* PrimitiveCountShapeDescriptor::toString() const {
    char buffer[2048];
    char fullBuffer[4096];
    int offset = 0;
    offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset, "Primitive counts for %d types:\n", numberOfElements);

    for (int i = 0; i < numberOfElements; i++) {
        char line[256];
        java::String formatted = VSDK::formatDouble(featureVector[i]);
        offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset, "  - %s\n", formatted.c_str());
    }

    java::String* result = new java::String(fullBuffer);
    return result;
}
