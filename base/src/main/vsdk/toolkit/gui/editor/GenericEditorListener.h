#ifndef __GENERIC_EDITOR_LISTENER__
#define __GENERIC_EDITOR_LISTENER__

class Entity;

/**
Receives notifications from a `GenericEditor` when the user changes a value
of the entity under edition, so the application can repaint or react.
*/
class GenericEditorListener {
public:
    virtual ~GenericEditorListener() {}

    /**
    Called after a new value has been validated and written to the entity.
    @param entity the entity that changed
    */
    virtual void notifyEntityChanged(Entity *entity) = 0;
};

#endif
