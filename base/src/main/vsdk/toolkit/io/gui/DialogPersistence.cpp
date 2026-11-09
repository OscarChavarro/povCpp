#include "java/lang/IllegalArgumentException.h"
#include "java/lang/String.h"
#include <cstring>
#include "java/util/TreeMap.txx"
#include "java/util/TreeSet.txx"
#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetDialog.h"
#include "vsdk/toolkit/gui/widget/WidgetDialogElement.h"
#include "vsdk/toolkit/io/gui/DialogPersistence.h"

namespace {

/**
JSON value, as read by `JsonReader`.
*/
struct JsonValue {
    enum Type { NUL, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT };

    Type type;
    bool booleanValue;
    double numberValue;
    java::String stringValue;
    java::ArrayList<JsonValue> items;
    java::TreeMap<java::String, JsonValue> members;

    JsonValue() : type(NUL), booleanValue(false), numberValue(0) {}

    /**
    @return the member with the name, or null if this is not an object or
    it has no such member
    */
    const JsonValue* get(const java::String& name) const
    {
        if ( type != OBJECT ) {
            return nullptr;
        }
        const JsonValue* member = members.get(name);
        return member != nullptr ? &(*member) : nullptr;
    }
};

/**
Recursive descent reader of a JSON document (RFC 8259).
*/
class JsonReader {
private:
    const java::String& text;
    size_t position;

    void fail(const char* problem) const
    {
        throw java::IllegalArgumentException(java::String("dialogs.json: ") + problem +
            " at offset " + java::String::valueOf(position));
    }

    void skipBlanks()
    {
        while ( position < text.size() &&
                (text[position] == ' ' || text[position] == '\t' ||
                 text[position] == '\n' || text[position] == '\r') ) {
            position++;
        }
    }

    bool consume(char expected)
    {
        skipBlanks();
        if ( position < text.size() && text[position] == expected ) {
            position++;
            return true;
        }
        return false;
    }

    void expect(char expected)
    {
        if ( !consume(expected) ) {
            fail((java::String("expected '") + expected + "'").c_str());
        }
    }

    bool consumeWord(const char* word)
    {
        if ( text.startsWith(word, (int)position) ) {
            position += std::strlen(word);
            return true;
        }
        return false;
    }

    static void appendUtf8(java::String& out, unsigned long code)
    {
        if ( code < 0x80 ) {
            out += static_cast<char>(code);
        }
        else if ( code < 0x800 ) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else if ( code < 0x10000 ) {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else {
            out += static_cast<char>(0xF0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    unsigned long readHex4()
    {
        if ( position + 4 > text.size() ) {
            fail("truncated unicode escape");
        }
        java::String digits = text.substr(position, 4);
        char* end = nullptr;
        unsigned long code = std::strtoul(digits.c_str(), &end, 16);
        if ( end != digits.c_str() + 4 ) {
            fail("bad unicode escape");
        }
        position += 4;
        return code;
    }

    java::String readString()
    {
        expect('"');
        java::String out;
        while ( true ) {
            if ( position >= text.size() ) {
                fail("unterminated string");
            }
            char c = text[position++];
            if ( c == '"' ) {
                return out;
            }
            if ( c != '\\' ) {
                out += c;
                continue;
            }
            if ( position >= text.size() ) {
                fail("unterminated string");
            }
            char escape = text[position++];
            switch ( escape ) {
              case '"': out += '"'; break;
              case '\\': out += '\\'; break;
              case '/': out += '/'; break;
              case 'b': out += '\b'; break;
              case 'f': out += '\f'; break;
              case 'n': out += '\n'; break;
              case 'r': out += '\r'; break;
              case 't': out += '\t'; break;
              case 'u': {
                unsigned long code = readHex4();
                // Surrogate pair
                if ( code >= 0xD800 && code <= 0xDBFF &&
                     consumeWord("\\u") ) {
                    unsigned long low = readHex4();
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                }
                appendUtf8(out, code);
                break;
              }
              default:
                fail("bad escape");
            }
        }
    }

    JsonValue readValue()
    {
        skipBlanks();
        if ( position >= text.size() ) {
            fail("unexpected end");
        }
        JsonValue value;
        char c = text[position];
        if ( c == '{' ) {
            position++;
            value.type = JsonValue::OBJECT;
            if ( consume('}') ) {
                return value;
            }
            do {
                skipBlanks();
                java::String name = readString();
                expect(':');
                value.members.put(name, readValue());
            } while ( consume(',') );
            expect('}');
        }
        else if ( c == '[' ) {
            position++;
            value.type = JsonValue::ARRAY;
            if ( consume(']') ) {
                return value;
            }
            do {
                value.items.add(readValue());
            } while ( consume(',') );
            expect(']');
        }
        else if ( c == '"' ) {
            value.type = JsonValue::STRING;
            value.stringValue = readString();
        }
        else if ( consumeWord("true") ) {
            value.type = JsonValue::BOOLEAN;
            value.booleanValue = true;
        }
        else if ( consumeWord("false") ) {
            value.type = JsonValue::BOOLEAN;
        }
        else if ( consumeWord("null") ) {
            value.type = JsonValue::NUL;
        }
        else {
            const char* start = text.c_str() + position;
            char* end = nullptr;
            value.numberValue = std::strtod(start, &end);
            if ( end == start ) {
                fail("unexpected character");
            }
            value.type = JsonValue::NUMBER;
            position += static_cast<size_t>(end - start);
        }
        return value;
    }

public:
    explicit JsonReader(const java::String& text) : text(text), position(0) {}

    JsonValue readDocument()
    {
        JsonValue value = readValue();
        skipBlanks();
        if ( position != text.size() ) {
            fail("unexpected content after the document");
        }
        return value;
    }
};

void invalid(const java::String& message)
{
    throw java::IllegalArgumentException(message);
}

/**
@return the text of a non blank string member, or an empty string (null in
Java)
*/
java::String optional(const JsonValue& node, const char* field)
{
    const JsonValue* value = node.get(field);
    if ( value == nullptr || value->type != JsonValue::STRING ||
         value->stringValue.trim().isEmpty() ) {
        return "";
    }
    return value->stringValue;
}

java::String required(const JsonValue& node, const char* field,
                     const java::String& path)
{
    java::String value = optional(node, field);
    if ( value.isEmpty() ) {
        invalid(path + ": missing " + field);
    }
    return value;
}

bool isOneOf(const java::String& kind, const char* const* kinds)
{
    for ( int i = 0; kinds[i] != nullptr; i++ ) {
        if ( kind == kinds[i] ) {
            return true;
        }
    }
    return false;
}

WidgetDialogElement element(const JsonValue& node, const java::String& path,
                            java::TreeSet<java::String>& ids)
{
    static const char* const KINDS[] = { "column", "row", "textInput",
        "button", "tree", "action", "fileChooser", "fileFilter", nullptr };
    static const char* const BOUND_KINDS[] = { "textInput", "tree",
        "fileFilter", nullptr };
    static const char* const ACTION_KINDS[] = { "button", "action", nullptr };

    java::String kind = required(node, "kind", path);
    if ( !isOneOf(kind, KINDS) ) {
        invalid(path + ": unsupported kind " + kind);
    }
    java::String id = optional(node, "id");
    if ( !id.isEmpty() && !ids.add(id) ) {
        invalid(path + ": duplicate element id " + id);
    }
    java::String binding = optional(node, "binding");
    java::String action = optional(node, "action");
    if ( isOneOf(kind, BOUND_KINDS) && binding.isEmpty() ) {
        invalid(path + ": missing binding");
    }
    if ( isOneOf(kind, ACTION_KINDS) && action.isEmpty() ) {
        invalid(path + ": missing action");
    }
    java::ArrayList<WidgetDialogElement> children;
    const JsonValue* childNodes = node.get("children");
    if ( childNodes != nullptr && childNodes->type == JsonValue::ARRAY ) {
        for ( size_t i = 0; i < childNodes->items.size(); i++ ) {
            children.add(element(childNodes->items[i],
                path + ".children[" + java::String::valueOf(i) + "]", ids));
        }
    }
    return WidgetDialogElement(kind.c_str(), id.c_str(),
                               optional(node, "labelKey").c_str(),
                               binding.c_str(), action.c_str(), children);
}

}

void DialogPersistence::importDialogs(const java::String& source,
                                      Widget* widget)
{
    java::String text(source.c_str());
    JsonValue root = JsonReader(text).readDocument();
    const JsonValue* version = root.get("schemaVersion");
    if ( version == nullptr || version->type != JsonValue::NUMBER ||
         version->numberValue != 1 ) {
        invalid("dialogs.json: unsupported schemaVersion");
    }
    const JsonValue* dialogs = root.get("dialogs");
    if ( dialogs == nullptr || dialogs->type != JsonValue::ARRAY ) {
        invalid("dialogs.json: dialogs must be an array");
    }

    java::TreeSet<java::String> ids;
    for ( size_t i = 0; i < dialogs->items.size(); i++ ) {
        java::String path = "dialogs[" + java::String::valueOf(i) + "]";
        const JsonValue& node = dialogs->items[i];
        java::String id = required(node, "id", path);
        if ( !ids.add(id) ||
             widget->getDialogById(id.c_str()) != nullptr ) {
            invalid(path + ": duplicate dialog id " + id);
        }
        java::String titleKey = required(node, "titleKey", path);
        const JsonValue* modal = node.get("modal");
        const JsonValue* content = node.get("content");
        if ( content == nullptr || content->type != JsonValue::ARRAY ) {
            invalid(path + ".content: expected array");
        }
        java::TreeSet<java::String> elementIds;
        java::ArrayList<WidgetDialogElement> elements;
        for ( size_t j = 0; j < content->items.size(); j++ ) {
            elements.add(element(content->items[j],
                path + ".content[" + java::String::valueOf(j) + "]", elementIds));
        }

        WidgetDialog* dialog = new WidgetDialog();
        dialog->setId(id.c_str());
        dialog->setTitleKey(titleKey.c_str());
        dialog->setModal(modal != nullptr &&
                         modal->type == JsonValue::BOOLEAN &&
                         modal->booleanValue);
        for ( size_t j = 0; j < elements.size(); j++ ) {
            dialog->addElement(elements[j]);
        }
        widget->addDialog(dialog);
    }
}
