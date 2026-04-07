#include <catch.hpp>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/visitor.hpp"
#include <algorithm>
#include <cctype>
#include <string>

static std::string strip_spaces(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            res += c;
        }
    }
    return res;
}

static bool contains_token(const std::string& expr, const std::string& token) {
    return expr.find(token) != std::string::npos;
}

static std::string get_derivative_string(const std::string& expr, const std::string& var) {
    Lexer lexer(expr);
    Parser parser(lexer);
    AST tree = parser.parse();

    Derivative deriv_visitor(var);
    tree.get_root()->accept(deriv_visitor);
    AST derivative = deriv_visitor.get_result();

    return strip_spaces(print_ast(derivative.get_root()));
}

TEST_CASE("Derivative: Constants and Variables", "[derivative_symbolic]") {
    SECTION("DS01: Derivative of constant is 0") {
        REQUIRE(get_derivative_string("5", "x") == "0");
        REQUIRE(get_derivative_string("3.14", "x") == "0");
        REQUIRE(contains_token(get_derivative_string("-10", "x"), "0"));
    }

    SECTION("DS02: Derivative of variable x by x is 1") {
        REQUIRE(get_derivative_string("x", "x") == "1");
    }

    SECTION("DS03: Derivative of variable y by x is 0") {
        REQUIRE(get_derivative_string("y", "x") == "0");
        REQUIRE(get_derivative_string("z", "x") == "0");
    }
}

TEST_CASE("Derivative: Power Rule", "[derivative_symbolic]") {
    SECTION("DS04: d/dx(x^2) structure") {
        REQUIRE(get_derivative_string("x^2", "x") == "2*x");
    }

    SECTION("DS05: d/dx(x^n) structure") {
        REQUIRE(get_derivative_string("x^5", "x") == "5*x^4");
    }

    SECTION("DS06: d/dx(x^0) simplifies to zero") {
        REQUIRE(get_derivative_string("x^0", "x") == "0");
    }
}

TEST_CASE("Derivative: Sum and Difference Rules", "[derivative_symbolic]") {
    SECTION("DS07: d/dx(x + c) current output") {
        REQUIRE(get_derivative_string("x+5", "x") == "1");
    }

    SECTION("DS08: d/dx(c - x) current output") {
        REQUIRE(get_derivative_string("5-x", "x") == "-1");
    }

    SECTION("DS09: d/dx(x + y) by x current output") {
        REQUIRE(get_derivative_string("x+y", "x") == "1");
    }
}

TEST_CASE("Derivative: Product Rule", "[derivative_symbolic]") {
    SECTION("DS10: d/dx(x * x) current output") {
        REQUIRE(get_derivative_string("x*x", "x") == "x+x");
    }

    SECTION("DS11: d/dx(x * y) by x current output") {
        REQUIRE(get_derivative_string("x*y", "x") == "y");
    }

    SECTION("DS12: d/dx(x * y) by y current output") {
        REQUIRE(get_derivative_string("x*y", "y") == "x");
    }
}

TEST_CASE("Derivative: Quotient Rule", "[derivative_symbolic]") {
    SECTION("DS13: d/dx(1 / x)") {
        REQUIRE(get_derivative_string("1/x", "x") == "-1/x^2");
    }
}

TEST_CASE("Derivative: Trigonometric Functions", "[derivative_symbolic]") {
    SECTION("DS14: d/dx(sin(x)) current output") {
        REQUIRE(get_derivative_string("sin(x)", "x") == "cos(x)");
    }

    SECTION("DS15: d/dx(cos(x)) current output") {
        REQUIRE(get_derivative_string("cos(x)", "x") == "-sin(x)");
    }

    SECTION("DS16: d/dx(tan(x))") {
        REQUIRE(get_derivative_string("tan(x)", "x") == "1/cos(x)^2");
    }
}

TEST_CASE("Derivative: Logarithmic and Exponential", "[derivative_symbolic]") {
    SECTION("DS17: d/dx(ln(x)) current output") {
        REQUIRE(get_derivative_string("ln(x)", "x") == "1/x");
    }

    SECTION("DS18: d/dx(exp(x)) current output") {
        REQUIRE(get_derivative_string("exp(x)", "x") == "exp(x)");
    }
}

TEST_CASE("Derivative: Chain Rule", "[derivative_symbolic]") {
    SECTION("DS19: d/dx(sin(2*x))") {
        REQUIRE(get_derivative_string("sin(2*x)", "x") == "2*cos(2*x)");
    }

    SECTION("DS20: d/dx(exp(x^2))") {
        REQUIRE(get_derivative_string("exp(x^2)", "x") == "2*x*exp(x^2)");
    }

    SECTION("DS21: Nested chain sin(cos(x))") {
        REQUIRE(get_derivative_string("sin(cos(x))", "x") == "-(cos(cos(x))*sin(x))");
    }

    SECTION("DS22: Simplified readable derivative for nested polynomial chain") {
        REQUIRE(get_derivative_string("2+4+x+5*cos(12*x^4)", "x") == "1-240*x^3*sin(12*x^4)");
    }
}
