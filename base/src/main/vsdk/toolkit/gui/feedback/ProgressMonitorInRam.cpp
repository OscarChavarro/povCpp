#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitorInRam.h"
ProgressMonitorInRam::ProgressMonitorInRam()
    : currentPercent(0)
{
}

void ProgressMonitorInRam::begin()
{
    currentPercent = 0;
}

void ProgressMonitorInRam::end()
{
    currentPercent = 100.0;
}

void ProgressMonitorInRam::update(double minValue, double maxValue, double currentValue)
{
    if ( (maxValue - minValue) < VSDK::EPSILON ) {
        return;
    }
    currentPercent = 100 * (currentValue - minValue) / (maxValue - minValue);
}

double ProgressMonitorInRam::getCurrentPercent()
{
    return currentPercent;
}
