#include "jackson/databind/JsonNode.h"
#include "jackson/databind/ObjectMapper.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferMenuReader.h"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetMenu.h"
#include "vsdk/toolkit/gui/widget/WidgetMenuItem.h"

static WidgetMenuElement *
readMenuElement(const jackson::databind::JsonNode &node, Widget &widget) {
    const jackson::databind::JsonNode *type = node.get("type");
    const jackson::databind::JsonNode *name = node.get("name");
    if (!type || !type->isString()) {
        return nullptr;
    }
    if (type->asText().equals("menu")) {
        WidgetMenu *menu = new WidgetMenu(&widget);
        if (name && name->isString()) {
            menu->setName(name->asText());
        }
        const jackson::databind::JsonNode *children = node.get("children");
        if (children && children->isArray()) {
            for (long i = 0; i < children->size(); i++) {
                const jackson::databind::JsonNode *child = children->get(i);
                WidgetMenuElement *element = child ? readMenuElement(*child, widget) : nullptr;
                if (element) {
                    menu->addChild(element);
                }
            }
        }
        return menu;
    }
    if (type->asText().equals("item")) {
        WidgetMenuItem *item = new WidgetMenuItem(&widget);
        if (name && name->isString()) {
            item->setName(name->asText());
        }
        const jackson::databind::JsonNode *modifiers = node.get("modifiers");
        if (modifiers && modifiers->isArray()) {
            for (long i = 0; i < modifiers->size(); i++) {
                const jackson::databind::JsonNode *modifier = modifiers->get(i);
                if (modifier && modifier->isString()) {
                    item->addModifier(modifier->asText());
                }
            }
        }
        return item;
    }
    return nullptr;
}

bool
FramebufferMenuReader::read(const java::String &json, Widget &widget) {
    if (widget.getMenubar()) {
        return false;
    }
    jackson::databind::ObjectMapper mapper;
    jackson::databind::JsonNode root = mapper.readTree(json);
    const jackson::databind::JsonNode *bar = root.get("menubar");
    if (!bar || !bar->isObject()) {
        return false;
    }
    const jackson::databind::JsonNode *type = bar->get("type");
    if (!type || !type->asText().equals("menu")) {
        return false;
    }
    widget.setMenubar(static_cast<WidgetMenu *>(readMenuElement(*bar, widget)));
    return widget.getMenubar() != nullptr;
}
