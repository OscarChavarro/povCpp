#include <cstdio>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitorConsole.h"
ProgressMonitorConsole::ProgressMonitorConsole()
    : currentPercent(0.0), jumpPercent(2.0), lastPrintedLabel(0)
{
    pthread_mutex_init(&lock, 0);
}

ProgressMonitorConsole::~ProgressMonitorConsole()
{
    pthread_mutex_destroy(&lock);
}

void ProgressMonitorConsole::begin()
{
    pthread_mutex_lock(&lock);
    currentPercent = 0;
    lastPrintedLabel = 0;
    jumpPercent = 2;
    std::printf("[ 0%% ");
    std::fflush(stdout);
    pthread_mutex_unlock(&lock);
}

void ProgressMonitorConsole::end()
{
    pthread_mutex_lock(&lock);
    std::printf(" 100%% ]\n");
    std::fflush(stdout);
    pthread_mutex_unlock(&lock);
}

bool ProgressMonitorConsole::testLabelLimit(int limit)
{
    if ( limit == lastPrintedLabel ) return false;

    if ( currentPercent - 6 * jumpPercent / 10 < limit &&
         currentPercent + 6 * jumpPercent / 10 > limit ) {
        std::printf(" %d%% ", limit);
        lastPrintedLabel = limit;
        return true;
    }
    return false;
}

void ProgressMonitorConsole::update(double minValue, double maxValue, double currentValue)
{
    pthread_mutex_lock(&lock);
    if ( (maxValue - minValue) < VSDK::EPSILON ) {
        pthread_mutex_unlock(&lock);
        return;
    }

    double v = 100 * (currentValue - minValue) / (maxValue - minValue);

    while ( currentPercent + jumpPercent < v ) {
        currentPercent += jumpPercent;
        if ( !testLabelLimit(25) && !testLabelLimit(50) && !testLabelLimit(75) ) {
            std::printf("-");
        }
    }
    std::fflush(stdout);
    pthread_mutex_unlock(&lock);
}

double ProgressMonitorConsole::getCurrentPercent()
{
    pthread_mutex_lock(&lock);
    double result = currentPercent;
    pthread_mutex_unlock(&lock);
    return result;
}
