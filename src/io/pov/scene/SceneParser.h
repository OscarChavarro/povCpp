#ifndef __SCENE_PARSER__
#define __SCENE_PARSER__

#include "environment/scene/Scene.h"
#include "io/pov/context/ParserContext.h"

class SceneParser {
  public:
    static void parse(Scene *framePtr);
    static void parse(Scene *framePtr, ParserContext &ctx);
    static void tokenInit();
    static void tokenInit(ParserContext &ctx);
    static void frameInit();
    static void frameInit(Scene *framePtr, ParserContext &ctx);
    static void parseFrame();
    static void parseFrame(Scene *framePtr, ParserContext &ctx);
    // Frees a single Constant slot's data given its (old) constantType - the
    // same dispatch freeConstants() below uses per-slot at end-of-parse, also
    // used by DeclarationParser::parseDeclare to free a slot's previous value
    // when #declare re-declares an already-declared identifier. Safe for every
    // type: every consumer of a declared constant clones it on use (copy
    // constructor or value-copy), per the declared-constant aliasing audit -
    // see doc/memoryAudit/ownership.md.
    static void freeConstant(int constantType, void *data);

  private:
    static void postProcessPhase(Scene *framePtr);
    static void freeConstants(ParserContext &ctx);
};

#endif
