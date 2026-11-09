#include <cmath>

#include "java/lang/Double.h"
#include "java/lang/NumberFormatException.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpression.h"
#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpressionException.h"
#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionBinaryOperatorNode.h"
#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionConstantNode.h"
#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionUnaryOperatorNode.h"
#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionVariableNode.h"

namespace {

java::String charToString(char c)
{
    char buffer[2] = {c, '\0'};
    return java::String(buffer);
}

//= Character classes of java.io.StreamTokenizer, as configured by the Java
//= version of setExpression: resetSyntax, whitespaceChars(' ', ' '),
//= wordChars for letters, digits and '_', ordinaryChar for the operators
//= and parenthesis, and parseNumbers (which adds the "digit" class to the
//= digits, '.' and '-').
bool isWhitespaceClass(char c)
{
    return c == ' ';
}

bool isDigitClass(char c)
{
    return (c >= '0' && c <= '9') || c == '.' || c == '-';
}

bool isAlphaClass(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '_';
}

void deleteNode(_AlgebraicExpressionNode* node)
{
    delete node;
}

}

AlgebraicExpression::AlgebraicExpression() : root(nullptr)
{
    defineValue("PI", M_PI);
    defineValue("E", M_E);
}

AlgebraicExpression::~AlgebraicExpression()
{
    deleteNode(root);
}

void AlgebraicExpression::defineValue(const java::String& name, double val)
{
    values.put(name, val);
}

java::String AlgebraicExpression::toString() const
{
    java::String msg;

    if ( root == nullptr ) {
        msg = "<Invalid Expression>";
    }
    else {
        msg = root->toString();
    }

    return msg;
}

double AlgebraicExpression::getVariableValue(const java::String& name) const
{
    const double* val = values.get(name);
    if ( val == nullptr ) {
        throw AlgebraicExpressionException(
            java::String("AlgebraicExpression.getVariableValue: Variable \"") +
            name + "\" not defined");
    }
    return *val;
}

bool AlgebraicExpression::isOperator(const java::String& cad)
{
    if ( cad.length() != 1 ) return false;
    switch ( cad.charAt(0) ) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '^':
        return true;
      default:
        return false;
    }
}

java::ArrayList<java::String> AlgebraicExpression::tokenize(
    const java::String& text)
{
    // Emulation of java.io.StreamTokenizer.nextToken for the syntax set by
    // the Java version (see character classes above). Number tokens are
    // stored as Java does, with `"" + parser.nval`.
    java::ArrayList<java::String> tokens;
    int n = text.length();
    int position = 0;

    while ( true ) {
        while ( position < n && isWhitespaceClass(text.charAt(position)) ) {
            position++;
        }
        if ( position >= n ) {
            break;
        }
        char c = text.charAt(position);

        if ( isDigitClass(c) ) {
            bool negative = false;
            if ( c == '-' ) {
                char next = position + 1 < n ? text.charAt(position + 1) : '\0';
                if ( next != '.' && (next < '0' || next > '9') ) {
                    // A minus sign that does not start a number
                    tokens.add(charToString('-'));
                    position++;
                    continue;
                }
                negative = true;
                position++;
            }
            double v = 0;
            int decexp = 0;
            int seendot = 0;
            while ( position < n ) {
                c = text.charAt(position);
                if ( c == '.' && seendot == 0 ) {
                    seendot = 1;
                }
                else if ( c >= '0' && c <= '9' ) {
                    v = v * 10 + (c - '0');
                    decexp += seendot;
                }
                else {
                    break;
                }
                position++;
            }
            if ( decexp != 0 ) {
                double denom = 10;
                decexp--;
                while ( decexp > 0 ) {
                    denom *= 10;
                    decexp--;
                }
                v = v / denom;
            }
            tokens.add(java::Double::toString(negative ? -v : v));
        }
        else if ( isAlphaClass(c) ) {
            // Words go on with letters, digits, '_', and also '.' and '-',
            // as parseNumbers gives them the digit class
            int start = position;
            while ( position < n && (isAlphaClass(text.charAt(position)) ||
                                     isDigitClass(text.charAt(position))) ) {
                position++;
            }
            tokens.add(text.substring(start, position));
        }
        else {
            // Ordinary character (quote characters are skipped, as the
            // Java version does)
            if ( c != '\"' ) {
                tokens.add(charToString(c));
            }
            position++;
        }
    }
    return tokens;
}

_AlgebraicExpressionNode* AlgebraicExpression::buildExpressionTree(
    java::ArrayList<java::String>& tokens) const
{
    //-----------------------------------------------------------------
    int i;
    int level;
    char c;

    // The Java version fails here with an IndexOutOfBoundsException
    if ( tokens.size() == 0 ) {
        throw AlgebraicExpressionException("Parse error, empty expression");
    }

    //- Trim out embracing parenthesis from expression ----------------
    while ( tokens.get(0).equals("(") &&
            tokens.get(tokens.size()-1).equals(")") ) {
        level = 0;
        bool trim = true;

        for ( i = 0; i < tokens.size(); i++ ) {
            c = tokens.get(i).charAt(0);
            if ( c == '(' ) {
                level++;
            }
            else if ( c == ')' ) {
                level--;
            }
            if ( level == 0 && i < (tokens.size()-1) ) {
                trim = false;
            }
        }

        if ( !trim ) {
            break;
        }
        tokens.remove((long)(tokens.size()-1));
        tokens.remove((long)0);
        if ( tokens.size() == 0 ) {
            throw AlgebraicExpressionException("Parse error, empty expression");
        }
    }

    //- Trivial case: if I am a single token, I create myself here ----
    if ( tokens.size() == 1 ) {
        java::String token = tokens.get(0);
        if ( isOperator(token) ) {
            throw AlgebraicExpressionException(
                java::String("Parse error, invalid placement for operator \'") +
                token + "\'");
        }
        else if ( (token.charAt(0) >= '0' && token.charAt(0) <= '9') ||
                  token.charAt(0) == '-' ) {
            try {
                return new _AlgebraicExpressionConstantNode(
                    java::Double::parseDouble(token));
            }
            catch ( const java::NumberFormatException& e ) {
                throw AlgebraicExpressionException(e.getMessage());
            }
        }
        else {
            return new _AlgebraicExpressionVariableNode(this, token);
        }
    }

    //- Recursive cases -----------------------------------------------
    java::ArrayList<int> topLevelConnectorIndexes;

    // Search for top level connector operators
    level = 0;
    for ( i = 0; i < tokens.size(); i++ ) {
        c = tokens.get(i).charAt(0);
        if ( c == '(' ) {
            level++;
            continue;
        }
        else if ( c == ')' ) {
            level--;
            continue;
        }
        if ( level == 0 && isOperator(tokens.get(i)) ) {
            topLevelConnectorIndexes.add(i);
        }
    }

    // Determine the top level connector
    if ( topLevelConnectorIndexes.size() <= 0 ) {
        // Try a funcional form expression
        _AlgebraicExpressionUnaryOperatorNode* uo;
        uo = new _AlgebraicExpressionUnaryOperatorNode(this, tokens.get(0));
        tokens.remove((long)0);
        try {
            uo->setOperand(buildExpressionTree(tokens));
        }
        catch ( ... ) {
            delete uo;
            throw;
        }
        return uo;
    }

    int mainConnector = 0;

    // '^' goes with lower priority, will be overwritten if lower
    // precedent operator is later founded
    for ( i = 0; i < topLevelConnectorIndexes.size(); i++ ) {
        c = tokens.get(topLevelConnectorIndexes.get(i)).charAt(0);
        if ( c == '^' ) {
            mainConnector = topLevelConnectorIndexes.get(i);
            break;
        }
    }

    // '*' y '/' goes before `^`
    for ( i = 0; i < topLevelConnectorIndexes.size(); i++ ) {
        c = tokens.get(topLevelConnectorIndexes.get(i)).charAt(0);
        if ( c == '*' || c == '/' ) {
            mainConnector = topLevelConnectorIndexes.get(i);
            break;
        }
    }

    // '+' y '-' goes before the others
    for ( i = 0; i < topLevelConnectorIndexes.size(); i++ ) {
        c = tokens.get(topLevelConnectorIndexes.get(i)).charAt(0);
        if ( c == '+' || c == '-' ) {
            mainConnector = topLevelConnectorIndexes.get(i);
            break;
        }
    }

    char op = tokens.get(mainConnector).charAt(0);

    if ( op == '-' && mainConnector == 0 ) {
        _AlgebraicExpressionUnaryOperatorNode* uo;
        uo = new _AlgebraicExpressionUnaryOperatorNode(this, "-");
        tokens.remove((long)0);
        try {
            uo->setOperand(buildExpressionTree(tokens));
        }
        catch ( ... ) {
            delete uo;
            throw;
        }
        return uo;
    }

    java::ArrayList<java::String> leftTokens;
    for ( i = 0; i < mainConnector && i < tokens.size(); i++ ) {
        leftTokens.add(tokens.get(i));
    }

    java::ArrayList<java::String> rightTokens;
    for ( i = mainConnector + 1; i < tokens.size(); i++ ) {
        rightTokens.add(tokens.get(i));
    }

    _AlgebraicExpressionBinaryOperatorNode* bo;
    bo = new _AlgebraicExpressionBinaryOperatorNode(this, op);
    try {
        bo->setLeftOperand(buildExpressionTree(leftTokens));
        bo->setRightOperand(buildExpressionTree(rightTokens));
    }
    catch ( ... ) {
        delete bo;
        throw;
    }
    return bo;
}

void AlgebraicExpression::setExpression(const java::String& regexp)
{
    java::ArrayList<java::String> tokens = tokenize(regexp);

    //-----------------------------------------------------------------
    _AlgebraicExpressionNode* newRoot = buildExpressionTree(tokens);
    deleteNode(root);
    root = newRoot;
}

double AlgebraicExpression::eval() const
{
    if ( root == nullptr ) {
        throw AlgebraicExpressionException(
            "Null expression, can not evaluate.");
    }
    return root->eval();
}

//= Nodes ===================================================================

_AlgebraicExpressionConstantNode::_AlgebraicExpressionConstantNode(double val)
    : val(val)
{
}

double _AlgebraicExpressionConstantNode::eval() const
{
    return val;
}

java::String _AlgebraicExpressionConstantNode::toString() const
{
    return VSDK::formatDouble(val);
}

_AlgebraicExpressionVariableNode::_AlgebraicExpressionVariableNode(
    const AlgebraicExpression* parent, const java::String& name)
    : parent(parent), name(name)
{
}

double _AlgebraicExpressionVariableNode::eval() const
{
    return parent->getVariableValue(name);
}

java::String _AlgebraicExpressionVariableNode::toString() const
{
    return name;
}

_AlgebraicExpressionUnaryOperatorNode::_AlgebraicExpressionUnaryOperatorNode(
    const AlgebraicExpression* parent, const java::String& operatorName)
    : parent(parent), operatorName(operatorName), operand(nullptr)
{
}

_AlgebraicExpressionUnaryOperatorNode::~_AlgebraicExpressionUnaryOperatorNode()
{
    delete operand;
}

void _AlgebraicExpressionUnaryOperatorNode::setOperand(
    _AlgebraicExpressionNode* operand)
{
    if ( this->operand != operand ) {
        delete this->operand;
    }
    this->operand = operand;
}

double _AlgebraicExpressionUnaryOperatorNode::eval() const
{
    double operandValue = operand->eval();
    double val = NAN;

    if ( operatorName.equals("sin") ) {
        val = std::sin(operandValue);
    }
    else if ( operatorName.equals("cos") ) {
        val = std::cos(operandValue);
    }
    else if ( operatorName.equals("tan") ) {
        val = std::tan(operandValue);
    }
    else if ( operatorName.equals("asin") ) {
        val = std::asin(operandValue);
    }
    else if ( operatorName.equals("acos") ) {
        val = std::acos(operandValue);
    }
    else if ( operatorName.equals("atan") ) {
        val = std::atan(operandValue);
    }
    else if ( operatorName.equals("abs") ) {
        val = std::fabs(operandValue);
    }
    else if ( operatorName.equals("cbrt") ) {
        //val = Math.cbrt(operandValue);
        val = std::pow(operandValue, 1.0/3.0);
    }
    else if ( operatorName.equals("ceil") ) {
        val = std::ceil(operandValue);
    }
    else if ( operatorName.equals("sinh") ) {
        val = std::sinh(operandValue);
    }
    else if ( operatorName.equals("cosh") ) {
        val = std::cosh(operandValue);
    }
    else if ( operatorName.equals("tanh") ) {
        val = std::tanh(operandValue);
    }
    else if ( operatorName.equals("toDegrees") ) {
        // Same constant factors as java.lang.Math
        val = operandValue * (180.0 / M_PI);
    }
    else if ( operatorName.equals("toRadians") ) {
        val = operandValue * (M_PI / 180.0);
    }
    else if ( operatorName.equals("exp") ) {
        val = std::exp(operandValue);
    }
    else if ( operatorName.equals("floor") ) {
        val = std::floor(operandValue);
    }
    else if ( operatorName.equals("log") ) {
        val = std::log(operandValue);
    }
    else if ( operatorName.equals("ln") ) {
        val = std::log(operandValue);
    }
    else if ( operatorName.equals("log10") ) {
        val = std::log10(operandValue);
    }
    else if ( operatorName.equals("sqrt") ) {
        val = std::sqrt(operandValue);
    }
    else if ( operatorName.equals("-") ) {
        val = -operandValue;
    }
    else {
        throw AlgebraicExpressionException(
            java::String("Unknown unary operator or function \"") +
            operatorName + "\"");
    }
    return val;
}

java::String _AlgebraicExpressionUnaryOperatorNode::toString() const
{
    java::String msg;

    msg = operatorName + "(";
    msg += operand->toString();
    msg += ")";
    return msg;
}

_AlgebraicExpressionBinaryOperatorNode::_AlgebraicExpressionBinaryOperatorNode(
    const AlgebraicExpression* parent, char op)
    : parent(parent), operatorChar(op), leftOperand(nullptr),
      rightOperand(nullptr)
{
}

_AlgebraicExpressionBinaryOperatorNode::~_AlgebraicExpressionBinaryOperatorNode()
{
    delete leftOperand;
    delete rightOperand;
}

void _AlgebraicExpressionBinaryOperatorNode::setLeftOperand(
    _AlgebraicExpressionNode* operand)
{
    if ( leftOperand != operand ) {
        delete leftOperand;
    }
    leftOperand = operand;
}

void _AlgebraicExpressionBinaryOperatorNode::setRightOperand(
    _AlgebraicExpressionNode* operand)
{
    if ( rightOperand != operand ) {
        delete rightOperand;
    }
    rightOperand = operand;
}

double _AlgebraicExpressionBinaryOperatorNode::eval() const
{
    double lval = leftOperand->eval();
    double rval = rightOperand->eval();
    double val;

    switch ( operatorChar ) {
      case '+':    val = lval + rval;    break;
      case '-':    val = lval - rval;    break;
      case '*':    val = lval * rval;    break;
      case '/':    val = lval / rval;    break;
      case '^':    val = std::pow(lval, rval);    break;
      default:
        throw AlgebraicExpressionException(
            java::String("Unknown binary operator \"") +
            charToString(operatorChar) + "\"");
    }
    return val;
}

java::String _AlgebraicExpressionBinaryOperatorNode::toString() const
{
    return java::String("(") + leftOperand->toString() + ") " +
        charToString(operatorChar) + " (" + rightOperand->toString() + ")";
}
