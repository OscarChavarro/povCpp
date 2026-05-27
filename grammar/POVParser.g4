parser grammar POVParser;

options { tokenVocab=POVLexer; }

scene
    : topLevelStatement* EOF
    ;

topLevelStatement
    : includeStatement
    | declareStatement
    | anonymousTopLevelBlock
    | topLevelLiteralStatement
    | identifierInvocation
    | lightSourceStatement
    | csgStatement
    | sphereStatement
    | planeStatement
    | boxStatement
    | triangleStatement
    | smoothTriangleStatement
    | quadricStatement
    | quarticStatement
    | blobStatement
    | objectStatement
    | compositeStatement
    | cameraStatement
    | fogStatement
    | defaultStatement
    | maxTraceLevelStatement
    ;

topLevelLiteralStatement
    : scalarLiteral
    | vectorLiteral
    | STRING
    ;

anonymousTopLevelBlock
    : LEFT_CURLY topLevelStatement* RIGHT_CURLY
    ;

includeStatement
    : INCLUDE STRING
    ;

declareStatement
    : DECLARE IDENTIFIER EQUALS declareValue
    ;

declareValue
    : sphereStatement
    | planeStatement
    | boxStatement
    | triangleStatement
    | smoothTriangleStatement
    | quadricStatement
    | quarticStatement
    | blobStatement
    | objectStatement
    | compositeStatement
    | lightSourceStatement
    | csgStatement
    | cameraStatement
    | identifierInvocation
    | textureChain
    | vectorLiteral
    | signedNumber
    | colourLiteral
    | colourKeywordLiteral
    | colourNamedLiteral
    | IDENTIFIER
    ;

sphereStatement
    : SPHERE LEFT_CURLY sphereBase sphereModifier* RIGHT_CURLY
    ;

planeStatement
    : PLANE LEFT_CURLY planeBase planeModifier* RIGHT_CURLY
    ;

boxStatement
    : BOX LEFT_CURLY boxBase boxModifier* RIGHT_CURLY
    ;

triangleStatement
    : TRIANGLE LEFT_CURLY triangleBase triangleModifier* RIGHT_CURLY
    ;

smoothTriangleStatement
    : SMOOTH_TRIANGLE LEFT_CURLY smoothTriangleBase smoothTriangleModifier* RIGHT_CURLY
    ;

quadricStatement
    : QUADRIC LEFT_CURLY quadricBase quadricModifier* RIGHT_CURLY
    ;

quarticStatement
    : QUARTIC LEFT_CURLY quarticBase quarticModifier* RIGHT_CURLY
    ;

blobStatement
    : BLOB LEFT_CURLY blobBase blobModifier* RIGHT_CURLY
    ;

planeBase
    : vectorLiteral scalarLiteral
    | IDENTIFIER
    ;

planeModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

boxBase
    : vectorLiteral vectorLiteral
    | IDENTIFIER
    ;

boxModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

triangleBase
    : vectorLiteral vectorLiteral vectorLiteral
    | IDENTIFIER
    ;

triangleModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

smoothTriangleBase
    : vectorLiteral vectorLiteral vectorLiteral vectorLiteral vectorLiteral vectorLiteral
    | IDENTIFIER
    ;

smoothTriangleModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

quadricBase
    : vectorLiteral vectorLiteral vectorLiteral scalarLiteral
    | IDENTIFIER
    ;

quadricModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

quarticBase
    : IDENTIFIER
    | LEFT_ANGLE scalarLiteral+ RIGHT_ANGLE
    ;

quarticModifier
    : STURM
    | transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

blobBase
    : IDENTIFIER
    | blobElement+
    ;

blobElement
    : THRESHOLD scalarLiteral
    | COMPONENT scalarLiteral scalarLiteral vectorLiteral
    ;

blobModifier
    : STURM
    | transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

sphereBase
    : vectorLiteral scalarLiteral
    | IDENTIFIER
    ;

sphereModifier
    : transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

identifierInvocation
    : IDENTIFIER objectArgument* (LEFT_CURLY objectBodyElement* RIGHT_CURLY)?
    ;

objectArgument
    : scalarLiteral
    | vectorLiteral
    | STRING
    ;

objectStatement
    : OBJECT LEFT_CURLY objectBodyElement* RIGHT_CURLY
    ;

objectBodyElement
    : shapeStatement
    | identifierInvocation
    | lightSourceStatement
    | boundedByBlock
    | clippedByBlock
    | transform
    | textureElement
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | angleScalarList
    | vectorLiteral
    | signedNumber
    | STRING
    | INVERSE
    | NO_SHADOW
    ;

angleScalarList
    : LEFT_ANGLE scalarLiteral+ RIGHT_ANGLE
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
    | planeStatement
    | boxStatement
    | triangleStatement
    | smoothTriangleStatement
    | quadricStatement
    | quarticStatement
    | blobStatement
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
    | identifierInvocation
    | lightSourceStatement
    | transform
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | INVERSE
    ;

boundedByBlock
    : BOUNDED_BY LEFT_CURLY boundedShapeElement* RIGHT_CURLY
    ;

clippedByBlock
    : CLIPPED_BY LEFT_CURLY boundedShapeElement* RIGHT_CURLY
    ;

boundedShapeElement
    : shapeStatement
    | identifierInvocation
    ;

lightSourceStatement
    : LIGHT_SOURCE LEFT_CURLY lightSourceElement* RIGHT_CURLY
    ;

lightSourceElement
    : vectorLiteral
    | colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | POINT_AT vectorLiteral
    | TIGHTNESS scalarLiteral
    | RADIUS scalarLiteral
    | FALLOFF scalarLiteral
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
    | colourNamedLiteral
    | colourKeywordLiteral
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
    : IDENTIFIER textureArgument* (LEFT_CURLY textureBodyElement* RIGHT_CURLY)?
    | COLOUR
    | vectorLiteral
    | signedNumber
    | STRING
    | textureElement
    | transform
    | colourMapBlock
    ;

textureArgument
    : scalarLiteral
    | vectorLiteral
    | STRING
    ;

colourMapBlock
    : COLOUR_MAP LEFT_CURLY colourMapEntry* RIGHT_CURLY
    ;

colourMapEntry
    : LEFT_SQUARE scalarLiteral scalarLiteral colourMapColourStop+ RIGHT_SQUARE
    ;

colourMapColourStop
    : colourLiteral
    | colourNamedLiteral
    | colourKeywordLiteral
    | IDENTIFIER
    | vectorLiteral
    | signedNumber
    ;

colourLiteral
    : COLOUR vectorLiteral
    ;

colourNamedLiteral
    : COLOUR IDENTIFIER
    ;

colourKeywordLiteral
    : COLOUR (IDENTIFIER scalarLiteral)+
    ;

transform
    : TRANSLATE (vectorLiteral | IDENTIFIER)
    | ROTATE (vectorLiteral | IDENTIFIER)
    | SCALE (vectorLiteral | scalarLiteral)
    ;

vectorLiteral
    : LEFT_ANGLE scalarLiteral COMMA? scalarLiteral COMMA? scalarLiteral RIGHT_ANGLE
    ;

scalarLiteral
    : signedNumber
    | DASH IDENTIFIER
    | PLUS IDENTIFIER
    | IDENTIFIER
    ;

signedNumber
    : PLUS? FLOAT
    | DASH FLOAT
    ;
