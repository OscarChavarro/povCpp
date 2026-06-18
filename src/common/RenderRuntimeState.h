#ifndef __RENDER_RUNTIME_STATE__
#define __RENDER_RUNTIME_STATE__

class RenderRuntimeState {
  public:
    static double &maxTraceLevel();
    static volatile int &stopFlag();
    static void reset();
};

#endif
