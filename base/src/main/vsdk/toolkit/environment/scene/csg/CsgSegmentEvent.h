#ifndef VSDK_CSG_SEGMENT_EVENT_H
#define VSDK_CSG_SEGMENT_EVENT_H

/**
One boundary event of a ray's alternating in/out classification: the ray
parameter, whether the ray enters the solid there, and the caller's event
(a renderer's hit record, a crossing reference). [ROTH1982] section 2.2, the
t[i]/S[i] pairs of RAYCAST. The payload is copied, never interpreted.
*/
template <class Payload>
class CsgSegmentEvent {
private:
    double t;
    bool entering;
    Payload payload;
public:
    CsgSegmentEvent() : t(0.0), entering(false), payload() {}
    CsgSegmentEvent(double t, bool entering, const Payload& payload) :
        t(t), entering(entering), payload(payload) {}
    double getT() const { return t; }
    bool isEntering() const { return entering; }
    const Payload& getPayload() const { return payload; }
};

#endif
