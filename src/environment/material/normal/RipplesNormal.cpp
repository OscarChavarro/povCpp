#include "environment/material/normal/RipplesNormal.h"

RipplesNormal::RipplesNormal(double bumpAmount, double frequency, double phase, int numberOfWaves) :
    bumpAmount(bumpAmount),
    frequency(frequency),
    phase(phase),
    numberOfWaves(numberOfWaves)
{
}

void
RipplesNormal::applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
    const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    bumpFixture.ripples(x, y, z, bumpAmount, frequency, phase, numberOfWaves, newNormal);
}

SolidTextureNormal *
RipplesNormal::copy() const
{
    return new RipplesNormal(bumpAmount, frequency, phase, numberOfWaves);
}

double
RipplesNormal::getBumpAmount() const
{
    return bumpAmount;
}

double
RipplesNormal::getFrequency() const
{
    return frequency;
}

double
RipplesNormal::getPhase() const
{
    return phase;
}

int
RipplesNormal::getNumberOfWaves() const
{
    return numberOfWaves;
}
