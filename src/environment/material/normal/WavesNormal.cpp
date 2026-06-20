#include "environment/material/normal/WavesNormal.h"

WavesNormal::WavesNormal(double bumpAmount, double frequency, double phase, int numberOfWaves) :
    bumpAmount(bumpAmount),
    frequency(frequency),
    phase(phase),
    numberOfWaves(numberOfWaves)
{
}

void
WavesNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    bumpFixture.waves(x, y, z, bumpAmount, frequency, phase, numberOfWaves, newNormal);
}

SolidTextureNormal *
WavesNormal::copy() const
{
    return new WavesNormal(bumpAmount, frequency, phase, numberOfWaves);
}

double
WavesNormal::getBumpAmount() const
{
    return bumpAmount;
}

double
WavesNormal::getFrequency() const
{
    return frequency;
}

double
WavesNormal::getPhase() const
{
    return phase;
}

int
WavesNormal::getNumberOfWaves() const
{
    return numberOfWaves;
}
