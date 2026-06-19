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
    void reset();
};

#endif
