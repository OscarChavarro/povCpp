#ifndef __RENDER_RUNTIME_STATE__
#define __RENDER_RUNTIME_STATE__

class RenderRuntimeState {
  private:
    double maxTraceLevelValue;
    bool stopFlagValue;

  public:
    RenderRuntimeState() { reset(); }

    double &getMaxTraceLevel() { return maxTraceLevelValue; }
    bool &getStopFlag() { return stopFlagValue; }
    void reset();
};

#endif
