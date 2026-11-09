#ifndef __PARALLEL_PROGRESS_MONITOR_EVENT__
#define __PARALLEL_PROGRESS_MONITOR_EVENT__

#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorCommand.h"
class ParallelProgressMonitorEvent {
private:
    ParallelProgressMonitorCommand commandType;
    long long numberOfElementsToProcess;

public:
    ParallelProgressMonitorEvent()
        : commandType(FINISH), numberOfElementsToProcess(0) {}

    ParallelProgressMonitorEvent(ParallelProgressMonitorCommand commandType, long long numberOfElementsToProcess)
        : commandType(commandType), numberOfElementsToProcess(numberOfElementsToProcess) {}

    ParallelProgressMonitorCommand getCommandType() const { return commandType; }
    long long getNumberOfElementsToProcess() const { return numberOfElementsToProcess; }
};

#endif
