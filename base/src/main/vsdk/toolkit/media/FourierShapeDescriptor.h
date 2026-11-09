#ifndef __FOURIER_SHAPE_DESCRIPTOR__
#define __FOURIER_SHAPE_DESCRIPTOR__

#include "java/lang/String.h"
#include "vsdk/toolkit/media/ShapeDescriptor.h"
/**
Stores the feature vector for a set of 32 elements
around a volume or image.
*/
class FourierShapeDescriptor : public ShapeDescriptor {

private:
    double* featureVector;
    static const int numberOfElements = 32;
    static const int numberOfHarmonics = 16;

public:
    FourierShapeDescriptor(const java::String* label);
    virtual ~FourierShapeDescriptor();

    /**
    Set the Fourier transform (spherical harmonic) for sphere `sphere`, harmonic `harmonic` to complex value <r, i>
    */
    void setFeature(int sphere, int harmonic, double r, double i);

    virtual double* getFeatureVector() const override;

    virtual void setFeatureVector(double* vector) override;

    java::String* toString() const;
};

#endif
