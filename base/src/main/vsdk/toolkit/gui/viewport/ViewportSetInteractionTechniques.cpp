#include "vsdk/toolkit/gui/viewport/Viewport.h"
#include "vsdk/toolkit/gui/viewport/ViewportSet.h"
#include "vsdk/toolkit/gui/viewport/ViewportSetInteractionListener.h"
#include "vsdk/toolkit/gui/viewport/ViewportSetInteractionTechniques.h"

ViewportSetInteractionTechniques::ViewportSetInteractionTechniques(
    ViewportSet* viewportSet)
    : viewportSet(viewportSet), listener(nullptr), titlePressArmed(false)
{
}

void ViewportSetInteractionTechniques::setListener(
    ViewportSetInteractionListener* listener)
{
    this->listener = listener;
}

ViewportSet* ViewportSetInteractionTechniques::getViewportSet() const
{
    return viewportSet;
}

bool ViewportSetInteractionTechniques::processCommand(
    const java::String& command)
{
    return processCommand(command, viewportSet->getSelectedViewport());
}

bool ViewportSetInteractionTechniques::processCommand(
    const java::String& command, Viewport* viewport)
{
    if ( viewport == nullptr ) {
        return false;
    }
    if ( viewport->selectProjectionLocation(command) ) return true;
    int mode = Viewport::renderModeForCommand(command);
    if ( mode == 0 || !viewportSet->isRenderModeAvailable(mode) ) return false;
    viewport->setRenderMode(mode);
    return true;
}

bool ViewportSetInteractionTechniques::processKeyPressedEvent(
    const KeyEvent& event)
{
    switch ( event.unicodeId ) {
      case ';':
        viewportSet->selectNextViewport();
        return true;
      case ',':
        viewportSet->selectNextLayoutStyle();
        return true;
      case 'w':
        if ( (event.modifierMask & KeyEvent::MASK_ALT) != 0 ) {
            viewportSet->toggleFullViewport();
            return true;
        }
        return false;
      default:
        return processSelectedViewportCommand(event);
    }
}

bool ViewportSetInteractionTechniques::processSelectedViewportCommand(
    const KeyEvent& event)
{
    Viewport* viewport = viewportSet->getSelectedViewport();

    if ( viewport == nullptr ) {
        return false;
    }

    if ( event.keycode == KeyEvent::KEY_9 ) {
        viewport->toggleRenderMode();
        return true;
    }
    if ( event.keycode == KeyEvent::KEY_NUM0 ) {
        viewport->cycleRequestedSize();
        return true;
    }

    switch ( event.unicodeId ) {
      case '.':
        viewport->toggleRenderMode();
        return true;
      case 'g':
        viewport->toggleGrid();
        return true;
      case 't':
        viewport->setActiveCamera(viewport->getTopCamera());
        return true;
      case 'l':
        viewport->setActiveCamera(viewport->getLeftCamera());
        return true;
      case 'f':
        viewport->setActiveCamera(viewport->getFrontCamera());
        return true;
      case 'b':
        viewport->setActiveCamera(viewport->getBottomCamera());
        return true;
      case 'p':
        viewport->setActiveCamera(viewport->getPerspectiveCamera());
        return true;
      case '0':
        viewport->cycleRequestedSize();
        return true;
      default:
        return false;
    }
}

Viewport* ViewportSetInteractionTechniques::findViewportAt(
    const MouseEvent& event) const
{
    return viewportSet->findViewportAt(event.getX(), event.getY());
}

bool ViewportSetInteractionTechniques::isPointerOverTitle(
    const MouseEvent& event) const
{
    Viewport* viewport = findViewportAt(event);

    return viewport != nullptr && isOverTitle(event, viewport);
}

bool ViewportSetInteractionTechniques::processMousePressedEvent(
    const MouseEvent& event)
{
    Viewport* viewport = findViewportAt(event);

    // Must be evaluated before the selection changes
    titlePressArmed = viewport != nullptr &&
        event.getButton() == MouseEvent::BUTTON1 &&
        viewportSet->isSelected(viewport) &&
        isOverTitle(event, viewport);

    return selectViewportUnderPointer(event);
}

bool ViewportSetInteractionTechniques::processMouseReleasedEvent(
    const MouseEvent& event)
{
    bool titleClick = titlePressArmed;
    Viewport* viewport = findViewportAt(event);

    titlePressArmed = false;
    bool selected = selectViewportUnderPointer(event);

    if ( titleClick && viewport != nullptr && listener != nullptr &&
         viewportSet->isSelected(viewport) && isOverTitle(event, viewport) ) {
        // Just below the title, aligned with it
        int x = viewport->getPixelStartX() + viewport->getTitleAreaStartX();
        int viewportTop = viewportSet->getSizeYInPixels() -
            (viewport->getPixelStartY() + viewport->getPixelSizeY());
        int y = viewportTop + viewport->getTitleAreaStartY() +
            viewport->getTitleAreaSizeY();
        listener->projectionLocationMenuRequested(viewport, x, y);
    }
    return selected;
}

bool ViewportSetInteractionTechniques::processMouseClickedEvent(
    const MouseEvent& event)
{
    titlePressArmed = false;
    return selectViewportUnderPointer(event);
}

bool ViewportSetInteractionTechniques::processMouseDraggedEvent(
    const MouseEvent& event)
{
    return selectViewportUnderPointer(event);
}

bool ViewportSetInteractionTechniques::selectViewportUnderPointer(
    const MouseEvent& event)
{
    Viewport* viewport = findViewportAt(event);

    if ( viewport == nullptr ) {
        return false;
    }
    return viewportSet->selectViewport(viewport);
}

bool ViewportSetInteractionTechniques::isOverTitle(
    const MouseEvent& event, const Viewport* viewport) const
{
    return viewport->isOverTitle(
        viewportSet->toViewportX(viewport, event.getX()),
        viewportSet->toViewportY(viewport, event.getY()));
}

MouseEvent ViewportSetInteractionTechniques::toViewportEvent(
    const MouseEvent& event, const Viewport* viewport) const
{
    MouseEvent translated;

    translated.setX(viewportSet->toViewportX(viewport, event.getX()));
    translated.setY(viewportSet->toViewportY(viewport, event.getY()));
    translated.setButton(event.getButton());
    translated.setModifiers(event.getModifiers());
    translated.setClicks(event.getClicks());
    return translated;
}
