#include "io/pov/ast/AstPrimitiveParser.h"

#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/ParserContext.h"
#include "io/pov/PrimitiveParser.h"

AstVector3
AstPrimitiveParser::parseVector(ParserContext &ctx)
{
    Vector3Dd v;
    PrimitiveParser::parseVector(&v, ctx);

    AstVector3 out;
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
    return out;
}

AstColor
AstPrimitiveParser::parseColour(ParserContext &ctx)
{
    RGBAColor c;
    Color::makeColor(&c, 0.0, 0.0, 0.0);
    PrimitiveParser::parseColour(&c, ctx);

    AstColor out;
    out.r = c.Red;
    out.g = c.Green;
    out.b = c.Blue;
    out.a = c.Alpha;
    return out;
}

double
AstPrimitiveParser::parseFloat(ParserContext &ctx)
{
    return PrimitiveParser::parseFloat(ctx);
}
