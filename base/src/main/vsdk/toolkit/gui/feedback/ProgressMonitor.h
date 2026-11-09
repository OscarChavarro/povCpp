#ifndef __PROGRESS_MONITOR__
#define __PROGRESS_MONITOR__

#include "vsdk/toolkit/gui/PresentationElement.h"
class ProgressMonitor : public PresentationElement {
public:
    virtual ~ProgressMonitor() {}
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void update(double minValue, double maxValue, double currentValue) = 0;
    virtual double getCurrentPercent() = 0;
};

#endif
