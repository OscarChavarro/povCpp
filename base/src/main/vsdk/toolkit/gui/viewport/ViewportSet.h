#ifndef __VIEWPORT_SET__
#define __VIEWPORT_SET__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"

class Viewport;
class ViewportElementScaler;
class Widget;

/**
A `ViewportSet` models one rectangular drawing area of the application (for
example one canvas / one display / one screen) that is partitioned in one or
more `Viewport`s. The set knows which viewport is selected (the one receiving
interaction), how the viewports are arranged (layout style, or one viewport
maximized) and the size in pixels of the whole area.

An application can have several `ViewportSet`s, conceptually one for each
display available to the user (see `ApplicationModel`).

This class is a plain model object: it depends only on vitral-base classes,
and knows nothing about the GUI or rendering technology used to present it.
It owns its viewports; the I18N context and a scaler given with
`setElementScaler` are referenced, not owned.

The layout arrangements are the ones defined for 2, 3 and 4 viewports, plus a
last one for each of them where only the selected viewport is visible (see
`getLayoutStyleCount`); the percent-based area of each viewport has its origin
at the lower left corner of the set area. If the set has more viewports than
the supported layouts, only the first one is shown, maximized.
*/
class ViewportSet {
private:
    java::String name;
    java::ArrayList<Viewport*> viewports;
    int selectedViewportIndex;
    int layoutStyle;
    bool fullViewport;
    int sizeXInPixels;
    int sizeYInPixels;
    ColorRgb titleColor;
    ColorRgb selectedTitleColor;
    ViewportElementScaler* ownedElementScaler;
    ViewportElementScaler* elementScaler;
    Widget* i18nContext;
    java::ArrayList<int> availableRenderModes;

    java::String findMenuItemName(const java::String& popupName,
                                  const java::String& command) const;
    void applyLayout(const double layouts[][4][4], int layoutsCount);
    void showOnlySelectedViewport();
    void applyAreas(const double areas[][4], int n);
    void clampSelection();
    void coerceRenderMode(Viewport* viewport);

    ViewportSet(const ViewportSet& other);
    ViewportSet& operator=(const ViewportSet& other);

public:
    ViewportSet();
    virtual ~ViewportSet();

    /**
    Creates a set with the standard four viewports arrangement: Left,
    Perspective, Top and Front, with the perspective one selected.
    @param name name of the set
    @return a new set owned by the caller, with its layout already updated
    */
    static ViewportSet* createStandardSet(const java::String& name);

    const java::String& getName() const;
    void setName(const java::String& name);

    /**
    @return a read-only view of the viewports; use `addViewport` and
    `removeViewport` to change the list
    */
    const java::ArrayList<Viewport*>& getViewports() const;
    int getViewportCount() const;
    Viewport* getViewport(int index) const;

    /**
    Adds a viewport at the end of the set, taking its ownership, and updates
    the layout.
    */
    void addViewport(Viewport* viewport);

    java::ArrayList<int> getAvailableRenderModes() const;
    void setAvailableRenderModes(const java::ArrayList<int>& modes);
    void enforceAvailableRenderModes();
    bool isRenderModeAvailable(int renderMode) const;
    void cycleRenderMode(Viewport* viewport);

    /**
    Removes the viewport at the given index (if any) and updates the layout.
    @return the removed viewport, now owned by the caller, or null if the
    index is not valid
    */
    Viewport* removeViewport(int index);

    int getSelectedViewportIndex() const;
    void setSelectedViewportIndex(int selectedViewportIndex);

    /**
    @return the selected viewport, or null if the set is empty
    */
    Viewport* getSelectedViewport();
    bool isSelected(const Viewport* viewport);

    /**
    Makes the given viewport the selected one, if it belongs to this set.
    @return true if the selection was applied
    */
    bool selectViewport(const Viewport* viewport);

    /**
    Selects the next viewport, cyclically, and updates the layout (which, if
    a viewport is maximized, shows the new selected one).
    */
    void selectNextViewport();
    int getLayoutStyle() const;
    void setLayoutStyle(int layoutStyle);

    /**
    @return the number of layout styles available for the current number of
    viewports: the defined arrangements plus the one showing only the
    selected viewport (a set with a single viewport has just one)
    */
    int getLayoutStyleCount() const;

    /**
    Selects the next arrangement of the viewports and updates the layout.
    The last arrangement shows only the selected viewport.
    */
    void selectNextLayoutStyle();
    bool isFullViewport() const;
    void setFullViewport(bool fullViewport);

    /**
    Maximizes the selected viewport, or restores the layout if it is already
    maximized, and updates the layout.
    */
    void toggleFullViewport();

    /**
    @return the color used to draw the name of the viewports that are not
    selected (white by default)
    */
    const ColorRgb& getTitleColor() const;
    void setTitleColor(const ColorRgb& titleColor);

    /**
    @return the color used to draw the name of the selected viewport (yellow
    by default)
    */
    const ColorRgb& getSelectedTitleColor() const;
    void setSelectedTitleColor(const ColorRgb& selectedTitleColor);

    /**
    @return the I18N context (the GUI definition, in the language currently
    selected by the user) used to present the texts of this set, or null if
    there is none
    */
    Widget* getI18nContext() const;

    /**
    Sets the I18N context used to present the texts of this set. It must be
    updated whenever the user changes the language, so the set is presented
    with the messages of the new one. With a null context the default texts
    are used.
    */
    void setI18nContext(Widget* i18nContext);

    /**
    @param viewport a viewport of this set
    @return the name to present for the viewport: the text of its projection
    location in the `VIEWPORT_SET_PROJECTION_LOCATION` popup of the I18N
    context (falling back to the text of the command, and to the default name
    of the camera if the context does not define them)
    */
    java::String getTitleFor(const Viewport* viewport) const;

    /**
    @return the scaler that gives the size of the texts of this set for the
    resolution of the screen where the set is presented
    */
    ViewportElementScaler* getElementScaler() const;

    /**
    @param elementScaler the scaler for the elements of this set (not owned);
    a null value is ignored
    */
    void setElementScaler(ViewportElementScaler* elementScaler);

    /**
    @return the color to draw the name of the given viewport, depending on
    whether it is the selected one
    */
    const ColorRgb& getTitleColorFor(const Viewport* viewport);
    int getSizeXInPixels() const;
    void setSizeXInPixels(int sizeXInPixels);
    int getSizeYInPixels() const;
    void setSizeYInPixels(int sizeYInPixels);

    /**
    Sets the size in pixels of the area containing the viewports and updates
    the pixel area of each one.
    */
    void resize(int sizeXInPixels, int sizeYInPixels);

    /**
    Recalculates the pixel area of every viewport from the current size in
    pixels of the set.
    */
    void updatePixelAreas();
    int countActiveViewports() const;

    /**
    Finds the active viewport under a point given in pixels of the set area,
    with origin at the upper left corner. If several viewports contain the
    point (points over shared borders), the last one in the list is returned.
    @return the viewport under the point, or null if none
    */
    Viewport* findViewportAt(int x, int y) const;

    /**
    Translates a x coordinate given in pixels of the set area to pixels of the
    given viewport.
    @return x relative to the left of the viewport
    */
    int toViewportX(const Viewport* viewport, int x) const;

    /**
    Translates a y coordinate given in pixels of the set area (origin at the
    upper left corner) to pixels of the given viewport (same convention).
    @return y relative to the top of the viewport
    */
    int toViewportY(const Viewport* viewport, int y) const;

    /**
    Translates a x coordinate given in pixels of the given viewport to pixels
    of the set area. It is the inverse of `toViewportX`.
    @return x in the set area
    */
    int toSetX(const Viewport* viewport, int x) const;

    /**
    Translates a y coordinate given in pixels of the given viewport (origin
    at its upper left corner) to pixels of the set area (same convention). It
    is the inverse of `toViewportY`.
    @return y in the set area
    */
    int toSetY(const Viewport* viewport, int y) const;

    /**
    Updates the percent-based area and the active status of the viewports
    according to the selection, the maximized status and the layout style.
    */
    void updateLayout();
};

#endif
