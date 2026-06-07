#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpression.h"
#include "vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpressionException.h"
#include <cctype>
#include <cmath>
#include <cstdlib>

class Parser {
private:
    const java::String& s;
    const java::HashMap<java::String, double>& vars;
    mutable size_t p;

    void skip() const { while (p < s.size() && std::isspace((unsigned char)s[p])) p++; }
    bool consume(char c) const { skip(); if (p < s.size() && s[p] == c) { p++; return true; } return false; }
    java::String id() const {
        skip(); size_t b = p;
        if (p < s.size() && (std::isalpha((unsigned char)s[p]) || s[p] == '_')) {
            p++;
            while (p < s.size() && (std::isalnum((unsigned char)s[p]) || s[p] == '_')) p++;
        }
        return s.substr(b, p-b);
    }
    double num() const {
        skip(); const char* st = s.c_str() + p; char* end = 0;
        double v = std::strtod(st, &end);
        if (end == st) throw AlgebraicExpressionException("Invalid number");
        p += (size_t)(end - st);
        return v;
    }
    double fn(const java::String& f, double v) const {
        if (f == "sin") return std::sin(v); if (f == "cos") return std::cos(v);
        if (f == "tan") return std::tan(v); if (f == "asin") return std::asin(v);
        if (f == "acos") return std::acos(v); if (f == "atan") return std::atan(v);
        if (f == "exp") return std::exp(v); if (f == "log") return std::log(v);
        if (f == "sqrt") return std::sqrt(v); if (f == "abs") return std::fabs(v);
        throw AlgebraicExpressionException("Unknown function: " + f);
    }
    double primary() const {
        skip();
        if (consume('(')) { double v = expr(); if (!consume(')')) throw AlgebraicExpressionException("Missing ')' "); return v; }
        if (p < s.size() && (std::isdigit((unsigned char)s[p]) || s[p] == '.')) return num();
        java::String name = id();
        if (name.empty()) throw AlgebraicExpressionException("Unexpected token");
        if (name == "pi") return 3.14159265358979323846;
        if (name == "e") return 2.71828182845904523536;
        double varValue = 0.0;
        if (vars.tryGet(name, &varValue)) return varValue;
        if (consume('(')) { double a = expr(); if (!consume(')')) throw AlgebraicExpressionException("Missing ')' in function"); return fn(name, a); }
        throw AlgebraicExpressionException(java::String("Unknown variable: ").concat(name).toCString());
    }
    double unary() const { if (consume('+')) return unary(); if (consume('-')) return -unary(); return primary(); }
    double power() const { double a = unary(); if (consume('^')) return std::pow(a, power()); return a; }
    double term() const { double a = power(); while (true) { if (consume('*')) a *= power(); else if (consume('/')) a /= power(); else break; } return a; }
    double expr() const { double a = term(); while (true) { if (consume('+')) a += term(); else if (consume('-')) a -= term(); else break; } return a; }

public:
    Parser(const java::String& s, const java::HashMap<java::String, double>& vars) : s(s), vars(vars), p(0) {}
    double eval() const { double v = expr(); skip(); if (p != s.size()) throw AlgebraicExpressionException("Trailing input"); return v; }
};

void AlgebraicExpression::setExpression(const java::String& expr) { expression = expr; }
void AlgebraicExpression::defineValue(const java::String& name, double value) { vars.put(name, value); }
double AlgebraicExpression::eval() const { return Parser(expression, vars).eval(); }
