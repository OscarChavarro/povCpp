parser grammar POVParser;

options { tokenVocab=POVLexer; }

scene
    : topLevelStatement* EOF
    ;

topLevelStatement
    : includeStatement
    | declareStatement
    | lightSourceStatement
    | csgStatement
    | sphereStatement
    | objectStatement
    | compositeStatement
    | cameraStatement
    | fogStatement
    | defaultStatement
    | maxTraceLevelStatement
    ;

includeStatement
    : INCLUDE STRING
    ;

declareStatement
    : DECLARE IDENTIFIER EQUALS declareValue
    ;

declareValue
    : sphereStatement
    | objectStatement
    | compositeStatement
    | lightSourceStatement
    | csgStatement
    | cameraStatement
    | textureChain
    | vectorLiteral
    | signedNumber
    | colourLiteral
    | IDENTIFIER
    ;

sphereStatement
    : SPHERE LEFT_CURLY sphereBase sphereModifier* RIGHT_CURLY
    ;

sphereBase
    : vectorLiteral signedNumber
    | IDENTIFIER
    ;

sphereModifier
    : transform
    | textureElement
    | colourLiteral
    ;

objectStatement
    : OBJECT LEFT_CURLY objectBodyElement* RIGHT_CURLY
    ;

objectBodyElement
    : shapeStatement
    | boundedByBlock
    | clippedByBlock
    | transform
    | textureElement
    | colourLiteral
    | INVERSE
    | NO_SHADOW
    | IDENTIFIER
    ;

compositeStatement
    : COMPOSITE LEFT_CURLY compositeBodyElement* RIGHT_CURLY
    ;

compositeBodyElement
    : shapeStatement
    | boundedByBlock
    | clippedByBlock
    | transform
    | IDENTIFIER
    ;

shapeStatement
    : sphereStatement
    | objectStatement
    | compositeStatement
    | csgStatement
    ;

csgStatement
    : csgKeyword LEFT_CURLY csgBodyElement* RIGHT_CURLY
    ;

csgKeyword
    : UNION
    | INTERSECTION
    | DIFFERENCE
    ;

csgBodyElement
    : shapeStatement
    | boundedByBlock
    | clippedByBlock
    | transform
    | textureElement
    | colourLiteral
    | INVERSE
    | NO_SHADOW
    | IDENTIFIER
    ;

boundedByBlock
    : BOUNDED_BY LEFT_CURLY shapeStatement* RIGHT_CURLY
    ;

clippedByBlock
    : CLIPPED_BY LEFT_CURLY shapeStatement* RIGHT_CURLY
    ;

lightSourceStatement
    : LIGHT_SOURCE LEFT_CURLY lightSourceElement* RIGHT_CURLY
    ;

lightSourceElement
    : vectorLiteral
    | colourLiteral
    | POINT_AT vectorLiteral
    | TIGHTNESS signedNumber
    | RADIUS signedNumber
    | FALLOFF signedNumber
    | SPOTLIGHT
    | transform
    | IDENTIFIER
    ;

cameraStatement
    : CAMERA LEFT_CURLY cameraElement* RIGHT_CURLY
    ;

cameraElement
    : LOCATION vectorLiteral
    | DIRECTION vectorLiteral
    | UP vectorLiteral
    | RIGHT vectorLiteral
    | SKY vectorLiteral
    | LOOK_AT vectorLiteral
    | transform
    | IDENTIFIER
    ;

fogStatement
    : FOG LEFT_CURLY fogElement* RIGHT_CURLY
    ;

fogElement
    : colourLiteral
    | signedNumber
    ;

defaultStatement
    : DEFAULT LEFT_CURLY textureChain? RIGHT_CURLY
    ;

maxTraceLevelStatement
    : MAX_TRACE_LEVEL signedNumber
    ;

textureChain
    : textureElement+
    ;

textureElement
    : TEXTURE LEFT_CURLY textureBodyElement* RIGHT_CURLY
    ;

textureBodyElement
    : IDENTIFIER
    | vectorLiteral
    | signedNumber
    | STRING
    | textureElement
    ;

colourLiteral
    : COLOUR vectorLiteral
    ;

transform
    : TRANSLATE vectorLiteral
    | ROTATE vectorLiteral
    | SCALE vectorLiteral
    ;

vectorLiteral
    : LEFT_ANGLE signedNumber COMMA? signedNumber COMMA? signedNumber RIGHT_ANGLE
    ;

signedNumber
    : PLUS? FLOAT
    | DASH FLOAT
    ;
