#include "java/lang/String.h"

#include "java/lang/Double.h"
#include "java/lang/Float.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/NumberFormatException.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/EntityControlAccessor.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/gui/editor/ControlSpecification.h"
#include "vsdk/toolkit/gui/editor/GenericEditor.h"
#include "vsdk/toolkit/gui/editor/GenericEditorListener.h"

namespace {

bool typeIsSupported(const java::String &type)
{
    return type == "double" || type == "float" || type == "int" ||
        type == "long";
}

/**
@return the value of the text for a supported type, as a double
@throws java::NumberFormatException if the text is not a value of the type
*/
double parseValue(const java::String &type, const java::String &text)
{
    if ( type == "double" ) {
        return java::Double::parseDouble(text);
    }
    if ( type == "float" ) {
        return java::Float::parseFloat(text);
    }
    if ( type == "int" ) {
        return java::Integer::parseInt(text);
    }
    if ( type == "long" ) {
        return (double)java::Long::parseLong(text);
    }
    throw java::NumberFormatException(java::String("Unsupported type ") + type);
}

}

GenericEditor::GenericEditor() :
    listener(nullptr), entityDeleted(false), entity(nullptr)
{
}

GenericEditor::~GenericEditor()
{
    detach();
    clearSpecifications();
}

void
GenericEditor::setListener(GenericEditorListener *listener)
{
    this->listener = listener;
}

void
GenericEditor::clearSpecifications()
{
    for ( long int i = 0; i < specifications.size(); i++ ) {
        delete specifications.get(i);
    }
    specifications.clear();
}

void
GenericEditor::build(Entity *entity)
{
    detach();
    this->entity = entity;
    clearSpecifications();

    if ( entity == nullptr ) {
        Logger::reportMessage("GenericEditor", Logger::WARNING,
            "GenericEditor.build",
            "Null entity received, building an empty editor.");
        beginBuild("");
        showValidationMessage("No object to edit.");
        endBuild();
        return;
    }

    entity->addEntityListener(this);
    entityClassName = getSimpleClassName(*entity);
    beginBuild(entityClassName);

    java::ArrayList<java::String> &texts = entity->getControlSpecifications();
    for ( long int i = 0; i < texts.size(); i++ ) {
        const java::String &text = texts.get(i);
        ControlSpecification *specification = ControlSpecification::parse(text);
        if ( specification == nullptr ) {
            continue;
        }
        if ( !typeIsSupported(specification->getType()) ) {
            Logger::reportMessage("GenericEditor", Logger::WARNING,
                "GenericEditor.build",
                java::String("Unsupported type \"") +
                specification->getType() +
                "\" in control specification \"" + text + "\" of class " +
                entityClassName + ", control skipped.");
            delete specification;
            continue;
        }
        java::String value;
        if ( !readValue(*specification, value) ) {
            delete specification;
            continue;
        }
        specifications.add(specification);
        addControl(specification, value);
    }

    if ( specifications.size() == 0 ) {
        showValidationMessage((java::String("No editable attributes for ") +
            entityClassName).c_str());
    }
    else {
        showValidationMessage(nullptr);
    }
    endBuild();
}

void
GenericEditor::detach()
{
    if ( entity != nullptr ) {
        entity->removeEntityListener(this);
    }
    entity = nullptr;
    entityDeleted = false;
}

bool
GenericEditor::isEntityDeleted() const
{
    return entityDeleted;
}

void
GenericEditor::notifyEntityEvent(const EntityEvent &event)
{
    if ( event.getSource() != entity || entity == nullptr ) {
        return;
    }
    if ( event.getType() == EntityEvent::DELETED ) {
        // Also emitted by the destructor of the entity, when only its
        // Entity part is alive: the class name is the one known at build
        java::String className = entityClassName;
        entity->removeEntityListener(this);
        entity = nullptr;
        clearSpecifications();
        entityDeleted = true;
        clearControls(java::String("Entity deleted (") + className + ").");
    }
    else if ( event.getType() == EntityEvent::UPDATED ) {
        for ( long int i = 0; i < specifications.size(); i++ ) {
            java::String value;
            if ( readValue(*specifications.get(i), value) ) {
                setControlValue(specifications.get(i), value);
            }
        }
    }
}

Entity *
GenericEditor::getEntity() const
{
    return entity;
}

bool
GenericEditor::readValue(const ControlSpecification &specification,
                         java::String &outValue) const
{
    const EntityControlAccessor *getter = findGetter(specification);
    if ( getter == nullptr ) {
        return false;
    }
    if ( !getter->readValue(*entity, outValue) ) {
        Logger::reportMessage("GenericEditor", Logger::WARNING,
            "GenericEditor.readValue",
            java::String("Cannot call get") + specification.getLabel() +
            "() on an object of class " + entityClassName + ".");
        return false;
    }
    return true;
}

bool
GenericEditor::updateValue(const ControlSpecification *specification,
                           const java::String &text)
{
    if ( entity == nullptr || specification == nullptr ) {
        return false;
    }

    java::String trimmed = text.trim();

    double numeric;
    try {
        numeric = parseValue(specification->getType(), trimmed);
    }
    catch ( const java::NumberFormatException & ) {
        showValidationMessage((specification->getLabel() + ": \"" + text +
            "\" is not a valid " + specification->getType() + ".").c_str());
        return false;
    }

    if ( !specification->contains(numeric) ) {
        showValidationMessage((specification->getLabel() + " must be in " +
            specification->getIntervalText() + ".").c_str());
        return false;
    }

    const EntityControlAccessor *setter = findSetter(*specification);
    if ( setter == nullptr ) {
        showValidationMessage((specification->getLabel() +
            " cannot be changed.").c_str());
        return false;
    }
    if ( !setter->writeValue(*entity, trimmed) ) {
        Logger::reportMessage("GenericEditor", Logger::WARNING,
            "GenericEditor.updateValue",
            java::String("Cannot call set") + specification->getLabel() +
            " on an object of class " + entityClassName + ".");
        showValidationMessage((specification->getLabel() +
            " could not be changed.").c_str());
        return false;
    }

    showValidationMessage(nullptr);
    Entity *changed = entity;
    changed->update();
    if ( listener != nullptr ) {
        listener->notifyEntityChanged(changed);
    }
    return true;
}

const EntityControlAccessor *
GenericEditor::findGetter(const ControlSpecification &specification) const
{
    java::String methodName = java::String("get") + specification.getLabel();
    const EntityControlAccessor *accessor =
        entity->getControlAccessor(specification.getName());
    if ( accessor == nullptr ) {
        reportMissingAccessor(specification, methodName + "()");
        return nullptr;
    }
    if ( specification.getType() != accessor->getType() ) {
        reportMissingAccessor(specification, methodName + "() returning " +
            specification.getType());
        return nullptr;
    }
    return accessor;
}

const EntityControlAccessor *
GenericEditor::findSetter(const ControlSpecification &specification) const
{
    java::String methodName = java::String("set") + specification.getLabel();
    const EntityControlAccessor *accessor =
        entity->getControlAccessor(specification.getName());
    if ( accessor == nullptr ||
         specification.getType() != accessor->getType() ) {
        reportMissingAccessor(specification, methodName + "(" +
            specification.getType() + ")");
        return nullptr;
    }
    return accessor;
}

void
GenericEditor::reportMissingAccessor(const ControlSpecification &specification,
                                     const java::String &accessor) const
{
    Logger::reportMessage("GenericEditor", Logger::WARNING, "GenericEditor",
        java::String("Class ") + entityClassName +
        " declares control \"" + specification.getName() +
        "\" but registers no accessor " + accessor +
        "; register it with addControlSpecification or fix the control " +
        "specification.");
}

java::String
GenericEditor::getSimpleClassName(const Entity &entity)
{
    return entity.getClassSimpleName();
}
