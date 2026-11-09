#ifndef __TANGIBLE_INTERFACE_LISTENER__
#define __TANGIBLE_INTERFACE_LISTENER__

class TangibleInterfaceEvent;

class TangibleInterfaceListener {
public:
    virtual ~TangibleInterfaceListener() {}
    virtual void tangibleInterfaceEventReceived(const TangibleInterfaceEvent& event) = 0;
};

#endif
