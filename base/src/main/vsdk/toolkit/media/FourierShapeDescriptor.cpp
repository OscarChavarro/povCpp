#include <cmath>
#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/FourierShapeDescriptor.h"
FourierShapeDescriptor::FourierShapeDescriptor(const java::String* label) :
    ShapeDescriptor(label) {
    featureVector = new double[numberOfElements * numberOfHarmonics];
    for (int i = 0; i < numberOfElements * numberOfHarmonics; i++) {
        featureVector[i] = 0.0;
    }
}

FourierShapeDescriptor::~FourierShapeDescriptor() {
    if (featureVector != nullptr) {
        delete[] featureVector;
        featureVector = nullptr;
    }
}

void FourierShapeDescriptor::setFeature(int sphere, int harmonic, double r, double i) {
    if (sphere < 0 || sphere >= numberOfElements || harmonic < 0 || harmonic >= numberOfHarmonics) {
        return;
    }
    double harmonicAmplitude = std::sqrt(r * r + i * i);
    featureVector[sphere * numberOfHarmonics + harmonic] = harmonicAmplitude;
}

double* FourierShapeDescriptor::getFeatureVector() const {
    if (featureVector == nullptr) {
        return nullptr;
    }
    double* copy = new double[numberOfElements * numberOfHarmonics];
    for (int i = 0; i < numberOfElements * numberOfHarmonics; i++) {
        copy[i] = featureVector[i];
    }
    return copy;
}

void FourierShapeDescriptor::setFeatureVector(double* vector) {
    if (vector == nullptr) {
        return;
    }
    for (int i = 0; i < numberOfElements * numberOfHarmonics; i++) {
        featureVector[i] = vector[i];
    }
}

java::String* FourierShapeDescriptor::toString() const {
    char fullBuffer[8192];
    int offset = 0;
    offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset, "SphericalHarmonics amplitudes for %d spheres and %d harmonics:\n",
        numberOfElements, numberOfHarmonics);

    for (int i = 0; i < numberOfElements * numberOfHarmonics; i++) {
        char line[256];
        java::String formatted = VSDK::formatDouble(featureVector[i]);
        offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset, "  - %s\n", formatted.c_str());
    }

    java::String* result = new java::String(fullBuffer);
    return result;
}
