#ifndef __RENDER_RUNTIME_STATE__
#define __RENDER_RUNTIME_STATE__

class RenderRuntimeState {
  private:
    double maxTraceLevelValue;
    volatile int stopFlagValue;

  public:
    RenderRuntimeState() { reset(); }

    double &getMaxTraceLevel() { return maxTraceLevelValue; }
    volatile int &getStopFlag() { return stopFlagValue; }

    static RenderRuntimeState *sActive;
    static void installActive(RenderRuntimeState *p) { sActive = p; }
    static inline RenderRuntimeState &global() {
        if (!sActive) {
            static RenderRuntimeState defaultInstance;
            sActive = &defaultInstance;
        }
        return *sActive;
    }
    static inline double &maxTraceLevel() {
        if (!sActive) {
            static RenderRuntimeState defaultInstance;
            sActive = &defaultInstance;
        }
        return sActive->maxTraceLevelValue;
    }
    static inline volatile int &stopFlag() {
        if (!sActive) {
            static RenderRuntimeState defaultInstance;
            sActive = &defaultInstance;
        }
        return sActive->stopFlagValue;
    }
    void reset();
};

#endif
