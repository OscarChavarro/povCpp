#include "java/util/ArrayList.txx"
#include "jackson/databind/JsonNode.h"
namespace jackson {
namespace databind {

JsonNode::JsonNode()
    : type_(NULL_VALUE)
    , boolValue_(false)
    , numberValue_(0.0)
    , stringValue_("")
    , arrayValue_(nullptr)
    , objectValue_(nullptr) {}

JsonNode::JsonNode(const JsonNode& other)
    : type_(NULL_VALUE)
    , boolValue_(false)
    , numberValue_(0.0)
    , stringValue_("")
    , arrayValue_(nullptr)
    , objectValue_(nullptr) {
    copyFrom(other);
}

JsonNode&
JsonNode::operator=(const JsonNode& other) {
    if (this != &other) {
        copyFrom(other);
    }
    return *this;
}

JsonNode::~JsonNode() {
    disposeStorage();
}

JsonNode JsonNode::newNull() { return JsonNode(); }
JsonNode JsonNode::newBoolean(bool value) { JsonNode n; n.type_ = BOOLEAN; n.boolValue_ = value; return n; }
JsonNode JsonNode::newNumber(double value) { JsonNode n; n.type_ = NUMBER; n.numberValue_ = value; return n; }
JsonNode JsonNode::newString(const java::String& value) { JsonNode n; n.type_ = STRING; n.stringValue_ = value; return n; }
JsonNode JsonNode::newArray() { JsonNode n; n.type_ = ARRAY; return n; }
JsonNode JsonNode::newObject() { JsonNode n; n.type_ = OBJECT; return n; }

JsonNode::Type JsonNode::type() const { return type_; }
bool JsonNode::isNull() const { return type_ == NULL_VALUE; }
bool JsonNode::isBoolean() const { return type_ == BOOLEAN; }
bool JsonNode::isNumber() const { return type_ == NUMBER; }
bool JsonNode::isString() const { return type_ == STRING; }
bool JsonNode::isArray() const { return type_ == ARRAY; }
bool JsonNode::isObject() const { return type_ == OBJECT; }

bool JsonNode::asBoolean(bool defaultValue) const { return isBoolean() ? boolValue_ : defaultValue; }
double JsonNode::asDouble(double defaultValue) const { return isNumber() ? numberValue_ : defaultValue; }
int JsonNode::asInt(int defaultValue) const { return isNumber() ? static_cast<int>(numberValue_) : defaultValue; }
java::String JsonNode::asText(const java::String& defaultValue) const { return isString() ? stringValue_ : defaultValue; }

long JsonNode::size() const {
    if (isArray() && arrayValue_ != nullptr) return static_cast<long>(arrayValue_->size());
    if (isObject() && objectValue_ != nullptr) return static_cast<long>(objectValue_->size());
    return 0;
}

const JsonNode* JsonNode::get(long index) const {
    if (!isArray() || arrayValue_ == nullptr) return nullptr;
    if (index < 0 || index >= arrayValue_->size()) return nullptr;
    return &(*arrayValue_)[index];
}

const JsonNode* JsonNode::get(const java::String& key) const {
    if (!isObject() || objectValue_ == nullptr) return nullptr;
    return objectValue_->get(key);
}

void JsonNode::add(const JsonNode& value) {
    if (!isArray()) return;
    ensureArrayStorage()->add(value);
}

void JsonNode::put(const java::String& key, const JsonNode& value) {
    if (!isObject()) return;
    ensureObjectStorage()->put(key, value);
}

void
JsonNode::copyFrom(const JsonNode& other) {
    if (this == &other) {
        return;
    }

    disposeStorage();
    type_ = other.type_;
    boolValue_ = other.boolValue_;
    numberValue_ = other.numberValue_;
    stringValue_ = other.stringValue_;

    if (other.arrayValue_ != nullptr) {
        arrayValue_ = new ArrayStorage(*other.arrayValue_);
    }
    if (other.objectValue_ != nullptr) {
        objectValue_ = new ObjectStorage(*other.objectValue_);
    }
}

void
JsonNode::disposeStorage() {
    if (arrayValue_ != nullptr) {
        delete arrayValue_;
        arrayValue_ = nullptr;
    }
    if (objectValue_ != nullptr) {
        delete objectValue_;
        objectValue_ = nullptr;
    }
}

JsonNode::ArrayStorage*
JsonNode::ensureArrayStorage() {
    if (arrayValue_ == nullptr) {
        arrayValue_ = new ArrayStorage();
    }
    return arrayValue_;
}

JsonNode::ObjectStorage*
JsonNode::ensureObjectStorage() {
    if (objectValue_ == nullptr) {
        objectValue_ = new ObjectStorage();
    }
    return objectValue_;
}

}
}
