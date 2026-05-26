#include "io/pov/antlr/AntlrParseTreeToIrMapper.h"

#ifdef POV_WITH_ANTLR_RUNTIME

#include <cstdlib>
#include <stdexcept>

#include "POVParserBaseVisitor.h"
#include "io/pov/antlr/AntlrParsedSceneProgram.h"
#include "io/pov/antlr/AntlrSceneIr.h"
#include "io/pov/antlr/AntlrSceneParserFrontend.h"

namespace {
double parseNumberText(const std::string &text)
{
    return std::strtod(text.c_str(), nullptr);
}

AntlrIrVector3 parseVector(POVParser::VectorLiteralContext *ctx)
{
    AntlrIrVector3 out = {0.0, 0.0, 0.0};
    if (ctx == nullptr) {
        return out;
    }
    const auto nums = ctx->signedNumber();
    if (nums.size() >= 3) {
        out.x = parseNumberText(nums[0]->getText());
        out.y = parseNumberText(nums[1]->getText());
        out.z = parseNumberText(nums[2]->getText());
    }
    return out;
}

AntlrIrColor parseColour(POVParser::ColourLiteralContext *ctx)
{
    AntlrIrColor out = {0.0, 0.0, 0.0, 0.0};
    AntlrIrVector3 v = parseVector(ctx != nullptr ? ctx->vectorLiteral() : nullptr);
    out.r = v.x;
    out.g = v.y;
    out.b = v.z;
    out.a = 0.0;
    return out;
}

AntlrIrTextureChain parseTextureChain(POVParser::TextureChainContext *ctx)
{
    AntlrIrTextureChain out;
    if (ctx == nullptr) {
        return out;
    }

    for (POVParser::TextureElementContext *elem : ctx->textureElement()) {
        if (elem == nullptr) {
            continue;
        }
        out.rawElements.push_back(elem->getText());

        const auto bodies = elem->textureBodyElement();
        if (bodies.size() == 1 && bodies[0] != nullptr && bodies[0]->IDENTIFIER() != nullptr) {
            out.simpleReferenceIdentifiers.push_back(bodies[0]->IDENTIFIER()->getText());
        }
    }
    return out;
}

AntlrIrTextureChain parseSingleTextureElement(POVParser::TextureElementContext *ctx)
{
    AntlrIrTextureChain out;
    if (ctx == nullptr) {
        return out;
    }
    out.rawElements.push_back(ctx->getText());
    const auto bodies = ctx->textureBodyElement();
    if (bodies.size() == 1 && bodies[0] != nullptr && bodies[0]->IDENTIFIER() != nullptr) {
        out.simpleReferenceIdentifiers.push_back(bodies[0]->IDENTIFIER()->getText());
    }
    return out;
}

void appendTextureChain(AntlrIrTextureChain &dst, const AntlrIrTextureChain &src)
{
    for (const std::string &elem : src.rawElements) {
        dst.rawElements.push_back(elem);
    }
    for (const std::string &id : src.simpleReferenceIdentifiers) {
        dst.simpleReferenceIdentifiers.push_back(id);
    }
}

void fillSphereNodeFromContext(AntlrIrSphereNode &node, POVParser::SphereStatementContext *ctx)
{
    POVParser::SphereBaseContext *base = ctx->sphereBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral() != nullptr && base->signedNumber() != nullptr) {
        node.hasInlineBase = true;
        node.center = parseVector(base->vectorLiteral());
        node.radius = parseNumberText(base->signedNumber()->getText());
    }

    for (POVParser::SphereModifierContext *mod : ctx->sphereModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseVector(t->vectorLiteral());
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(
                    node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR sphere transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            AntlrIrTextureChain local = parseSingleTextureElement(mod->textureElement());
            appendTextureChain(node.textureChain, local);
        }
    }
}

AntlrIrSphereNode *makeSphereNodeFromContext(POVParser::SphereStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrSphereNode *node = new AntlrIrSphereNode();
    fillSphereNodeFromContext(*node, ctx);
    return node;
}

void fillObjectNodeFromContext(AntlrIrObjectNode &node, POVParser::ObjectStatementContext *ctx)
;
void fillCompositeNodeFromContext(
    AntlrIrCompositeNode &node, POVParser::CompositeStatementContext *ctx);

AntlrIrObjectNode *makeObjectNodeFromContext(POVParser::ObjectStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrObjectNode *node = new AntlrIrObjectNode();
    fillObjectNodeFromContext(*node, ctx);
    return node;
}

AntlrIrCompositeNode *makeCompositeNodeFromContext(POVParser::CompositeStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrCompositeNode *node = new AntlrIrCompositeNode();
    fillCompositeNodeFromContext(*node, ctx);
    return node;
}

void fillObjectNodeFromContext(AntlrIrObjectNode &node, POVParser::ObjectStatementContext *ctx)
{
    for (POVParser::ObjectBodyElementContext *elem : ctx->objectBodyElement()) {
        if (elem->IDENTIFIER() != nullptr) {
            if (!node.hasReference && node.childSphereCount == 0 &&
                node.childObjectCount == 0 && node.childCompositeCount == 0) {
                node.hasReference = true;
                node.referenceIdentifier = elem->IDENTIFIER()->getText();
            } else if (node.childReferenceCount < AntlrIrObjectNode::MAX_CHILD_REFERENCES) {
                node.childReferenceIdentifiers[node.childReferenceCount++] =
                    elem->IDENTIFIER()->getText();
            } else {
                throw std::runtime_error("Too many ANTLR IR object child references");
            }
            continue;
        }
        if (elem->shapeStatement() != nullptr) {
            POVParser::ShapeStatementContext *shape = elem->shapeStatement();
            if (shape->sphereStatement() != nullptr) {
                if (node.childSphereCount < AntlrIrObjectNode::MAX_CHILD_SPHERES) {
                    node.childSpheres[node.childSphereCount++] =
                        makeSphereNodeFromContext(shape->sphereStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child spheres");
                }
            } else if (shape->objectStatement() != nullptr) {
                if (node.childObjectCount < AntlrIrObjectNode::MAX_CHILD_OBJECTS) {
                    node.childObjects[node.childObjectCount++] =
                        makeObjectNodeFromContext(shape->objectStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child objects");
                }
            } else if (shape->compositeStatement() != nullptr) {
                if (node.childCompositeCount < AntlrIrObjectNode::MAX_CHILD_COMPOSITES) {
                    node.childComposites[node.childCompositeCount++] =
                        makeCompositeNodeFromContext(shape->compositeStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child composites");
                }
            }
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseVector(t->vectorLiteral());
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR object transforms");
            }
            continue;
        }
        if (elem->colourLiteral() != nullptr) {
            node.colour = parseColour(elem->colourLiteral());
            node.hasColour = true;
            continue;
        }
        if (elem->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(elem->textureElement()));
            continue;
        }
        if (elem->NO_SHADOW() != nullptr) {
            node.noShadow = true;
            continue;
        }
    }
}

void fillCompositeNodeFromContext(
    AntlrIrCompositeNode &node, POVParser::CompositeStatementContext *ctx)
{
    for (POVParser::CompositeBodyElementContext *elem : ctx->compositeBodyElement()) {
        if (elem->IDENTIFIER() != nullptr) {
            if (!node.hasReference && node.childSphereCount == 0 &&
                node.childObjectCount == 0 && node.childCompositeCount == 0 &&
                node.childReferenceCount == 0) {
                node.hasReference = true;
                node.referenceIdentifier = elem->IDENTIFIER()->getText();
            } else if (node.childReferenceCount < AntlrIrCompositeNode::MAX_CHILD_REFERENCES) {
                node.childReferenceIdentifiers[node.childReferenceCount++] =
                    elem->IDENTIFIER()->getText();
            } else {
                throw std::runtime_error("Too many ANTLR IR composite child references");
            }
            continue;
        }
        if (elem->shapeStatement() != nullptr) {
            POVParser::ShapeStatementContext *shape = elem->shapeStatement();
            if (shape->sphereStatement() != nullptr) {
                if (node.childSphereCount < AntlrIrCompositeNode::MAX_CHILD_SPHERES) {
                    node.childSpheres[node.childSphereCount++] =
                        makeSphereNodeFromContext(shape->sphereStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR composite child spheres");
                }
            } else if (shape->objectStatement() != nullptr) {
                if (node.childObjectCount < AntlrIrCompositeNode::MAX_CHILD_OBJECTS) {
                    node.childObjects[node.childObjectCount++] =
                        makeObjectNodeFromContext(shape->objectStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR composite child objects");
                }
            } else if (shape->compositeStatement() != nullptr) {
                if (node.childCompositeCount < AntlrIrCompositeNode::MAX_CHILD_COMPOSITES) {
                    node.childComposites[node.childCompositeCount++] =
                        makeCompositeNodeFromContext(shape->compositeStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR composite child composites");
                }
            }
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseVector(t->vectorLiteral());
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR composite transforms");
            }
            continue;
        }
    }
}

class PovIrSubsetVisitor : public POVParserBaseVisitor {
  public:
    PovIrSubsetVisitor(AntlrSceneIrProgram &program)
        : mProgram(program)
    {
    }

    antlrcpp::Any visitMaxTraceLevelStatement(POVParser::MaxTraceLevelStatementContext *ctx) override
    {
        const double value = parseNumberText(ctx->signedNumber()->getText());
        const int line = (int)ctx->getStart()->getLine();
        const int column = (int)ctx->getStart()->getCharPositionInLine() + 1;
        AntlrSceneParserFrontend::appendMaxTraceLevel(mProgram, value, line, column, "<antlr>");
        return nullptr;
    }

    antlrcpp::Any visitDeclareStatement(POVParser::DeclareStatementContext *ctx) override
    {
        AntlrIrDeclareNode *node = new AntlrIrDeclareNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        node->identifier = ctx->IDENTIFIER()->getText();

        POVParser::DeclareValueContext *value = ctx->declareValue();
        if (value != nullptr && value->textureChain() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_TEXTURE_CHAIN;
            node->hasTextureChainValue = true;
            node->textureChainValue = parseTextureChain(value->textureChain());
        } else if (value != nullptr && value->sphereStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_SPHERE;
            node->hasSphereValue = true;
            node->sphereValue = new AntlrIrSphereNode();
            fillSphereNodeFromContext(*node->sphereValue, value->sphereStatement());
        } else if (value != nullptr && value->objectStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_OBJECT;
            node->hasObjectValue = true;
            node->objectValue = new AntlrIrObjectNode();
            fillObjectNodeFromContext(*node->objectValue, value->objectStatement());
        } else if (value != nullptr && value->compositeStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_COMPOSITE;
            node->hasCompositeValue = true;
            node->compositeValue = new AntlrIrCompositeNode();
            fillCompositeNodeFromContext(*node->compositeValue, value->compositeStatement());
        }
        AntlrSceneParserFrontend::appendDeclareNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitSphereStatement(POVParser::SphereStatementContext *ctx) override
    {
        AntlrIrSphereNode *node = new AntlrIrSphereNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";

        fillSphereNodeFromContext(*node, ctx);

        AntlrSceneParserFrontend::appendSphereNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitObjectStatement(POVParser::ObjectStatementContext *ctx) override
    {
        AntlrIrObjectNode *node = new AntlrIrObjectNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";

        fillObjectNodeFromContext(*node, ctx);

        AntlrSceneParserFrontend::appendObjectNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitCompositeStatement(POVParser::CompositeStatementContext *ctx) override
    {
        AntlrIrCompositeNode *node = new AntlrIrCompositeNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";

        fillCompositeNodeFromContext(*node, ctx);

        AntlrSceneParserFrontend::appendCompositeNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitFogStatement(POVParser::FogStatementContext *ctx) override
    {
        const int line = (int)ctx->getStart()->getLine();
        const int column = (int)ctx->getStart()->getCharPositionInLine() + 1;
        AntlrIrFogNode *fog = AntlrSceneParserFrontend::beginFogNode(line, column, "<antlr>");
        for (POVParser::FogElementContext *element : ctx->fogElement()) {
            if (element->colourLiteral() != nullptr) {
                fog->colour = parseColour(element->colourLiteral());
                fog->hasColour = true;
            } else if (element->signedNumber() != nullptr) {
                fog->distance = parseNumberText(element->signedNumber()->getText());
                fog->hasDistance = true;
            }
        }
        AntlrSceneParserFrontend::appendFogNode(mProgram, fog);
        return nullptr;
    }

    antlrcpp::Any visitDefaultStatement(POVParser::DefaultStatementContext *ctx) override
    {
        AntlrIrDefaultTextureNode *node = new AntlrIrDefaultTextureNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        if (ctx->textureChain() != nullptr) {
            node->hasTextureChain = true;
            node->textureChain = parseTextureChain(ctx->textureChain());
        }
        AntlrSceneParserFrontend::appendDefaultTextureNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitCameraStatement(POVParser::CameraStatementContext *ctx) override
    {
        const int line = (int)ctx->getStart()->getLine();
        const int column = (int)ctx->getStart()->getCharPositionInLine() + 1;
        AntlrIrCameraNode *camera =
            AntlrSceneParserFrontend::beginCameraNode(line, column, "<antlr>");
        for (POVParser::CameraElementContext *element : ctx->cameraElement()) {
            if (element->IDENTIFIER() != nullptr) {
                AntlrIrVector3 empty = {0.0, 0.0, 0.0};
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_REF, -1, empty);
                continue;
            }

            if (element->transform() != nullptr) {
                POVParser::TransformContext *t = element->transform();
                AntlrIrVector3 v = parseVector(t->vectorLiteral());
                if (t->TRANSLATE() != nullptr) {
                    AntlrSceneParserFrontend::appendCameraOp(
                        *camera, ANTLR_IR_CAMERA_TRANSLATE, -1, v);
                } else if (t->ROTATE() != nullptr) {
                    AntlrSceneParserFrontend::appendCameraOp(
                        *camera, ANTLR_IR_CAMERA_ROTATE, -1, v);
                } else if (t->SCALE() != nullptr) {
                    AntlrSceneParserFrontend::appendCameraOp(
                        *camera, ANTLR_IR_CAMERA_SCALE, -1, v);
                }
                continue;
            }

            AntlrIrVector3 v = parseVector(element->vectorLiteral());
            if (element->LOCATION() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_LOCATION, -1, v);
            } else if (element->DIRECTION() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_DIRECTION, -1, v);
            } else if (element->UP() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_UP, -1, v);
            } else if (element->RIGHT() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_RIGHT, -1, v);
            } else if (element->SKY() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_SKY, -1, v);
            } else if (element->LOOK_AT() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    *camera, ANTLR_IR_CAMERA_LOOK_AT, -1, v);
            }
        }
        AntlrSceneParserFrontend::appendCameraNode(mProgram, camera);
        return nullptr;
    }

  private:
    AntlrSceneIrProgram &mProgram;
};
}

AntlrParsedSceneProgram *
AntlrParseTreeToIrMapper::mapScene(POVParser::SceneContext *sceneCtx)
{
    if (sceneCtx == nullptr) {
        throw std::runtime_error("ANTLR scene context is null");
    }

    AntlrParsedSceneProgram *parsed = AntlrSceneParserFrontend::parseProgram();
    if (parsed == nullptr || parsed->program == nullptr) {
        throw std::runtime_error("Failed to initialize ANTLR parsed scene program");
    }

    PovIrSubsetVisitor visitor(*parsed->program);
    visitor.visitScene(sceneCtx);
    return parsed;
}

#endif
