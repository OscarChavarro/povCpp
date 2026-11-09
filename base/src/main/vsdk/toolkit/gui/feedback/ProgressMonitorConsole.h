#ifndef __PROGRESS_MONITOR_CONSOLE__
#define __PROGRESS_MONITOR_CONSOLE__

#include <pthread.h>
#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
class ProgressMonitorConsole : public ProgressMonitor {
private:
    pthread_mutex_t lock;
    double currentPercent;
    double jumpPercent;
    int lastPrintedLabel;

    bool testLabelLimit(int limit);

public:
    ProgressMonitorConsole();
    virtual ~ProgressMonitorConsole() override;

    virtual void begin() override;
    virtual void end() override;
    virtual void update(double minValue, double maxValue, double currentValue) override;
    virtual double getCurrentPercent() override;
};

#endif
