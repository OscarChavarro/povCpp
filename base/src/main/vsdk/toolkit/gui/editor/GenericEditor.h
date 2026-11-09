#ifndef __GENERIC_EDITOR__
#define __GENERIC_EDITOR__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/EntityListener.h"

class ControlSpecification;
class Entity;
class EntityControlAccessor;
class GenericEditorListener;

/**
Builds an editor for any `Entity` from its control specifications (see
`Entity::getControlSpecifications()` and `ControlSpecification`), without
code specific to the entity class.

This class holds all the logic independent of GUI technology: it parses the
specifications, reads and writes values through the entity accessors (the
pointers to member functions registered with
`Entity::addControlSpecification`, the C++ replacement of Java reflection),
validates the intervals and orchestrates the construction. Each GUI
technology (Xt, Qt, Web...) provides a subclass that only implements the
presentation hooks: `beginBuild`, `addControl`, `endBuild`,
`showValidationMessage`, `setControlValue` and `clearControls`.

The editor subscribes to the entity under edition: when the entity emits
`UPDATED` the controls are refreshed, and when it emits `DELETED` the
controls are removed and an "Entity deleted" message is shown.
*/
class GenericEditor : public EntityListener {
private:
    GenericEditorListener *listener;
    bool entityDeleted;
    /// Class of the entity under edition, known while it is fully alive
    java::String entityClassName;

    GenericEditor(const GenericEditor &other);
    GenericEditor &operator=(const GenericEditor &other);

    void clearSpecifications();
    const EntityControlAccessor *findGetter(
        const ControlSpecification &specification) const;
    const EntityControlAccessor *findSetter(
        const ControlSpecification &specification) const;
    void reportMissingAccessor(const ControlSpecification &specification,
                               const java::String &accessor) const;

protected:
    Entity *entity;
    /// Specifications with a control, in presentation order (owned)
    java::ArrayList<ControlSpecification *> specifications;

    GenericEditor();

    /**
    Starts the presentation of a new editor, discarding any previous one.
    @param title name of the edited entity class
    */
    virtual void beginBuild(const java::String &title) = 0;

    /**
    Adds the control for one attribute. When the user confirms a new value,
    the subclass must call `updateValue(specification, text)`.
    @param specification the attribute presented by the control (owned by
    the editor, valid until the next `build` or `detach`)
    @param value current value as text
    */
    virtual void addControl(const ControlSpecification *specification,
                            const java::String &value) = 0;

    /**
    Finishes the presentation of the editor.
    */
    virtual void endBuild() = 0;

    /**
    Presents a message to the user about the last edition.
    @param message message to show, or null to clear it
    */
    virtual void showValidationMessage(const char *message) = 0;

    /**
    Shows a new value in the control of one attribute, after the entity
    emitted `UPDATED`.
    @param specification the attribute whose control must change
    @param value current value as text
    */
    virtual void setControlValue(const ControlSpecification *specification,
                                 const java::String &value) = 0;

    /**
    Removes all the controls and presents only a message, used when the
    entity under edition is deleted.
    @param message message to show in place of the controls
    */
    virtual void clearControls(const java::String &message) = 0;

public:
    /**
    Stops listening to the entity under edition.
    */
    virtual ~GenericEditor();

    /**
    @param listener object notified after each accepted change (referenced,
    not owned), may be null
    */
    void setListener(GenericEditorListener *listener);

    /**
    Builds the editor for the given entity: one control per supported control
    specification, or a message when there is nothing to edit.
    @param entity the entity to edit
    */
    void build(Entity *entity);

    /**
    Stops listening to the entity under edition, if any. Call it when the
    editor is hidden or reused for another entity.
    */
    void detach();

    /**
    @return true if the last entity under edition emitted `DELETED` and the
    editor is showing the "Entity deleted" message
    */
    bool isEntityDeleted() const;

    /**
    Reacts to the events of the entity under edition.
    @param event the event emitted by the entity
    */
    virtual void notifyEntityEvent(const EntityEvent &event) override;

    /**
    @return the entity under edition, or null
    */
    Entity *getEntity() const;

    /**
    Reads the current value of an attribute through its getter.
    @param specification the attribute to read
    @param outValue the value as text
    @return false if the entity has no usable getter
    */
    bool readValue(const ControlSpecification &specification,
                   java::String &outValue) const;

    /**
    Validates a value typed by the user and, if valid, writes it to the
    entity through its setter and notifies the listener. On failure, the
    reason is presented with `showValidationMessage`.
    @param specification the attribute to change
    @param text the new value as typed by the user
    @return true if the value was accepted and written
    */
    bool updateValue(const ControlSpecification *specification,
                     const java::String &text);

    /**
    @param entity any entity
    @return the name of the class of the entity, without namespaces (as
    Java `getClass().getSimpleName()`)
    */
    static java::String getSimpleClassName(const Entity &entity);
};

#endif
