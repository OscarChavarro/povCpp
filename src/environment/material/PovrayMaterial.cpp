#include "environment/material/PovrayMaterial.h"

#include "java/util/ArrayList.txx"
#include "environment/material/MaterialUtils.h"

PovrayMaterial::PovrayMaterial()
{
    setNumberOfWaves(DEFAULT_NUMBER_OF_WAVES);
}

PovrayMaterial *
PovrayMaterial::copy()
{
    return MaterialUtils::instance().copyTexture(this);
}

void
PovrayMaterial::translate(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    MaterialUtils::instance().translateTexture(&self, vector);
}

void
PovrayMaterial::rotate(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    MaterialUtils::instance().rotateTexture(&self, vector);
}

void
PovrayMaterial::scale(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    MaterialUtils::instance().scaleTexture(&self, vector);
}
