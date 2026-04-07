#include <catch.hpp>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/visitor.hpp"
#include <unordered_map>
#include <cmath>
#include <limits>
#include <string>

static double evaluate_expression_at_point(const std::string& expr,
                                           std::unordered_map<std::string, double> vars) {
    Lexer lexer(expr);
    Parser parser(lexer);
    AST tree = parser.parse();

    Evaluator evaluator(vars);
    tree.get_root()->accept(evaluator);

    return evaluator.get_result();
}

static double evaluate_derivative_at_point(const std::string& expr,
                                           const std::string& variable,
                                           std::unordered_map<std::string, double> vars) {
    Lexer lexer(expr);
    Parser parser(lexer);
    AST tree = parser.parse();

    Derivative deriv_visitor(variable);
    tree.get_root()->accept(deriv_visitor);
    AST derivative_tree = deriv_visitor.get_result();

    Evaluator evaluator(vars);
    derivative_tree.get_root()->accept(evaluator);

    return evaluator.get_result();
}

static double central_difference(const std::string& expr,
                                 const std::string& variable,
                                 std::unordered_map<std::string, double> vars,
                                 double step = 1e-6) {
    double point = vars.at(variable);
    std::unordered_map<std::string, double> vars_plus = vars;
    std::unordered_map<std::string, double> vars_minus = vars;

    vars_plus[variable] = point + step;
    vars_minus[variable] = point - step;

    return (evaluate_expression_at_point(expr, vars_plus) - evaluate_expression_at_point(expr, vars_minus)) / (2.0 * step);
}

static bool approx(double a, double b, double eps = 1e-5) {
    return std::abs(a - b) < eps;
}

static void require_derivative_value(const std::string& expr,
                                     const std::string& variable,
                                     std::unordered_map<std::string, double> vars,
                                     double expected,
                                     double eps = 1e-5) {
    INFO("expr = " << expr << ", variable = " << variable);
    REQUIRE(approx(evaluate_derivative_at_point(expr, variable, vars), expected, eps));
}

static void require_matches_central_difference(const std::string& expr,
                                               const std::string& variable,
                                               std::unordered_map<std::string, double> vars,
                                               double eps = 1e-4,
                                               double step = 1e-6) {
    INFO("expr = " << expr << ", variable = " << variable);
    double derivative_value = evaluate_derivative_at_point(expr, variable, vars);
    double difference_value = central_difference(expr, variable, vars, step);
    REQUIRE(approx(derivative_value, difference_value, eps));
}

TEST_CASE("Derivative Numeric: Basic Rules", "[derivative_numeric]") {
    SECTION("DN01: Constant derivative is 0 everywhere") {
        require_derivative_value("42", "x", {{"x", 1.0}}, 0.0);
        require_derivative_value("3.14", "x", {{"x", 100.0}}, 0.0);
    }

    SECTION("DN02: Linear function d/dx(ax+b) = a") {
        require_derivative_value("2*x+3", "x", {{"x", 5.0}}, 2.0);
        require_derivative_value("-x", "x", {{"x", 1.0}}, -1.0);
    }

    SECTION("DN03: Power rule d/dx(x^n) = n*x^(n-1)") {
        require_derivative_value("x^2", "x", {{"x", 3.0}}, 6.0);
        require_derivative_value("x^3", "x", {{"x", 2.0}}, 12.0);
        require_derivative_value("x^0.5", "x", {{"x", 4.0}}, 0.25);
    }
}

TEST_CASE("Derivative Numeric: Trigonometry", "[derivative_numeric]") {
    SECTION("DN04: d/dx(sin(x)) = cos(x)") {
        require_derivative_value("sin(x)", "x", {{"x", 0.0}}, 1.0);
        require_derivative_value("sin(x)", "x", {{"x", M_PI_2}}, 0.0, 1e-4);
    }

    SECTION("DN05: d/dx(cos(x)) = -sin(x)") {
        require_derivative_value("cos(x)", "x", {{"x", 0.0}}, 0.0);
        require_derivative_value("cos(x)", "x", {{"x", M_PI_2}}, -1.0);
    }

    SECTION("DN06: d/dx(tan(x)) = sec^2(x)") {
        require_derivative_value("tan(x)", "x", {{"x", 0.0}}, 1.0);
        require_derivative_value("tan(x)", "x", {{"x", M_PI_4}}, 2.0);
    }

    SECTION("DN07: d/dx(ctg(x)) = -csc^2(x)") {
        require_derivative_value("ctg(x)", "x", {{"x", M_PI_4}}, -2.0);
    }
}

TEST_CASE("Derivative Numeric: Exponentials and Logs", "[derivative_numeric]") {
    SECTION("DN08: d/dx(exp(x)) = exp(x)") {
        require_derivative_value("exp(x)", "x", {{"x", 0.0}}, 1.0);
        require_derivative_value("exp(x)", "x", {{"x", 1.0}}, M_E);
    }

    SECTION("DN09: d/dx(ln(x)) = 1/x") {
        require_derivative_value("ln(x)", "x", {{"x", 1.0}}, 1.0);
        require_derivative_value("ln(x)", "x", {{"x", 2.0}}, 0.5);
    }

    SECTION("DN10: d/dx(lg(x)) = 1/(x*ln(10))") {
        require_derivative_value("lg(x)", "x", {{"x", 10.0}}, 1.0 / (10.0 * std::log(10.0)));
    }

    SECTION("DN11: d/dx(log(2, x)) = 1/(x*ln(2))") {
        require_derivative_value("log(2,x)", "x", {{"x", 8.0}}, 1.0 / (8.0 * std::log(2.0)));
    }
}

TEST_CASE("Derivative Numeric: Chain Rule", "[derivative_numeric]") {
    SECTION("DN12: d/dx(sin(2x)) = 2*cos(2x)") {
        require_derivative_value("sin(2*x)", "x", {{"x", 0.0}}, 2.0);
        require_derivative_value("sin(2*x)", "x", {{"x", M_PI_4}}, 0.0, 1e-4);
    }

    SECTION("DN13: d/dx(exp(x^2)) = 2x*exp(x^2)") {
        require_derivative_value("exp(x^2)", "x", {{"x", 1.0}}, 2.0 * M_E);
    }

    SECTION("DN14: d/dx(ln(sin(x))) = cot(x)") {
        require_derivative_value("ln(sin(x))", "x", {{"x", M_PI_4}}, 1.0);
    }

    SECTION("DN15: d/dx(sqrt(x)) = 1/(2*sqrt(x))") {
        require_derivative_value("sqrt(x)", "x", {{"x", 9.0}}, 1.0 / 6.0);
    }

    SECTION("DN16: d/dx(abs(x)) away from zero") {
        require_derivative_value("abs(x)", "x", {{"x", 3.0}}, 1.0);
        require_derivative_value("abs(x)", "x", {{"x", -3.0}}, -1.0);
    }
}

TEST_CASE("Derivative Numeric: Product and Quotient Rules", "[derivative_numeric]") {
    SECTION("DN17: d/dx(x*sin(x)) = sin(x) + x*cos(x)") {
        require_derivative_value("x*sin(x)", "x", {{"x", 0.0}}, 0.0);
        require_derivative_value("x*sin(x)", "x", {{"x", M_PI}}, -M_PI);
    }

    SECTION("DN18: d/dx(x/(1+x)) = 1/(1+x)^2") {
        require_derivative_value("x/(1+x)", "x", {{"x", 1.0}}, 0.25);
        require_derivative_value("x/(1+x)", "x", {{"x", 2.0}}, 1.0 / 9.0);
    }
}

TEST_CASE("Derivative Numeric: Multivariable Partial Derivatives", "[derivative_numeric]") {
    SECTION("DN19: Partial d/dx(x*y) = y") {
        std::unordered_map<std::string, double> vars = {{"x", 2.0}, {"y", 3.0}};
        require_derivative_value("x*y", "x", vars, 3.0);
    }

    SECTION("DN20: Partial d/dy(x*y) = x") {
        std::unordered_map<std::string, double> vars = {{"x", 2.0}, {"y", 3.0}};
        require_derivative_value("x*y", "y", vars, 2.0);
    }

    SECTION("DN21: Partial d/dx(x^2 + y^2) = 2x") {
        std::unordered_map<std::string, double> vars = {{"x", 3.0}, {"y", 4.0}};
        require_derivative_value("x^2+y^2", "x", vars, 6.0);
    }

    SECTION("DN22: Partial d/dy(x^2 + y^2) = 2y") {
        std::unordered_map<std::string, double> vars = {{"x", 3.0}, {"y", 4.0}};
        require_derivative_value("x^2+y^2", "y", vars, 8.0);
    }

    SECTION("DN23: Complex multivariable d/dx(sin(x)*y + cos(y)*x)") {
        std::unordered_map<std::string, double> vars = {{"x", 0.0}, {"y", 1.0}};
        double expected = std::cos(0.0) + std::cos(1.0);
        require_derivative_value("sin(x)*y+cos(y)*x", "x", vars, expected);
    }

    SECTION("DN24: Partial d/dy(sin(x*y)) = x*cos(x*y)") {
        std::unordered_map<std::string, double> vars = {{"x", 2.0}, {"y", 0.5}};
        require_derivative_value("sin(x*y)", "y", vars, 2.0 * std::cos(1.0));
    }
}

TEST_CASE("Derivative Numeric: Finite Difference Validation", "[derivative_numeric][finite_difference]") {
    SECTION("DN25: Polynomial agrees with central difference") {
        require_matches_central_difference("x^4+3*x^2-7*x+10", "x", {{"x", 1.25}});
    }

    SECTION("DN26: Nested trigonometric composition agrees with central difference") {
        require_matches_central_difference("sin(cos(exp(x)))", "x", {{"x", 0.1}}, 5e-4);
    }

    SECTION("DN27: Logarithmic composition agrees with central difference") {
        require_matches_central_difference("lg(x^2+1)", "x", {{"x", 0.7}});
    }

    SECTION("DN28: Root composition agrees with central difference") {
        require_matches_central_difference("sqrt(x^2+4)", "x", {{"x", 1.3}});
    }

    SECTION("DN29: Absolute value away from kink agrees with central difference") {
        require_matches_central_difference("abs(x^2-4*x)", "x", {{"x", 5.0}}, 5e-4);
        require_matches_central_difference("abs(x^2-4*x)", "x", {{"x", 1.0}}, 5e-4);
    }

    SECTION("DN30: Inverse trig composition agrees with central difference") {
        require_matches_central_difference("asin(x/2)", "x", {{"x", 0.4}}, 5e-4);
        require_matches_central_difference("atan(sin(x))", "x", {{"x", 0.3}}, 5e-4);
    }

    SECTION("DN31: pow(x, x) agrees with central difference") {
        require_matches_central_difference("pow(x,x)", "x", {{"x", 2.0}}, 5e-4);
    }

    SECTION("DN32: pow(x, y) partial by x agrees with central difference") {
        require_matches_central_difference("pow(x,y)", "x", {{"x", 2.0}, {"y", 3.0}}, 5e-4);
    }

    SECTION("DN33: pow(x, y) partial by y agrees with central difference") {
        require_matches_central_difference("pow(x,y)", "y", {{"x", 2.0}, {"y", 3.0}}, 5e-4);
    }

    SECTION("DN34: log(2, x^2+1) agrees with central difference") {
        require_matches_central_difference("log(2,x^2+1)", "x", {{"x", 1.5}}, 5e-4);
    }

    SECTION("DN35: Quotient with polynomial numerator agrees with central difference") {
        require_matches_central_difference("(x^2+1)/(x+2)", "x", {{"x", 0.6}}, 5e-4);
    }

    SECTION("DN36: Multivariable chain by x agrees with central difference") {
        require_matches_central_difference("x*y+sin(x*y)", "x", {{"x", 1.2}, {"y", 0.7}}, 5e-4);
    }

    SECTION("DN37: Multivariable chain by y agrees with central difference") {
        require_matches_central_difference("x*y+sin(x*y)", "y", {{"x", 1.2}, {"y", 0.7}}, 5e-4);
    }
}

TEST_CASE("Derivative Numeric: Edge Cases and Errors", "[derivative_numeric]") {
    SECTION("DN38: High power polynomial") {
        require_derivative_value("x^10", "x", {{"x", 2.0}}, 5120.0);
    }

    SECTION("DN39: Nested chain deep") {
        require_derivative_value("sin(sin(sin(x)))", "x", {{"x", 0.0}}, 1.0);
    }

    SECTION("DN40: Different variable gives zero derivative") {
        require_derivative_value("sin(x)", "y", {{"x", 1.0}, {"y", 2.0}}, 0.0);
    }

    SECTION("DN41: Variadic min derivative is rejected") {
        REQUIRE_THROWS_AS(evaluate_derivative_at_point("min(x,1)", "x", {{"x", 2.0}}), std::runtime_error);
    }

    SECTION("DN42: Variadic max derivative is rejected") {
        REQUIRE_THROWS_AS(evaluate_derivative_at_point("max(x,1)", "x", {{"x", 2.0}}), std::runtime_error);
    }
}
