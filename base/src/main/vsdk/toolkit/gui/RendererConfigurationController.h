#ifndef __RENDERER_CONFIGURATION_CONTROLLER__
#define __RENDERER_CONFIGURATION_CONTROLLER__

#include "vsdk/toolkit/gui/Controller.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
class RendererConfiguration;

class RendererConfigurationController : public Controller {
private:
    RendererConfiguration* qualitySelection;

public:
    RendererConfigurationController();
    explicit RendererConfigurationController(RendererConfiguration* qualitySelection);
    virtual ~RendererConfigurationController() {}

    void setRendererConfiguration(RendererConfiguration* q);

    bool processKeyPressedEvent(const KeyEvent& keyEvent);
    bool processKeyReleasedEvent(const KeyEvent& keyEvent);
};


#endif
