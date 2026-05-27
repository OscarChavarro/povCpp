#include "io/pov/antlr/AntlrParseTreeToIrMapper.h"

#ifdef POV_WITH_ANTLR_RUNTIME

#include <cstdlib>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

#include "POVParserBaseVisitor.h"
#include "io/pov/antlr/AntlrParsedSceneProgram.h"
#include "io/pov/antlr/AntlrSceneIr.h"
#include "io/pov/antlr/AntlrSceneParserFrontend.h"

namespace {
std::unordered_map<std::string, double> gDeclaredScalars;

double parseNumberText(const std::string &text)
{
    return std::strtod(text.c_str(), nullptr);
}

double parseScalarLiteral(POVParser::ScalarLiteralContext *ctx)
{
    if (ctx == nullptr) {
        return 0.0;
    }
    if (ctx->signedNumber() != nullptr) {
        return parseNumberText(ctx->signedNumber()->getText());
    }
    if (ctx->IDENTIFIER() != nullptr) {
        const std::string text = ctx->getText();
        bool negative = false;
        std::string name = text;
        if (!name.empty() && (name[0] == '+' || name[0] == '-')) {
            negative = (name[0] == '-');
            name = name.substr(1);
        }
        const auto it = gDeclaredScalars.find(name);
        if (it != gDeclaredScalars.end()) {
            return negative ? -it->second : it->second;
        }
    }
    return 0.0;
}

AntlrIrVector3 parseVector(POVParser::VectorLiteralContext *ctx)
{
    AntlrIrVector3 out = {0.0, 0.0, 0.0};
    if (ctx == nullptr) {
        return out;
    }
    const auto scalars = ctx->scalarLiteral();
    if (scalars.size() >= 3) {
        out.x = parseScalarLiteral(scalars[0]);
        out.y = parseScalarLiteral(scalars[1]);
        out.z = parseScalarLiteral(scalars[2]);
    }
    return out;
}

AntlrIrVector3 parseTransformVector(POVParser::TransformContext *ctx)
{
    if (ctx == nullptr) {
        return {0.0, 0.0, 0.0};
    }
    if (ctx->vectorLiteral() != nullptr) {
        return parseVector(ctx->vectorLiteral());
    }
    if (ctx->scalarLiteral() != nullptr) {
        const double s = parseScalarLiteral(ctx->scalarLiteral());
        return {s, s, s};
    }
    return {0.0, 0.0, 0.0};
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

AntlrIrColor parseColourKeywordLiteral(POVParser::ColourKeywordLiteralContext *ctx)
{
    AntlrIrColor out = {0.0, 0.0, 0.0, 0.0};
    if (ctx == nullptr) {
        return out;
    }
    const auto ids = ctx->IDENTIFIER();
    const auto scalars = ctx->scalarLiteral();
    if (ids.size() < 3 || scalars.size() < 3) {
        return out;
    }
    for (size_t i = 0; i < ids.size() && i < scalars.size(); ++i) {
        const std::string key = ids[i]->getText();
        const double value = parseScalarLiteral(scalars[i]);
        if (key == "red") {
            out.r = value;
        } else if (key == "green") {
            out.g = value;
        } else if (key == "blue") {
            out.b = value;
        } else if (key == "alpha") {
            out.a = value;
        }
    }
    return out;
}

std::string toLowerAscii(const std::string &s)
{
    std::string out = s;
    for (char &c : out) {
        c = (char)std::tolower((unsigned char)c);
    }
    return out;
}

AntlrIrColor parseNamedColourIdentifier(const std::string &name)
{
    AntlrIrColor out = {1.0, 1.0, 1.0, 0.0};
    const std::string key = toLowerAscii(name);
    if (key == "white") return out;
    if (key == "black") return {0.0, 0.0, 0.0, 0.0};
    if (key == "red") return {1.0, 0.0, 0.0, 0.0};
    if (key == "green") return {0.0, 1.0, 0.0, 0.0};
    if (key == "blue") return {0.0, 0.0, 1.0, 0.0};
    if (key == "yellow") return {1.0, 1.0, 0.0, 0.0};
    if (key == "cyan") return {0.0, 1.0, 1.0, 0.0};
    if (key == "magenta") return {1.0, 0.0, 1.0, 0.0};
    if (key == "gray" || key == "grey") return {0.5, 0.5, 0.5, 0.0};
    if (key == "lightgray" || key == "lightgrey") return {0.75, 0.75, 0.75, 0.0};
    if (key == "darkgray" || key == "darkgrey") return {0.25, 0.25, 0.25, 0.0};
    if (key == "dimgray" || key == "dimgrey") return {0.41, 0.41, 0.41, 0.0};
    return out;
}

AntlrIrColor parseColourNamedLiteral(POVParser::ColourNamedLiteralContext *ctx)
{
    if (ctx == nullptr || ctx->IDENTIFIER() == nullptr) {
        return {1.0, 1.0, 1.0, 0.0};
    }
    return parseNamedColourIdentifier(ctx->IDENTIFIER()->getText());
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
    } else if (base->vectorLiteral() != nullptr && base->scalarLiteral() != nullptr) {
        node.hasInlineBase = true;
        node.center = parseVector(base->vectorLiteral());
        node.radius = parseScalarLiteral(base->scalarLiteral());
    }

    for (POVParser::SphereModifierContext *mod : ctx->sphereModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
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
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
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

void fillPlaneNodeFromContext(AntlrIrPlaneNode &node, POVParser::PlaneStatementContext *ctx)
{
    POVParser::PlaneBaseContext *base = ctx->planeBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral() != nullptr && base->scalarLiteral() != nullptr) {
        node.hasInlineBase = true;
        node.normal = parseVector(base->vectorLiteral());
        node.distance = parseScalarLiteral(base->scalarLiteral());
    }

    for (POVParser::PlaneModifierContext *mod : ctx->planeModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR plane transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrPlaneNode *makePlaneNodeFromContext(POVParser::PlaneStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrPlaneNode *node = new AntlrIrPlaneNode();
    fillPlaneNodeFromContext(*node, ctx);
    return node;
}

void fillBoxNodeFromContext(AntlrIrBoxNode &node, POVParser::BoxStatementContext *ctx)
{
    POVParser::BoxBaseContext *base = ctx->boxBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral().size() >= 2) {
        node.hasInlineBase = true;
        node.minBounds = parseVector(base->vectorLiteral(0));
        node.maxBounds = parseVector(base->vectorLiteral(1));
    }

    for (POVParser::BoxModifierContext *mod : ctx->boxModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR box transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrBoxNode *makeBoxNodeFromContext(POVParser::BoxStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrBoxNode *node = new AntlrIrBoxNode();
    fillBoxNodeFromContext(*node, ctx);
    return node;
}

void fillTriangleNodeFromContext(AntlrIrTriangleNode &node, POVParser::TriangleStatementContext *ctx)
{
    POVParser::TriangleBaseContext *base = ctx->triangleBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral().size() >= 3) {
        node.hasInlineBase = true;
        node.p1 = parseVector(base->vectorLiteral(0));
        node.p2 = parseVector(base->vectorLiteral(1));
        node.p3 = parseVector(base->vectorLiteral(2));
    }

    for (POVParser::TriangleModifierContext *mod : ctx->triangleModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR triangle transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrTriangleNode *makeTriangleNodeFromContext(POVParser::TriangleStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrTriangleNode *node = new AntlrIrTriangleNode();
    fillTriangleNodeFromContext(*node, ctx);
    return node;
}

void fillSmoothTriangleNodeFromContext(
    AntlrIrSmoothTriangleNode &node, POVParser::SmoothTriangleStatementContext *ctx)
{
    POVParser::SmoothTriangleBaseContext *base = ctx->smoothTriangleBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral().size() >= 6) {
        node.hasInlineBase = true;
        node.p1 = parseVector(base->vectorLiteral(0));
        node.n1 = parseVector(base->vectorLiteral(1));
        node.p2 = parseVector(base->vectorLiteral(2));
        node.n2 = parseVector(base->vectorLiteral(3));
        node.p3 = parseVector(base->vectorLiteral(4));
        node.n3 = parseVector(base->vectorLiteral(5));
    }

    for (POVParser::SmoothTriangleModifierContext *mod : ctx->smoothTriangleModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR smooth_triangle transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrSmoothTriangleNode *makeSmoothTriangleNodeFromContext(
    POVParser::SmoothTriangleStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrSmoothTriangleNode *node = new AntlrIrSmoothTriangleNode();
    fillSmoothTriangleNodeFromContext(*node, ctx);
    return node;
}

void fillQuadricNodeFromContext(AntlrIrQuadricNode &node, POVParser::QuadricStatementContext *ctx)
{
    POVParser::QuadricBaseContext *base = ctx->quadricBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else if (base->vectorLiteral().size() >= 3 && base->scalarLiteral() != nullptr) {
        node.hasInlineBase = true;
        node.object2Terms = parseVector(base->vectorLiteral(0));
        node.objectMixedTerms = parseVector(base->vectorLiteral(1));
        node.objectTerms = parseVector(base->vectorLiteral(2));
        node.objectConstant = parseScalarLiteral(base->scalarLiteral());
    }

    for (POVParser::QuadricModifierContext *mod : ctx->quadricModifier()) {
        if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR quadric transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrQuadricNode *makeQuadricNodeFromContext(POVParser::QuadricStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrQuadricNode *node = new AntlrIrQuadricNode();
    fillQuadricNodeFromContext(*node, ctx);
    return node;
}

void fillQuarticNodeFromContext(AntlrIrQuarticNode &node, POVParser::QuarticStatementContext *ctx)
{
    POVParser::QuarticBaseContext *base = ctx->quarticBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else {
        node.hasInlineBase = true;
        const auto coeffs = base->scalarLiteral();
        for (size_t i = 0; i < coeffs.size(); ++i) {
            if (node.coefficientCount >= AntlrIrQuarticNode::MAX_COEFFICIENTS) {
                throw std::runtime_error("Too many ANTLR IR quartic coefficients");
            }
            node.coefficients[node.coefficientCount++] = parseScalarLiteral(coeffs[i]);
        }
    }

    for (POVParser::QuarticModifierContext *mod : ctx->quarticModifier()) {
        if (mod->STURM() != nullptr) {
            node.sturm = true;
        } else if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR quartic transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrQuarticNode *makeQuarticNodeFromContext(POVParser::QuarticStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrQuarticNode *node = new AntlrIrQuarticNode();
    fillQuarticNodeFromContext(*node, ctx);
    return node;
}

void fillBlobNodeFromContext(AntlrIrBlobNode &node, POVParser::BlobStatementContext *ctx)
{
    POVParser::BlobBaseContext *base = ctx->blobBase();
    if (base->IDENTIFIER() != nullptr) {
        node.hasReferenceBase = true;
        node.referenceIdentifier = base->IDENTIFIER()->getText();
    } else {
        node.hasInlineBase = true;
        for (POVParser::BlobElementContext *elem : base->blobElement()) {
            if (elem->THRESHOLD() != nullptr && !elem->scalarLiteral().empty()) {
                node.hasThreshold = true;
                node.threshold = parseScalarLiteral(elem->scalarLiteral(0));
            } else if (
                elem->COMPONENT() != nullptr && elem->scalarLiteral().size() >= 2 &&
                elem->vectorLiteral() != nullptr) {
                if (node.componentCount >= AntlrIrBlobNode::MAX_COMPONENTS) {
                    throw std::runtime_error("Too many ANTLR IR blob components");
                }
                AntlrIrBlobComponent &c = node.components[node.componentCount++];
                c.coeff = parseScalarLiteral(elem->scalarLiteral(0));
                c.radius = parseScalarLiteral(elem->scalarLiteral(1));
                c.position = parseVector(elem->vectorLiteral());
            }
        }
    }

    for (POVParser::BlobModifierContext *mod : ctx->blobModifier()) {
        if (mod->STURM() != nullptr) {
            node.sturm = true;
        } else if (mod->transform() != nullptr) {
            POVParser::TransformContext *t = mod->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR blob transforms");
            }
        } else if (mod->colourLiteral() != nullptr) {
            node.colour = parseColour(mod->colourLiteral());
            node.hasColour = true;
        } else if (mod->colourNamedLiteral() != nullptr) {
            node.colour = parseColourNamedLiteral(mod->colourNamedLiteral());
            node.hasColour = true;
        } else if (mod->textureElement() != nullptr) {
            node.hasTextureChain = true;
            appendTextureChain(node.textureChain, parseSingleTextureElement(mod->textureElement()));
        } else if (mod->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
        }
    }
}

AntlrIrBlobNode *makeBlobNodeFromContext(POVParser::BlobStatementContext *ctx)
{
    if (ctx == nullptr) {
        return nullptr;
    }
    AntlrIrBlobNode *node = new AntlrIrBlobNode();
    fillBlobNodeFromContext(*node, ctx);
    return node;
}

void fillObjectNodeFromContext(AntlrIrObjectNode &node, POVParser::ObjectStatementContext *ctx)
;
void fillCompositeNodeFromContext(
    AntlrIrCompositeNode &node, POVParser::CompositeStatementContext *ctx);
void fillLightNodeFromContext(AntlrIrLightNode &node, POVParser::LightSourceStatementContext *ctx);
void fillCsgNodeFromContext(AntlrIrCsgNode &node, POVParser::CsgStatementContext *ctx);
void appendObjectBoundedShape(
    AntlrIrObjectNode &node, POVParser::ShapeStatementContext *shape, bool clipped);
void appendCompositeBoundedShape(
    AntlrIrCompositeNode &node, POVParser::ShapeStatementContext *shape, bool clipped);
AntlrIrCsgNode *makeCsgWrapperFromShape(POVParser::ShapeStatementContext *shape);

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
        POVParser::IdentifierInvocationContext *inv = elem->identifierInvocation();
        if (inv != nullptr && inv->LEFT_CURLY() == nullptr && inv->objectArgument().empty()) {
            if (!node.hasReference && node.childSphereCount == 0 &&
                node.childPlaneCount == 0 && node.childBoxCount == 0 &&
                node.childTriangleCount == 0 && node.childSmoothTriangleCount == 0 &&
                node.childQuadricCount == 0 && node.childQuarticCount == 0 &&
                node.childBlobCount == 0 &&
                node.childLightCount == 0 &&
                node.childObjectCount == 0 && node.childCompositeCount == 0) {
                node.hasReference = true;
                node.referenceIdentifier = inv->IDENTIFIER()->getText();
            } else if (node.childReferenceCount < AntlrIrObjectNode::MAX_CHILD_REFERENCES) {
                node.childReferenceIdentifiers[node.childReferenceCount++] =
                    inv->IDENTIFIER()->getText();
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
            } else if (shape->planeStatement() != nullptr) {
                if (node.childPlaneCount < AntlrIrObjectNode::MAX_CHILD_PLANES) {
                    node.childPlanes[node.childPlaneCount++] =
                        makePlaneNodeFromContext(shape->planeStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child planes");
                }
            } else if (shape->boxStatement() != nullptr) {
                if (node.childBoxCount < AntlrIrObjectNode::MAX_CHILD_BOXES) {
                    node.childBoxes[node.childBoxCount++] =
                        makeBoxNodeFromContext(shape->boxStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child boxes");
                }
            } else if (shape->triangleStatement() != nullptr) {
                if (node.childTriangleCount < AntlrIrObjectNode::MAX_CHILD_TRIANGLES) {
                    node.childTriangles[node.childTriangleCount++] =
                        makeTriangleNodeFromContext(shape->triangleStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child triangles");
                }
            } else if (shape->smoothTriangleStatement() != nullptr) {
                if (node.childSmoothTriangleCount < AntlrIrObjectNode::MAX_CHILD_SMOOTH_TRIANGLES) {
                    node.childSmoothTriangles[node.childSmoothTriangleCount++] =
                        makeSmoothTriangleNodeFromContext(shape->smoothTriangleStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child smooth_triangles");
                }
            } else if (shape->quadricStatement() != nullptr) {
                if (node.childQuadricCount < AntlrIrObjectNode::MAX_CHILD_QUADRICS) {
                node.childQuadrics[node.childQuadricCount++] =
                    makeQuadricNodeFromContext(shape->quadricStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child quadrics");
                }
            } else if (shape->quarticStatement() != nullptr) {
                if (node.childQuarticCount < AntlrIrObjectNode::MAX_CHILD_QUARTICS) {
                    node.childQuartics[node.childQuarticCount++] =
                        makeQuarticNodeFromContext(shape->quarticStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child quartics");
                }
            } else if (shape->blobStatement() != nullptr) {
                if (node.childBlobCount < AntlrIrObjectNode::MAX_CHILD_BLOBS) {
                    node.childBlobs[node.childBlobCount++] =
                        makeBlobNodeFromContext(shape->blobStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child blobs");
                }
            } else if (shape->csgStatement() != nullptr) {
                if (node.childCsgCount < AntlrIrObjectNode::MAX_CHILD_CSGS) {
                    AntlrIrCsgNode *child = new AntlrIrCsgNode();
                    fillCsgNodeFromContext(*child, shape->csgStatement());
                    node.childCsgs[node.childCsgCount++] = child;
                } else {
                    throw std::runtime_error("Too many ANTLR IR object child csg nodes");
                }
            } else {
                throw std::runtime_error("Unsupported ANTLR object child shape");
            }
            continue;
        }
        if (elem->lightSourceStatement() != nullptr) {
            if (node.childLightCount < AntlrIrObjectNode::MAX_CHILD_LIGHTS) {
                AntlrIrLightNode *light = new AntlrIrLightNode();
                fillLightNodeFromContext(*light, elem->lightSourceStatement());
                node.childLights[node.childLightCount++] = light;
            } else {
                throw std::runtime_error("Too many ANTLR IR object child light sources");
            }
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseTransformVector(t);
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
        if (elem->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
            continue;
        }
        if (elem->boundedByBlock() != nullptr) {
            for (POVParser::BoundedShapeElementContext *bounded : elem->boundedByBlock()->boundedShapeElement()) {
                if (bounded->shapeStatement() != nullptr) {
                    appendObjectBoundedShape(node, bounded->shapeStatement(), false);
                }
            }
            continue;
        }
        if (elem->clippedByBlock() != nullptr) {
            for (POVParser::BoundedShapeElementContext *bounded : elem->clippedByBlock()->boundedShapeElement()) {
                if (bounded->shapeStatement() != nullptr) {
                    appendObjectBoundedShape(node, bounded->shapeStatement(), true);
                }
            }
            continue;
        }
    }
}

void fillCompositeNodeFromContext(
    AntlrIrCompositeNode &node, POVParser::CompositeStatementContext *ctx)
{
    const bool singleReferenceComposite =
        ctx->compositeBodyElement().size() == 1 &&
        ctx->compositeBodyElement()[0] != nullptr &&
        ctx->compositeBodyElement()[0]->IDENTIFIER() != nullptr;

    for (POVParser::CompositeBodyElementContext *elem : ctx->compositeBodyElement()) {
        if (elem->IDENTIFIER() != nullptr) {
            if (singleReferenceComposite && !node.hasReference) {
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
            } else {
                throw std::runtime_error(
                    "ANTLR composite child primitive parsed but not lowered yet");
            }
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseTransformVector(t);
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
        if (elem->boundedByBlock() != nullptr) {
            for (POVParser::BoundedShapeElementContext *bounded : elem->boundedByBlock()->boundedShapeElement()) {
                if (bounded->shapeStatement() != nullptr) {
                    appendCompositeBoundedShape(node, bounded->shapeStatement(), false);
                }
            }
            continue;
        }
        if (elem->clippedByBlock() != nullptr) {
            for (POVParser::BoundedShapeElementContext *bounded : elem->clippedByBlock()->boundedShapeElement()) {
                if (bounded->shapeStatement() != nullptr) {
                    appendCompositeBoundedShape(node, bounded->shapeStatement(), true);
                }
            }
            continue;
        }
    }
}

void appendObjectBoundedShape(
    AntlrIrObjectNode &node, POVParser::ShapeStatementContext *shape, bool clipped)
{
    if (shape == nullptr) {
        return;
    }
    if (shape->sphereStatement() != nullptr) {
        if (!clipped && node.boundedSphereCount < AntlrIrObjectNode::MAX_BOUNDED_SPHERES) {
            node.boundedSpheres[node.boundedSphereCount++] =
                makeSphereNodeFromContext(shape->sphereStatement());
            return;
        }
        if (clipped && node.clippedSphereCount < AntlrIrObjectNode::MAX_CLIPPED_SPHERES) {
            node.clippedSpheres[node.clippedSphereCount++] =
                makeSphereNodeFromContext(shape->sphereStatement());
            return;
        }
        throw std::runtime_error("Too many ANTLR IR object bounded/clipped spheres");
    }
    if (shape->csgStatement() != nullptr) {
        AntlrIrCsgNode *child = new AntlrIrCsgNode();
        fillCsgNodeFromContext(*child, shape->csgStatement());
        if (!clipped && node.boundedCsgCount < AntlrIrObjectNode::MAX_BOUNDED_CSGS) {
            node.boundedCsgs[node.boundedCsgCount++] = child;
            return;
        }
        if (clipped && node.clippedCsgCount < AntlrIrObjectNode::MAX_CLIPPED_CSGS) {
            node.clippedCsgs[node.clippedCsgCount++] = child;
            return;
        }
        delete child;
        throw std::runtime_error("Too many ANTLR IR object bounded/clipped csg nodes");
    }
    AntlrIrCsgNode *wrapped = makeCsgWrapperFromShape(shape);
    if (wrapped != nullptr) {
        if (!clipped && node.boundedCsgCount < AntlrIrObjectNode::MAX_BOUNDED_CSGS) {
            node.boundedCsgs[node.boundedCsgCount++] = wrapped;
            return;
        }
        if (clipped && node.clippedCsgCount < AntlrIrObjectNode::MAX_CLIPPED_CSGS) {
            node.clippedCsgs[node.clippedCsgCount++] = wrapped;
            return;
        }
        delete wrapped;
        throw std::runtime_error("Too many ANTLR IR object bounded/clipped csg nodes");
    }
    throw std::runtime_error("Unsupported ANTLR bounded_by/clipped_by shape in object");
}

void appendCompositeBoundedShape(
    AntlrIrCompositeNode &node, POVParser::ShapeStatementContext *shape, bool clipped)
{
    if (shape == nullptr) {
        return;
    }
    if (shape->sphereStatement() != nullptr) {
        if (!clipped && node.boundedSphereCount < AntlrIrCompositeNode::MAX_BOUNDED_SPHERES) {
            node.boundedSpheres[node.boundedSphereCount++] =
                makeSphereNodeFromContext(shape->sphereStatement());
            return;
        }
        if (clipped && node.clippedSphereCount < AntlrIrCompositeNode::MAX_CLIPPED_SPHERES) {
            node.clippedSpheres[node.clippedSphereCount++] =
                makeSphereNodeFromContext(shape->sphereStatement());
            return;
        }
        throw std::runtime_error("Too many ANTLR IR composite bounded/clipped spheres");
    }
    if (shape->csgStatement() != nullptr) {
        AntlrIrCsgNode *child = new AntlrIrCsgNode();
        fillCsgNodeFromContext(*child, shape->csgStatement());
        if (!clipped && node.boundedCsgCount < AntlrIrCompositeNode::MAX_BOUNDED_CSGS) {
            node.boundedCsgs[node.boundedCsgCount++] = child;
            return;
        }
        if (clipped && node.clippedCsgCount < AntlrIrCompositeNode::MAX_CLIPPED_CSGS) {
            node.clippedCsgs[node.clippedCsgCount++] = child;
            return;
        }
        delete child;
        throw std::runtime_error("Too many ANTLR IR composite bounded/clipped csg nodes");
    }
    AntlrIrCsgNode *wrapped = makeCsgWrapperFromShape(shape);
    if (wrapped != nullptr) {
        if (!clipped && node.boundedCsgCount < AntlrIrCompositeNode::MAX_BOUNDED_CSGS) {
            node.boundedCsgs[node.boundedCsgCount++] = wrapped;
            return;
        }
        if (clipped && node.clippedCsgCount < AntlrIrCompositeNode::MAX_CLIPPED_CSGS) {
            node.clippedCsgs[node.clippedCsgCount++] = wrapped;
            return;
        }
        delete wrapped;
        throw std::runtime_error("Too many ANTLR IR composite bounded/clipped csg nodes");
    }
    throw std::runtime_error("Unsupported ANTLR bounded_by/clipped_by shape in composite");
}

AntlrIrCsgNode *makeCsgWrapperFromShape(POVParser::ShapeStatementContext *shape)
{
    if (shape == nullptr || shape->csgStatement() != nullptr || shape->sphereStatement() != nullptr) {
        return nullptr;
    }
    AntlrIrCsgNode *wrapped = new AntlrIrCsgNode();
    wrapped->op = ANTLR_IR_CSG_UNION;
    if (shape->planeStatement() != nullptr) {
        wrapped->childPlanes[wrapped->childPlaneCount++] = makePlaneNodeFromContext(shape->planeStatement());
    } else if (shape->boxStatement() != nullptr) {
        wrapped->childBoxes[wrapped->childBoxCount++] = makeBoxNodeFromContext(shape->boxStatement());
    } else if (shape->triangleStatement() != nullptr) {
        wrapped->childTriangles[wrapped->childTriangleCount++] = makeTriangleNodeFromContext(shape->triangleStatement());
    } else if (shape->smoothTriangleStatement() != nullptr) {
        wrapped->childSmoothTriangles[wrapped->childSmoothTriangleCount++] =
            makeSmoothTriangleNodeFromContext(shape->smoothTriangleStatement());
    } else if (shape->quadricStatement() != nullptr) {
        wrapped->childQuadrics[wrapped->childQuadricCount++] = makeQuadricNodeFromContext(shape->quadricStatement());
    } else if (shape->quarticStatement() != nullptr) {
        wrapped->childQuartics[wrapped->childQuarticCount++] = makeQuarticNodeFromContext(shape->quarticStatement());
    } else if (shape->blobStatement() != nullptr) {
        wrapped->childBlobs[wrapped->childBlobCount++] = makeBlobNodeFromContext(shape->blobStatement());
    } else if (shape->objectStatement() != nullptr) {
        wrapped->childObjects[wrapped->childObjectCount++] = makeObjectNodeFromContext(shape->objectStatement());
    } else if (shape->compositeStatement() != nullptr) {
        wrapped->childComposites[wrapped->childCompositeCount++] =
            makeCompositeNodeFromContext(shape->compositeStatement());
    } else {
        delete wrapped;
        return nullptr;
    }
    return wrapped;
}

void fillLightNodeFromContext(AntlrIrLightNode &node, POVParser::LightSourceStatementContext *ctx)
{
    for (POVParser::LightSourceElementContext *elem : ctx->lightSourceElement()) {
        if (elem->IDENTIFIER() != nullptr) {
            if (!node.hasReference && !node.hasCenter) {
                node.hasReference = true;
                node.referenceIdentifier = elem->IDENTIFIER()->getText();
            }
            continue;
        }
        if (elem->vectorLiteral() != nullptr && elem->POINT_AT() == nullptr) {
            if (!node.hasCenter) {
                node.hasCenter = true;
                node.center = parseVector(elem->vectorLiteral());
            }
            continue;
        }
        if (elem->colourLiteral() != nullptr) {
            node.hasColour = true;
            node.colour = parseColour(elem->colourLiteral());
            continue;
        }
        if (elem->colourNamedLiteral() != nullptr) {
            node.hasColour = true;
            node.colour = parseColourNamedLiteral(elem->colourNamedLiteral());
            continue;
        }
        if (elem->colourKeywordLiteral() != nullptr) {
            node.hasColour = true;
            node.colour = parseColourKeywordLiteral(elem->colourKeywordLiteral());
            continue;
        }
        if (elem->POINT_AT() != nullptr && elem->vectorLiteral() != nullptr) {
            node.hasPointAt = true;
            node.pointAt = parseVector(elem->vectorLiteral());
            continue;
        }
        if (elem->TIGHTNESS() != nullptr && elem->scalarLiteral() != nullptr) {
            node.hasTightness = true;
            node.tightness = parseScalarLiteral(elem->scalarLiteral());
            continue;
        }
        if (elem->RADIUS() != nullptr && elem->scalarLiteral() != nullptr) {
            node.hasRadius = true;
            node.radiusDegrees = parseScalarLiteral(elem->scalarLiteral());
            continue;
        }
        if (elem->FALLOFF() != nullptr && elem->scalarLiteral() != nullptr) {
            node.hasFalloff = true;
            node.falloffDegrees = parseScalarLiteral(elem->scalarLiteral());
            continue;
        }
        if (elem->SPOTLIGHT() != nullptr) {
            node.spotlight = true;
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR light transforms");
            }
        }
    }
}

void fillCameraNodeFromContext(AntlrIrCameraNode &node, POVParser::CameraStatementContext *ctx)
{
    for (POVParser::CameraElementContext *element : ctx->cameraElement()) {
        if (element->IDENTIFIER() != nullptr) {
            if (!node.hasReference && node.opCount == 0) {
                node.hasReference = true;
                node.referenceIdentifier = element->IDENTIFIER()->getText();
            } else {
                AntlrIrVector3 empty = {0.0, 0.0, 0.0};
                AntlrSceneParserFrontend::appendCameraOp(
                    node, ANTLR_IR_CAMERA_REF, -1, empty);
            }
            continue;
        }

        if (element->transform() != nullptr) {
            POVParser::TransformContext *t = element->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            if (t->TRANSLATE() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    node, ANTLR_IR_CAMERA_TRANSLATE, -1, v);
            } else if (t->ROTATE() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    node, ANTLR_IR_CAMERA_ROTATE, -1, v);
            } else if (t->SCALE() != nullptr) {
                AntlrSceneParserFrontend::appendCameraOp(
                    node, ANTLR_IR_CAMERA_SCALE, -1, v);
            }
            continue;
        }

        AntlrIrVector3 v = parseVector(element->vectorLiteral());
        if (element->LOCATION() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_LOCATION, -1, v);
        } else if (element->DIRECTION() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_DIRECTION, -1, v);
        } else if (element->UP() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_UP, -1, v);
        } else if (element->RIGHT() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_RIGHT, -1, v);
        } else if (element->SKY() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_SKY, -1, v);
        } else if (element->LOOK_AT() != nullptr) {
            AntlrSceneParserFrontend::appendCameraOp(
                node, ANTLR_IR_CAMERA_LOOK_AT, -1, v);
        }
    }
}

void fillCsgNodeFromContext(AntlrIrCsgNode &node, POVParser::CsgStatementContext *ctx)
{
    if (ctx->csgKeyword()->UNION() != nullptr) {
        node.op = ANTLR_IR_CSG_UNION;
    } else if (ctx->csgKeyword()->INTERSECTION() != nullptr) {
        node.op = ANTLR_IR_CSG_INTERSECTION;
    } else {
        node.op = ANTLR_IR_CSG_DIFFERENCE;
    }

    for (POVParser::CsgBodyElementContext *elem : ctx->csgBodyElement()) {
        POVParser::IdentifierInvocationContext *inv = elem->identifierInvocation();
        if (inv != nullptr && inv->LEFT_CURLY() == nullptr && inv->objectArgument().empty()) {
            if (!node.hasReference && node.childSphereCount == 0 &&
                node.childPlaneCount == 0 && node.childBoxCount == 0 &&
                node.childTriangleCount == 0 && node.childSmoothTriangleCount == 0 &&
                node.childQuadricCount == 0 && node.childQuarticCount == 0 &&
                node.childBlobCount == 0 && node.childLightCount == 0 &&
                node.childObjectCount == 0 && node.childCompositeCount == 0 &&
                node.childCsgCount == 0 && node.childReferenceCount == 0) {
                node.hasReference = true;
                node.referenceIdentifier = inv->IDENTIFIER()->getText();
            } else if (node.childReferenceCount < AntlrIrCsgNode::MAX_CHILD_REFERENCES) {
                node.childReferenceIdentifiers[node.childReferenceCount++] =
                    inv->IDENTIFIER()->getText();
            } else {
                throw std::runtime_error("Too many ANTLR IR csg child references");
            }
            continue;
        }
        if (elem->shapeStatement() != nullptr) {
            POVParser::ShapeStatementContext *shape = elem->shapeStatement();
            if (shape->sphereStatement() != nullptr) {
                if (node.childSphereCount < AntlrIrCsgNode::MAX_CHILD_SPHERES) {
                    node.childSpheres[node.childSphereCount++] =
                        makeSphereNodeFromContext(shape->sphereStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child spheres");
                }
            } else if (shape->planeStatement() != nullptr) {
                if (node.childPlaneCount < AntlrIrCsgNode::MAX_CHILD_PLANES) {
                    node.childPlanes[node.childPlaneCount++] =
                        makePlaneNodeFromContext(shape->planeStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child planes");
                }
            } else if (shape->boxStatement() != nullptr) {
                if (node.childBoxCount < AntlrIrCsgNode::MAX_CHILD_BOXES) {
                    node.childBoxes[node.childBoxCount++] =
                        makeBoxNodeFromContext(shape->boxStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child boxes");
                }
            } else if (shape->triangleStatement() != nullptr) {
                if (node.childTriangleCount < AntlrIrCsgNode::MAX_CHILD_TRIANGLES) {
                    node.childTriangles[node.childTriangleCount++] =
                        makeTriangleNodeFromContext(shape->triangleStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child triangles");
                }
            } else if (shape->smoothTriangleStatement() != nullptr) {
                if (node.childSmoothTriangleCount < AntlrIrCsgNode::MAX_CHILD_SMOOTH_TRIANGLES) {
                    node.childSmoothTriangles[node.childSmoothTriangleCount++] =
                        makeSmoothTriangleNodeFromContext(shape->smoothTriangleStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child smooth_triangles");
                }
            } else if (shape->quadricStatement() != nullptr) {
                if (node.childQuadricCount < AntlrIrCsgNode::MAX_CHILD_QUADRICS) {
                    node.childQuadrics[node.childQuadricCount++] =
                        makeQuadricNodeFromContext(shape->quadricStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child quadrics");
                }
            } else if (shape->quarticStatement() != nullptr) {
                if (node.childQuarticCount < AntlrIrCsgNode::MAX_CHILD_QUARTICS) {
                    node.childQuartics[node.childQuarticCount++] =
                        makeQuarticNodeFromContext(shape->quarticStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child quartics");
                }
            } else if (shape->blobStatement() != nullptr) {
                if (node.childBlobCount < AntlrIrCsgNode::MAX_CHILD_BLOBS) {
                    node.childBlobs[node.childBlobCount++] =
                        makeBlobNodeFromContext(shape->blobStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child blobs");
                }
            } else if (shape->objectStatement() != nullptr) {
                if (node.childObjectCount < AntlrIrCsgNode::MAX_CHILD_OBJECTS) {
                    node.childObjects[node.childObjectCount++] =
                        makeObjectNodeFromContext(shape->objectStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child objects");
                }
            } else if (shape->compositeStatement() != nullptr) {
                if (node.childCompositeCount < AntlrIrCsgNode::MAX_CHILD_COMPOSITES) {
                    node.childComposites[node.childCompositeCount++] =
                        makeCompositeNodeFromContext(shape->compositeStatement());
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child composites");
                }
            } else if (shape->csgStatement() != nullptr) {
                if (node.childCsgCount < AntlrIrCsgNode::MAX_CHILD_CSGS) {
                    AntlrIrCsgNode *child = new AntlrIrCsgNode();
                    fillCsgNodeFromContext(*child, shape->csgStatement());
                    node.childCsgs[node.childCsgCount++] = child;
                } else {
                    throw std::runtime_error("Too many ANTLR IR csg child csgs");
                }
            } else {
                throw std::runtime_error("Unsupported ANTLR CSG child shape");
            }
            continue;
        }
        if (elem->lightSourceStatement() != nullptr) {
            if (node.childLightCount < AntlrIrCsgNode::MAX_CHILD_LIGHTS) {
                AntlrIrLightNode *light = new AntlrIrLightNode();
                fillLightNodeFromContext(*light, elem->lightSourceStatement());
                node.childLights[node.childLightCount++] = light;
            } else {
                throw std::runtime_error("Too many ANTLR IR csg child light sources");
            }
            continue;
        }
        if (elem->transform() != nullptr) {
            POVParser::TransformContext *t = elem->transform();
            AntlrIrVector3 v = parseTransformVector(t);
            AntlrIrTransformKind kind = ANTLR_IR_TRANSLATE;
            if (t->TRANSLATE() != nullptr) {
                kind = ANTLR_IR_TRANSLATE;
            } else if (t->ROTATE() != nullptr) {
                kind = ANTLR_IR_ROTATE;
            } else if (t->SCALE() != nullptr) {
                kind = ANTLR_IR_SCALE;
            }
            if (!AntlrSceneIrNodes::appendTransform(node.transforms, node.transformCount, kind, v)) {
                throw std::runtime_error("Too many ANTLR IR csg transforms");
            }
            continue;
        }
        if (elem->INVERSE() != nullptr) {
            node.inverted = !node.inverted;
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
        } else if (value != nullptr && value->planeStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_PLANE;
            node->hasPlaneValue = true;
            node->planeValue = new AntlrIrPlaneNode();
            fillPlaneNodeFromContext(*node->planeValue, value->planeStatement());
        } else if (value != nullptr && value->boxStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_BOX;
            node->hasBoxValue = true;
            node->boxValue = new AntlrIrBoxNode();
            fillBoxNodeFromContext(*node->boxValue, value->boxStatement());
        } else if (value != nullptr && value->triangleStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_TRIANGLE;
            node->hasTriangleValue = true;
            node->triangleValue = new AntlrIrTriangleNode();
            fillTriangleNodeFromContext(*node->triangleValue, value->triangleStatement());
        } else if (value != nullptr && value->smoothTriangleStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_SMOOTH_TRIANGLE;
            node->hasSmoothTriangleValue = true;
            node->smoothTriangleValue = new AntlrIrSmoothTriangleNode();
            fillSmoothTriangleNodeFromContext(
                *node->smoothTriangleValue, value->smoothTriangleStatement());
        } else if (value != nullptr && value->quadricStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_QUADRIC;
            node->hasQuadricValue = true;
            node->quadricValue = new AntlrIrQuadricNode();
            fillQuadricNodeFromContext(*node->quadricValue, value->quadricStatement());
        } else if (value != nullptr && value->quarticStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_QUARTIC;
            node->hasQuarticValue = true;
            node->quarticValue = new AntlrIrQuarticNode();
            fillQuarticNodeFromContext(*node->quarticValue, value->quarticStatement());
        } else if (value != nullptr && value->blobStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_BLOB;
            node->hasBlobValue = true;
            node->blobValue = new AntlrIrBlobNode();
            fillBlobNodeFromContext(*node->blobValue, value->blobStatement());
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
        } else if (value != nullptr && value->lightSourceStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_LIGHT;
            node->hasLightValue = true;
            node->lightValue = new AntlrIrLightNode();
            fillLightNodeFromContext(*node->lightValue, value->lightSourceStatement());
        } else if (value != nullptr && value->csgStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_CSG;
            node->hasCsgValue = true;
            node->csgValue = new AntlrIrCsgNode();
            fillCsgNodeFromContext(*node->csgValue, value->csgStatement());
        } else if (value != nullptr && value->cameraStatement() != nullptr) {
            node->valueKind = AntlrIrDeclareNode::DECLARE_CAMERA;
            node->hasCameraValue = true;
            node->cameraValue = new AntlrIrCameraNode();
            node->cameraValue->sourceLine = (int)value->cameraStatement()->getStart()->getLine();
            node->cameraValue->sourceColumn =
                (int)value->cameraStatement()->getStart()->getCharPositionInLine() + 1;
            node->cameraValue->sourceFile = "<antlr>";
            fillCameraNodeFromContext(*node->cameraValue, value->cameraStatement());
        } else if (value != nullptr && value->signedNumber() != nullptr) {
            gDeclaredScalars[node->identifier] = parseNumberText(value->signedNumber()->getText());
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

    antlrcpp::Any visitPlaneStatement(POVParser::PlaneStatementContext *ctx) override
    {
        AntlrIrPlaneNode *node = new AntlrIrPlaneNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillPlaneNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendPlaneNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitBoxStatement(POVParser::BoxStatementContext *ctx) override
    {
        AntlrIrBoxNode *node = new AntlrIrBoxNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillBoxNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendBoxNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitTriangleStatement(POVParser::TriangleStatementContext *ctx) override
    {
        AntlrIrTriangleNode *node = new AntlrIrTriangleNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillTriangleNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendTriangleNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitSmoothTriangleStatement(POVParser::SmoothTriangleStatementContext *ctx) override
    {
        AntlrIrSmoothTriangleNode *node = new AntlrIrSmoothTriangleNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillSmoothTriangleNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendSmoothTriangleNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitQuadricStatement(POVParser::QuadricStatementContext *ctx) override
    {
        AntlrIrQuadricNode *node = new AntlrIrQuadricNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillQuadricNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendQuadricNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitQuarticStatement(POVParser::QuarticStatementContext *ctx) override
    {
        AntlrIrQuarticNode *node = new AntlrIrQuarticNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillQuarticNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendQuarticNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitBlobStatement(POVParser::BlobStatementContext *ctx) override
    {
        AntlrIrBlobNode *node = new AntlrIrBlobNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillBlobNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendBlobNode(mProgram, node);
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

    antlrcpp::Any visitLightSourceStatement(POVParser::LightSourceStatementContext *ctx) override
    {
        AntlrIrLightNode *node = new AntlrIrLightNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillLightNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendLightNode(mProgram, node);
        return nullptr;
    }

    antlrcpp::Any visitCsgStatement(POVParser::CsgStatementContext *ctx) override
    {
        AntlrIrCsgNode *node = new AntlrIrCsgNode();
        node->sourceLine = (int)ctx->getStart()->getLine();
        node->sourceColumn = (int)ctx->getStart()->getCharPositionInLine() + 1;
        node->sourceFile = "<antlr>";
        fillCsgNodeFromContext(*node, ctx);
        AntlrSceneParserFrontend::appendCsgNode(mProgram, node);
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
        } else if (element->colourNamedLiteral() != nullptr) {
            fog->colour = parseColourNamedLiteral(element->colourNamedLiteral());
            fog->hasColour = true;
        } else if (element->colourKeywordLiteral() != nullptr) {
            fog->colour = parseColourKeywordLiteral(element->colourKeywordLiteral());
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
        fillCameraNodeFromContext(*camera, ctx);
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

    gDeclaredScalars.clear();
    PovIrSubsetVisitor visitor(*parsed->program);
    visitor.visitScene(sceneCtx);
    return parsed;
}

#endif
