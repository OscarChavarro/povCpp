#include <cmath>

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/gui/viewport/Viewport.h"
#include "vsdk/toolkit/gui/viewport/ViewportSetCommands.h"

namespace {

double toRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

}

Viewport::Viewport()
{
    Matrix4x4d r;

    requestedSizeXInPixels = 0;
    requestedSizeYInPixels = 0;

    perspectiveCamera = new Camera();
    perspectiveCamera->setPosition(Vector3Dd(-5, -5, 5));
    r = r.eulerAnglesRotation(toRadians(45), toRadians(-35), 0);
    perspectiveCamera->setRotation(r);
    perspectiveCamera->setName("Perspective");

    topCamera = new Camera();
    topCamera->setProjectionMode(Camera::PROJECTION_MODE_ORTHOGONAL);
    topCamera->setPosition(Vector3Dd(0, 0, 5));
    r = r.eulerAnglesRotation(toRadians(90), toRadians(-90), 0);
    topCamera->setRotation(r);
    topCamera->setOrthogonalZoom(0.25);
    topCamera->setName("Top");

    bottomCamera = new Camera();
    bottomCamera->setProjectionMode(Camera::PROJECTION_MODE_ORTHOGONAL);
    bottomCamera->setPosition(Vector3Dd(0, 0, -5));
    r = r.eulerAnglesRotation(toRadians(90), toRadians(90), 0);
    bottomCamera->setRotation(r);
    bottomCamera->setOrthogonalZoom(0.25);
    bottomCamera->setName("Bottom");

    leftCamera = new Camera();
    leftCamera->setProjectionMode(Camera::PROJECTION_MODE_ORTHOGONAL);
    leftCamera->setPosition(Vector3Dd(-5, 0, 0));
    r = r.identity();
    leftCamera->setRotation(r);
    leftCamera->setOrthogonalZoom(0.25);
    leftCamera->setName("Left");

    frontCamera = new Camera();
    frontCamera->setProjectionMode(Camera::PROJECTION_MODE_ORTHOGONAL);
    frontCamera->setPosition(Vector3Dd(0, -5, 0));
    r = r.eulerAnglesRotation(toRadians(90), 0, 0);
    frontCamera->setRotation(r);
    frontCamera->setOrthogonalZoom(0.25);
    frontCamera->setName("Front");

    activeCamera = perspectiveCamera;
    rendererConfiguration = new RendererConfiguration();
    // A new viewport starts with the perspective camera, whose surfaces
    // are shaded with Phong instead of the Gouraud default
    rendererConfiguration->setShadingType(
        RendererConfiguration::SHADING_TYPE_PHONG);
    title = activeCamera->getName();
    renderMode = RENDER_MODE_Z_BUFFER;
    showGrid = true;

    startXPercent = 0.0;
    startYPercent = 0.0;
    sizeXPercent = 1.0;
    sizeYPercent = 1.0;
    border = 2;
    active = true;

    pixelStartX = 0;
    pixelStartY = 0;
    pixelSizeX = 0;
    pixelSizeY = 0;

    titleAreaStartX = 0;
    titleAreaStartY = 0;
    titleAreaSizeX = 0;
    titleAreaSizeY = 0;
}

Viewport::~Viewport()
{
    delete perspectiveCamera;
    delete topCamera;
    delete bottomCamera;
    delete leftCamera;
    delete frontCamera;
    delete rendererConfiguration;
}

int Viewport::getRequestedSizeXInPixels() const
{
    return requestedSizeXInPixels;
}

void Viewport::setRequestedSizeXInPixels(int requestedSizeXInPixels)
{
    this->requestedSizeXInPixels = requestedSizeXInPixels;
}

int Viewport::getRequestedSizeYInPixels() const
{
    return requestedSizeYInPixels;
}

void Viewport::setRequestedSizeYInPixels(int requestedSizeYInPixels)
{
    this->requestedSizeYInPixels = requestedSizeYInPixels;
}

Camera* Viewport::getActiveCamera() const
{
    return activeCamera;
}

void Viewport::setActiveCamera(Camera* activeCamera)
{
    if ( activeCamera == nullptr ) {
        return;
    }
    this->activeCamera = activeCamera;
    title = activeCamera->getName();
}

Camera* Viewport::getPerspectiveCamera() const
{
    return perspectiveCamera;
}

Camera* Viewport::getTopCamera() const
{
    return topCamera;
}

Camera* Viewport::getBottomCamera() const
{
    return bottomCamera;
}

Camera* Viewport::getLeftCamera() const
{
    return leftCamera;
}

Camera* Viewport::getFrontCamera() const
{
    return frontCamera;
}

RendererConfiguration* Viewport::getRendererConfiguration() const
{
    return rendererConfiguration;
}

const java::String& Viewport::getTitle() const
{
    return title;
}

int Viewport::getRenderMode() const
{
    return renderMode;
}

void Viewport::setRenderMode(int renderMode)
{
    this->renderMode = renderMode;
}

bool Viewport::isShowGrid() const
{
    return showGrid;
}

void Viewport::setShowGrid(bool showGrid)
{
    this->showGrid = showGrid;
}

void Viewport::toggleGrid()
{
    showGrid = !showGrid;
}

void Viewport::cycleRequestedSize()
{
    switch ( requestedSizeXInPixels ) {
      case 0:
        requestedSizeXInPixels = 320;
        requestedSizeYInPixels = 240;
        break;
      case 320:
        requestedSizeXInPixels = 640;
        requestedSizeYInPixels = 480;
        break;
      case 640:
        requestedSizeXInPixels = 800;
        requestedSizeYInPixels = 600;
        break;
      case 800:
      default:
        requestedSizeXInPixels = 0;
        requestedSizeYInPixels = 0;
        break;
    }
}

void Viewport::toggleRenderMode()
{
    if ( renderMode == RENDER_MODE_Z_BUFFER ) {
        renderMode = RENDER_MODE_RAYTRACING;
    }
    else {
        renderMode = RENDER_MODE_Z_BUFFER;
    }
}

void Viewport::updateCameraViewports(int width, int height)
{
    perspectiveCamera->updateViewportResize(width, height);
    topCamera->updateViewportResize(width, height);
    bottomCamera->updateViewportResize(width, height);
    leftCamera->updateViewportResize(width, height);
    frontCamera->updateViewportResize(width, height);
}

double Viewport::getStartXPercent() const
{
    return startXPercent;
}

void Viewport::setStartXPercent(double startXPercent)
{
    this->startXPercent = startXPercent;
}

double Viewport::getStartYPercent() const
{
    return startYPercent;
}

void Viewport::setStartYPercent(double startYPercent)
{
    this->startYPercent = startYPercent;
}

double Viewport::getSizeXPercent() const
{
    return sizeXPercent;
}

void Viewport::setSizeXPercent(double sizeXPercent)
{
    this->sizeXPercent = sizeXPercent;
}

double Viewport::getSizeYPercent() const
{
    return sizeYPercent;
}

void Viewport::setSizeYPercent(double sizeYPercent)
{
    this->sizeYPercent = sizeYPercent;
}

int Viewport::getBorder() const
{
    return border;
}

void Viewport::setBorder(int border)
{
    this->border = border;
}

bool Viewport::isActive() const
{
    return active;
}

void Viewport::setActive(bool active)
{
    this->active = active;
}

int Viewport::getPixelStartX() const
{
    return pixelStartX;
}

int Viewport::getPixelStartY() const
{
    return pixelStartY;
}

int Viewport::getPixelSizeX() const
{
    return pixelSizeX;
}

int Viewport::getPixelSizeY() const
{
    return pixelSizeY;
}

void Viewport::setTitleArea(int startX, int startY, int sizeX, int sizeY)
{
    titleAreaStartX = startX;
    titleAreaStartY = startY;
    titleAreaSizeX = sizeX;
    titleAreaSizeY = sizeY;
}

int Viewport::getTitleAreaStartX() const
{
    return titleAreaStartX;
}

int Viewport::getTitleAreaStartY() const
{
    return titleAreaStartY;
}

int Viewport::getTitleAreaSizeX() const
{
    return titleAreaSizeX;
}

int Viewport::getTitleAreaSizeY() const
{
    return titleAreaSizeY;
}

bool Viewport::isOverTitle(int x, int y) const
{
    return x >= titleAreaStartX && x < titleAreaStartX + titleAreaSizeX &&
           y >= titleAreaStartY && y < titleAreaStartY + titleAreaSizeY;
}

void Viewport::setPercentArea(double startXPercent, double startYPercent,
                              double sizeXPercent, double sizeYPercent)
{
    this->startXPercent = startXPercent;
    this->startYPercent = startYPercent;
    this->sizeXPercent = sizeXPercent;
    this->sizeYPercent = sizeYPercent;
}

bool Viewport::contains(double x, double y) const
{
    return x >= startXPercent && x <= startXPercent + sizeXPercent &&
           y >= startYPercent && y <= startYPercent + sizeYPercent;
}

bool Viewport::useFullContainerArea() const
{
    return requestedSizeXInPixels == 0 || requestedSizeYInPixels == 0;
}

void Viewport::updatePixelArea(int containerXSize, int containerYSize)
{
    int w;
    int h;
    int subAreaXSize;
    int subAreaYSize;

    pixelStartX = (int)(startXPercent * ((double)containerXSize)) + border + 1;
    pixelStartY = (int)(startYPercent * ((double)containerYSize)) + border + 1;
    subAreaXSize = (int)(sizeXPercent * ((double)containerXSize)) - 2*border - 2;
    subAreaYSize = (int)(sizeYPercent * ((double)containerYSize)) - 2*border - 2;
    if ( useFullContainerArea() ) {
        pixelSizeX = subAreaXSize;
        pixelSizeY = subAreaYSize;
    }
    else {
        if ( requestedSizeXInPixels < subAreaXSize ) {
            w = requestedSizeXInPixels;
        }
        else {
            w = subAreaXSize;
        }
        if ( requestedSizeYInPixels < subAreaYSize ) {
            h = requestedSizeYInPixels;
        }
        else {
            h = subAreaYSize;
        }
        pixelStartX += (subAreaXSize - w) / 2;
        pixelStartY += (subAreaYSize - h) / 2;
        pixelSizeX = w;
        pixelSizeY = h;
    }
    updateCameraViewports(pixelSizeX, pixelSizeY);
}

java::String Viewport::getProjectionLocationCommand() const
{
    if ( activeCamera == topCamera ) {
        return ViewportSetCommands::IDV_PROJECTION_LOCATION_TOP;
    }
    else if ( activeCamera == bottomCamera ) {
        return ViewportSetCommands::IDV_PROJECTION_LOCATION_BOTTOM;
    }
    else if ( activeCamera == leftCamera ) {
        return ViewportSetCommands::IDV_PROJECTION_LOCATION_LEFT;
    }
    else if ( activeCamera == frontCamera ) {
        return ViewportSetCommands::IDV_PROJECTION_LOCATION_FRONT;
    }
    return ViewportSetCommands::IDV_PROJECTION_LOCATION_PERSPECTIVE;
}

java::String Viewport::getRenderModeCommand() const
{
    return commandForRenderMode(renderMode);
}

java::String Viewport::commandForRenderMode(int renderMode)
{
    if ( renderMode == RENDER_MODE_RAYTRACING ) {
        return ViewportSetCommands::IDV_RENDER_MODE_CPU;
    }
    if ( renderMode == RENDER_MODE_HIDDEN_LINES ) {
        return ViewportSetCommands::IDV_RENDER_MODE_CPU_HIDDEN_LINES;
    }
    return ViewportSetCommands::IDV_RENDER_MODE_GPU;
}

int Viewport::renderModeForCommand(const java::String& command)
{
    if ( command.equals(ViewportSetCommands::IDV_RENDER_MODE_GPU) ) return RENDER_MODE_Z_BUFFER;
    if ( command.equals(ViewportSetCommands::IDV_RENDER_MODE_CPU) ) return RENDER_MODE_RAYTRACING;
    if ( command.equals(ViewportSetCommands::IDV_RENDER_MODE_CPU_HIDDEN_LINES) ) return RENDER_MODE_HIDDEN_LINES;
    return 0;
}

bool Viewport::selectRenderMode(const java::String& command)
{
    int mode = renderModeForCommand(command);
    if ( mode == 0 ) return false;
    renderMode = mode;
    return true;
}

bool Viewport::selectProjectionLocation(const java::String& command)
{
    if ( command.equals(ViewportSetCommands::IDV_PROJECTION_LOCATION_PERSPECTIVE) ) {
        setActiveCamera(perspectiveCamera);
        return true;
    }
    if ( command.equals(ViewportSetCommands::IDV_PROJECTION_LOCATION_TOP) ) {
        setActiveCamera(topCamera);
        return true;
    }
    if ( command.equals(ViewportSetCommands::IDV_PROJECTION_LOCATION_BOTTOM) ) {
        setActiveCamera(bottomCamera);
        return true;
    }
    if ( command.equals(ViewportSetCommands::IDV_PROJECTION_LOCATION_LEFT) ) {
        setActiveCamera(leftCamera);
        return true;
    }
    if ( command.equals(ViewportSetCommands::IDV_PROJECTION_LOCATION_FRONT) ) {
        setActiveCamera(frontCamera);
        return true;
    }
    return false;
}

void Viewport::applyDefaultConfiguration(int numViews, int id)
{
    if ( numViews < 4 ) {
        return;
    }
    switch ( id ) {
      case 0:
        setActiveCamera(leftCamera);
        rendererConfiguration->setSurfaces(false);
        rendererConfiguration->setWires(true);
        break;
      case 1:
        setActiveCamera(perspectiveCamera);
        break;
      case 2:
        setActiveCamera(topCamera);
        rendererConfiguration->setSurfaces(false);
        rendererConfiguration->setWires(true);
        break;
      case 3: default:
        setActiveCamera(frontCamera);
        rendererConfiguration->setSurfaces(false);
        rendererConfiguration->setWires(true);
        break;
    }
}
