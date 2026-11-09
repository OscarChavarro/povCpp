#ifndef __PROGRESS_MONITOR_CONSOLE_LONG_FORMAT__
#define __PROGRESS_MONITOR_CONSOLE_LONG_FORMAT__

#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
class ProgressMonitorConsoleLongFormat : public ProgressMonitor {
private:
    long updateCount;
    int charactersPrintedInLastLine;
    double currentPercent;

public:
    ProgressMonitorConsoleLongFormat();
    virtual ~ProgressMonitorConsoleLongFormat() override {}

    virtual void begin() override;
    virtual void end() override;
    virtual void update(double minValue, double maxValue, double currentValue) override;
    virtual double getCurrentPercent() override;
};

#endif
