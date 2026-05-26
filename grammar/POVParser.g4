parser grammar POVParser;

options { tokenVocab=POVLexer; }

scene
    : topLevelStatement* EOF
    ;

topLevelStatement
    : declareStatement
    | sphereStatement
    | objectStatement
    | compositeStatement
    | cameraStatement
    | fogStatement
    | defaultStatement
    | maxTraceLevelStatement
    ;

declareStatement
    : DECLARE IDENTIFIER EQUALS declareValue
    ;

declareValue
    : sphereStatement
    | objectStatement
    | compositeStatement
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
    ;

boundedByBlock
    : BOUNDED_BY LEFT_CURLY shapeStatement* RIGHT_CURLY
    ;

clippedByBlock
    : CLIPPED_BY LEFT_CURLY shapeStatement* RIGHT_CURLY
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
