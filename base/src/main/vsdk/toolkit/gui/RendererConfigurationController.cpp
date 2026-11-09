#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/RendererConfigurationController.h"
RendererConfigurationController::RendererConfigurationController()
    : qualitySelection(0)
{
}

RendererConfigurationController::RendererConfigurationController(RendererConfiguration* qualitySelection)
    : qualitySelection(qualitySelection)
{
}

void RendererConfigurationController::setRendererConfiguration(RendererConfiguration* q)
{
    qualitySelection = q;
}

bool RendererConfigurationController::processKeyPressedEvent(const KeyEvent& keyEvent)
{
    if (qualitySelection == 0) return false;

    bool updated = false;
    int st;

    switch (keyEvent.keycode) {
        case KeyEvent::KEY_F1:
            qualitySelection->changePoints();
            updated = true;
            break;
        case KeyEvent::KEY_F2:
            qualitySelection->changeWires();
            updated = true;
            break;
        case KeyEvent::KEY_F3:
            qualitySelection->changeSurfaces();
            updated = true;
            break;
        case KeyEvent::KEY_F4:
            qualitySelection->changeBoundingVolume();
            updated = true;
            break;
        case KeyEvent::KEY_F5:
            qualitySelection->changeNormals();
            updated = true;
            break;
        case KeyEvent::KEY_F6:
            qualitySelection->changeTrianglesNormals();
            updated = true;
            break;
        case KeyEvent::KEY_F7:
            st = qualitySelection->getShadingType();
            if (st == RendererConfiguration::SHADING_TYPE_NOLIGHT) {
                st = RendererConfiguration::SHADING_TYPE_FLAT;
            }
            else if (st == RendererConfiguration::SHADING_TYPE_FLAT) {
                st = RendererConfiguration::SHADING_TYPE_GOURAUD;
            }
            else if (st == RendererConfiguration::SHADING_TYPE_GOURAUD) {
                st = RendererConfiguration::SHADING_TYPE_PHONG;
            }
            else if (st == RendererConfiguration::SHADING_TYPE_PHONG) {
                st = RendererConfiguration::SHADING_TYPE_COOK_TERRANCE;
            }
            else {
                st = RendererConfiguration::SHADING_TYPE_NOLIGHT;
            }
            qualitySelection->setShadingType(st);
            updated = true;
            break;
        case KeyEvent::KEY_F8:
            qualitySelection->changeTexture();
            updated = true;
            break;
        case KeyEvent::KEY_F9:
            qualitySelection->changeBumpMap();
            updated = true;
            break;
        default:
            break;
    }

    return updated;
}

bool RendererConfigurationController::processKeyReleasedEvent(const KeyEvent&)
{
    return false;
}
