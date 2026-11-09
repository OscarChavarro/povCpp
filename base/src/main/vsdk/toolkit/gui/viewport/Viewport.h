#ifndef __VIEWPORT__
#define __VIEWPORT__

#include "java/lang/String.h"

class Camera;
class RendererConfiguration;

/**
A `Viewport` is one rectangular area of a `ViewportSet`, showing the scene
through one of its cameras. It is a plain model object: it knows nothing about
the GUI or rendering technology used to present it. It owns its cameras and
its renderer configuration.

The area occupied is specified in two ways:
  - As percentages of the containing `ViewportSet` area (`startXPercent`,
    `startYPercent`, `sizeXPercent`, `sizeYPercent`), assigned by the layout
    of the `ViewportSet`. Origin is at the lower left corner of the container.
  - As pixels (`pixelStartX`, `pixelStartY`, `pixelSizeX`, `pixelSizeY`),
    calculated from the percentages by `updatePixelArea`. A viewport can
    request an specific size in pixels. If this size gets smaller than the
    percent-based area, the viewport is assigned to match the requested size,
    centered inside the percent-based area. If a requested size dimension is
    greater than the percent-based area, the requested size is ignored. A
    requested size of 0 means the requested size matches the percent-based
    size. This is useful in applications willing to use just a subset of the
    area, for example when previewing slow raytracing visualizations.
*/
class Viewport {
public:
    static const int RENDER_MODE_Z_BUFFER = 1;
    static const int RENDER_MODE_RAYTRACING = 2;
    static const int RENDER_MODE_HIDDEN_LINES = 3;

private:
    int requestedSizeXInPixels;
    int requestedSizeYInPixels;
    Camera* activeCamera;
    Camera* perspectiveCamera;
    Camera* topCamera;
    Camera* bottomCamera;
    Camera* leftCamera;
    Camera* frontCamera;
    RendererConfiguration* rendererConfiguration;
    java::String title;
    int renderMode;
    bool showGrid;

    double startXPercent;
    double startYPercent;
    double sizeXPercent;
    double sizeYPercent;
    int border;
    bool active;

    int pixelStartX;
    int pixelStartY;
    int pixelSizeX;
    int pixelSizeY;

    int titleAreaStartX;
    int titleAreaStartY;
    int titleAreaSizeX;
    int titleAreaSizeY;

    Viewport(const Viewport& other);
    Viewport& operator=(const Viewport& other);

public:
    Viewport();
    virtual ~Viewport();

    int getRequestedSizeXInPixels() const;
    void setRequestedSizeXInPixels(int requestedSizeXInPixels);
    int getRequestedSizeYInPixels() const;
    void setRequestedSizeYInPixels(int requestedSizeYInPixels);
    Camera* getActiveCamera() const;

    /**
    @param activeCamera one of the cameras of this viewport
    */
    void setActiveCamera(Camera* activeCamera);
    Camera* getPerspectiveCamera() const;
    Camera* getTopCamera() const;
    Camera* getBottomCamera() const;
    Camera* getLeftCamera() const;
    Camera* getFrontCamera() const;
    RendererConfiguration* getRendererConfiguration() const;
    const java::String& getTitle() const;
    int getRenderMode() const;
    void setRenderMode(int renderMode);
    bool isShowGrid() const;
    void setShowGrid(bool showGrid);
    void toggleGrid();
    void cycleRequestedSize();
    void toggleRenderMode();
    void updateCameraViewports(int width, int height);
    double getStartXPercent() const;
    void setStartXPercent(double startXPercent);
    double getStartYPercent() const;
    void setStartYPercent(double startYPercent);
    double getSizeXPercent() const;
    void setSizeXPercent(double sizeXPercent);
    double getSizeYPercent() const;
    void setSizeYPercent(double sizeYPercent);

    /**
    @return the border width, in pixels, left around the projection area
    */
    int getBorder() const;
    void setBorder(int border);

    /**
    An active viewport is visible and should be rendered. An inactive one is
    hidden (for example when another viewport is maximized).
    @return true if this viewport is visible
    */
    bool isActive() const;
    void setActive(bool active);
    int getPixelStartX() const;
    int getPixelStartY() const;
    int getPixelSizeX() const;
    int getPixelSizeY() const;

    /**
    Informs the area, in pixels, where the title of this viewport is
    presented. Only who presents the title knows its real size, so it must be
    informed each time the title is drawn. Coordinates are relative to the
    viewport, with origin at its upper left corner. An empty area (the
    initial value) means the title has not been presented.
    */
    void setTitleArea(int startX, int startY, int sizeX, int sizeY);
    int getTitleAreaStartX() const;
    int getTitleAreaStartY() const;
    int getTitleAreaSizeX() const;
    int getTitleAreaSizeY() const;

    /**
    @param x coordinate relative to the viewport, origin at its upper left
    corner
    @param y coordinate relative to the viewport, origin at its upper left
    corner
    @return true if the point is over the area where the title is presented
    */
    bool isOverTitle(int x, int y) const;

    /**
    Assigns the percent-based area of this viewport inside its container.
    */
    void setPercentArea(double startXPercent, double startYPercent,
                        double sizeXPercent, double sizeYPercent);

    /**
    Given a point (x, y) in the percent space of the container (origin at
    lower left corner), determines if the point is inside this viewport.
    @return true if the point is inside the percent-based area
    */
    bool contains(double x, double y) const;

    /**
    @return true if no specific size in pixels is requested, so the viewport
    uses all the area assigned to it
    */
    bool useFullContainerArea() const;

    /**
    A container area has valid pixel coordinates from (0, 0) to
    (containerXSize-1, containerYSize-1). This method calculates the pixel
    area of this viewport inside that container from its percent-based area,
    its border and its requested size, and updates the cameras accordingly.
    */
    void updatePixelArea(int containerXSize, int containerYSize);

    /**
    @return the standard command (see `ViewportSetCommands`) that selects the
    projection location currently used by this viewport
    */
    java::String getProjectionLocationCommand() const;

    /**
    @return the standard command of the render mode popup that corresponds to
    the render mode of this viewport (see `ViewportSetCommands`)
    */
    java::String getRenderModeCommand() const;
    static java::String commandForRenderMode(int renderMode);
    static int renderModeForCommand(const java::String& command);

    /**
    Selects the render mode given by one of the standard commands of the
    render mode popup (see `ViewportSetCommands`): GPU is the z-buffer of the
    graphics API, CPU is raytracing.
    @return true if the command was a render mode one and was applied
    */
    bool selectRenderMode(const java::String& command);

    /**
    Selects the projection location given by one of the standard commands of
    the projection location popup (see `ViewportSetCommands`).
    @return true if the command was a projection location one and was applied
    */
    bool selectProjectionLocation(const java::String& command);

    /**
    Selects a default camera and rendering configuration for this viewport,
    depending on its position in a set of `numViews` viewports.
    With four or more viewports, the standard arrangement is: Left (wires),
    Perspective, Top (wires) and Front (wires).
    @param numViews number of viewports in the set
    @param id position of this viewport in the set, starting at 0
    */
    void applyDefaultConfiguration(int numViews, int id);
};

#endif
