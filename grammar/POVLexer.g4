lexer grammar POVLexer;

// Directives and top-level keywords
DECLARE: '#declare';
INCLUDE: '#include';
SPHERE: 'sphere';
LIGHT_SOURCE: 'light_source';
UNION: 'union';
INTERSECTION: 'intersection';
DIFFERENCE: 'difference';
OBJECT: 'object';
COMPOSITE: 'composite';
FOG: 'fog';
DEFAULT: 'default';
MAX_TRACE_LEVEL: 'max_trace_level';
CAMERA: 'camera' | 'view_point';

// Common scene/object modifiers
BOUNDED_BY: 'bounded_by';
CLIPPED_BY: 'clipped_by';
TRANSLATE: 'translate';
ROTATE: 'rotate';
SCALE: 'scale';
INVERSE: 'inverse';
NO_SHADOW: 'no_shadow';
TEXTURE: 'texture';
COLOUR: 'colour' | 'color';

// Camera fields
LOCATION: 'location';
DIRECTION: 'direction';
UP: 'up';
RIGHT: 'right';
SKY: 'sky';
LOOK_AT: 'look_at';

// Light fields
POINT_AT: 'point_at';
TIGHTNESS: 'tightness';
RADIUS: 'radius';
FALLOFF: 'falloff';
SPOTLIGHT: 'spotlight';

// Punctuation/operators
LEFT_CURLY: '{';
RIGHT_CURLY: '}';
LEFT_ANGLE: '<';
RIGHT_ANGLE: '>';
LEFT_PAREN: '(';
RIGHT_PAREN: ')';
LEFT_SQUARE: '[';
RIGHT_SQUARE: ']';
COMMA: ',';
EQUALS: '=';
PLUS: '+';
DASH: '-';
STAR: '*';
SLASH: '/';

// Literals
FLOAT
    : DIGIT+ '.' DIGIT* EXP?
    | '.' DIGIT+ EXP?
    | DIGIT+ EXP
    | DIGIT+
    ;

STRING
    : '"' ( ESC | ~["\\\r\n] )* '"'
    ;

IDENTIFIER
    : [A-Za-z_] [A-Za-z_0-9]*
    ;

fragment DIGIT: [0-9];
fragment EXP: [eE] [+\-]? DIGIT+;
fragment ESC: '\\' .;

// Comments and whitespace
LINE_COMMENT: '//' ~[\r\n]* -> channel(HIDDEN);
BLOCK_COMMENT: '/*' .*? '*/' -> channel(HIDDEN);
WS: [ \t\r\n\f]+ -> channel(HIDDEN);
