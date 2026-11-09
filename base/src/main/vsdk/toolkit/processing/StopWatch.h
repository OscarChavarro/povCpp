#ifndef __STOP_WATCH__
#define __STOP_WATCH__

class StopWatch {
private:
    bool running;
    long long startTime;
    double elapsedSeconds;

public:
    StopWatch();
    void start();
    void stop();
    double getElapsedRealTime() const;
};

#endif
