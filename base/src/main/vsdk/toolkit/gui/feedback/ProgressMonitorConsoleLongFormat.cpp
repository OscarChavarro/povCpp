#include <cmath>
#include <cstdio>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitorConsoleLongFormat.h"
ProgressMonitorConsoleLongFormat::ProgressMonitorConsoleLongFormat()
    : updateCount(0), charactersPrintedInLastLine(0), currentPercent(0)
{
}

void ProgressMonitorConsoleLongFormat::begin()
{
    updateCount = 0;
    currentPercent = 0;
    charactersPrintedInLastLine = 0;
    std::printf("    ");
    std::fflush(stdout);
}

void ProgressMonitorConsoleLongFormat::end()
{
    currentPercent = 100;
    int pending = 55 - charactersPrintedInLastLine;
    for (int i = 0; i < pending; i++) {
        std::printf(" ");
    }
    std::printf(" - [100%% / Operation finished!] \n");
    std::fflush(stdout);
}

void ProgressMonitorConsoleLongFormat::update(double minValue, double maxValue, double currentValue)
{
    if ( std::abs(maxValue - minValue) < VSDK::EPSILON ) {
        return;
    }

    double v = 100 * (currentValue - minValue) / (maxValue - minValue);
    currentPercent = v;
    updateCount++;

    std::printf(".");
    charactersPrintedInLastLine++;

    if (updateCount % 10 == 0) {
        std::printf(" ");
        charactersPrintedInLastLine++;
    }
    if (updateCount % 50 == 0) {
        std::printf(" - [%s%% of %lld]\n    ", VSDK::formatDouble(v).c_str(), static_cast<long long>(std::llround(maxValue)));
        charactersPrintedInLastLine = 0;
    }
    std::fflush(stdout);
}

double ProgressMonitorConsoleLongFormat::getCurrentPercent()
{
    return currentPercent;
}
