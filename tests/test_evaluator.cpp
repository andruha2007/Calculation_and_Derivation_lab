// tests.cpp - Comprehensive test suite (90 tests)
#include <catch.hpp>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/visitor.hpp"
#include <unordered_map>
#include <cmath>
#include <limits>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static double evaluate(const std::string& expr, 
                      std::unordered_map<std::string, double> vars = {}) {
    Lexer lexer(expr);
    Parser parser(lexer);
    AST tree = parser.parse();
    Evaluator evaluator(vars);
    tree.get_root()->accept(evaluator);
    return evaluator.get_result();
}

static bool approx(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) < eps;
}

// ============================================================================
// LEXER TESTS (30 tests)
// ============================================================================

TEST_CASE("Lexer: Tokenization", "[lexer]") {
    
    // ===== POSITIVE TESTS (20) =====
    
    SECTION("L01: Integer number") {
        REQUIRE(lexer_flow_concatenator("42") == "42\n");
    }
    SECTION("L02: Zero") {
        REQUIRE(lexer_flow_concatenator("0") == "0\n");
    }
    SECTION("L03: Large integer") {
        REQUIRE(lexer_flow_concatenator("123456789") == "123456789\n");
    }
    SECTION("L04: Float with decimal") {
        REQUIRE(lexer_flow_concatenator("3.14") == "3.14\n");
    }
    SECTION("L05: Float starting with zero") {
        REQUIRE(lexer_flow_concatenator("0.5") == "0.5\n");
    }
    SECTION("L06: Scientific notation lowercase e") {
        REQUIRE(lexer_flow_concatenator("1e5") == "1e5\n");
    }
    SECTION("L07: Scientific notation with sign") {
        REQUIRE(lexer_flow_concatenator("2.5e-3") == "2.5e-3\n");
    }
    SECTION("L08: Scientific notation with plus") {
        REQUIRE(lexer_flow_concatenator("1.23e+10") == "1.23e+10\n");
    }
    SECTION("L09: Simple variable") {
        REQUIRE(lexer_flow_concatenator("x") == "x\n");
    }
    SECTION("L10: Variable with underscore") {
        REQUIRE(lexer_flow_concatenator("my_var") == "my_var\n");
    }
    SECTION("L11: Function name") {
        REQUIRE(lexer_flow_concatenator("sin") == "sin\n");
    }
    SECTION("L12: Addition operator") {
        REQUIRE(lexer_flow_concatenator("+") == "+\n");
    }
    SECTION("L13: Multiplication operator") {
        REQUIRE(lexer_flow_concatenator("*") == "*\n");
    }
    SECTION("L14: Power operator") {
        REQUIRE(lexer_flow_concatenator("^") == "^\n");
    }
    SECTION("L15: Comma operator") {
        REQUIRE(lexer_flow_concatenator(",") == ",\n");
    }
    SECTION("L16: Left parenthesis") {
        REQUIRE(lexer_flow_concatenator("(") == "(\n");
    }
    SECTION("L17: Right parenthesis") {
        REQUIRE(lexer_flow_concatenator(")") == ")\n");
    }
    SECTION("L18: Complex expression") {
        std::string exp = "2.5+3*sin(x)";
        std::string expected = "2.5\n+\n3\n*\nsin\n(\nx\n)\n";
        REQUIRE(lexer_flow_concatenator(exp) == expected);
    }
    SECTION("L19: Whitespace trimming") {
        REQUIRE(lexer_flow_concatenator("  2  +  3  ") == "2\n+\n3\n");
    }
    SECTION("L20: Multiple operators") {
        REQUIRE(lexer_flow_concatenator("+-*/^") == "+\n-\n*\n/\n^\n");
    }
    
    // ===== NEGATIVE TESTS (10) =====
    
    SECTION("L21: Leading space") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator(" 2"), std::runtime_error);
    }
    SECTION("L22: Unknown character @") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("2@3"), std::runtime_error);
    }
    SECTION("L23: Unknown character $") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("$x"), std::runtime_error);
    }
    SECTION("L24: Multiple decimal points") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("1.2.3"), std::runtime_error);
    }
    SECTION("L25: Incomplete scientific: 1e") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("1e"), std::runtime_error);
    }
    SECTION("L26: Incomplete scientific: 1e+") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("1e+"), std::runtime_error);
    }
    SECTION("L27: Leading zero in integer: 007") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("007"), std::runtime_error);
    }
    SECTION("L28: Leading zero in float: 002.5") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("002.5"), std::runtime_error);
    }
    SECTION("L29: Trailing decimal point") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("5."), std::runtime_error);
    }
    SECTION("L30: Double sign in exponent: 1e++5") {
        REQUIRE_THROWS_AS(lexer_flow_concatenator("1e++5"), std::runtime_error);
    }
}

// ============================================================================
// PARSER TESTS (30 tests)
// ============================================================================

TEST_CASE("Parser: AST construction and syntax", "[parser]") {
    
    // ===== POSITIVE TESTS (20) =====
    
    SECTION("P01: Single number") {
        REQUIRE(approx(evaluate("42"), 42.0));
    }
    SECTION("P02: Single variable") {
        REQUIRE(approx(evaluate("x", {{"x", 5.0}}), 5.0));
    }
    SECTION("P03: Simple addition") {
        REQUIRE(approx(evaluate("2+3"), 5.0));
    }
    SECTION("P04: Simple subtraction") {
        REQUIRE(approx(evaluate("10-4"), 6.0));
    }
    SECTION("P05: Simple multiplication") {
        REQUIRE(approx(evaluate("4*5"), 20.0));
    }
    SECTION("P06: Simple division") {
        REQUIRE(approx(evaluate("15/3"), 5.0));
    }
    SECTION("P07: Power operator") {
        REQUIRE(approx(evaluate("2^3"), 8.0));
    }
    SECTION("P08: Precedence: * before +") {
        REQUIRE(approx(evaluate("2+3*5"), 17.0));
    }
    SECTION("P09: Precedence: ^ before *") {
        REQUIRE(approx(evaluate("2*3^2"), 18.0));
    }
    SECTION("P10: Left associativity: -") {
        REQUIRE(approx(evaluate("10-5-2"), 3.0));  // (10-5)-2
    }
    SECTION("P11: Right associativity: ^") {
        REQUIRE(approx(evaluate("2^3^2"), 512.0));  // 2^(3^2)
    }
    SECTION("P12: Unary minus") {
        REQUIRE(approx(evaluate("-5"), -5.0));
    }
    SECTION("P13: Unary plus") {
        REQUIRE(approx(evaluate("+10"), 10.0));
    }
    SECTION("P14: Unary in expression") {
        REQUIRE(approx(evaluate("2+-3"), -1.0));
    }
    SECTION("P15: Parentheses override precedence") {
        REQUIRE(approx(evaluate("(2+3)*5"), 25.0));
    }
    SECTION("P16: Nested parentheses") {
        REQUIRE(approx(evaluate("((2+3)*4)"), 20.0));
    }
    SECTION("P17: Function sin") {
        REQUIRE(approx(evaluate("sin(0)"), 0.0));
    }
    SECTION("P18: Function with expression arg") {
        REQUIRE(approx(evaluate("sin(2+3)"), std::sin(5.0)));
    }
    SECTION("P19: Two-arg function log") {
        REQUIRE(approx(evaluate("log(2,8)"), 3.0));  // log_2(8)
    }
    SECTION("P20: Variadic function min") {
        REQUIRE(approx(evaluate("min(5,2,8,1)"), 1.0));
    }
    
    // ===== NEGATIVE TESTS (10) =====
    
    SECTION("P21: Empty expression") {
        REQUIRE_THROWS_AS(evaluate(""), std::runtime_error);
    }
    SECTION("P22: Mismatched parentheses: missing )") {
        REQUIRE_THROWS_AS(evaluate("(2+3"), std::runtime_error);
    }
    SECTION("P23: Mismatched parentheses: missing (") {
        REQUIRE_THROWS_AS(evaluate("2+3)"), std::runtime_error);
    }
    SECTION("P24: Empty parentheses") {
        REQUIRE_THROWS_AS(evaluate("()"), std::runtime_error);
    }
    SECTION("P25: Unexpected comma at start") {
        REQUIRE_THROWS_AS(evaluate(",2"), std::runtime_error);
    }
    SECTION("P26: Double comma") {
        REQUIRE_THROWS_AS(evaluate("2,,3"), std::runtime_error);
    }
    SECTION("P27: Trailing operator") {
        REQUIRE_THROWS_AS(evaluate("2+"), std::runtime_error);
    }
    SECTION("P28: Function name as variable") {
        REQUIRE_THROWS_AS(evaluate("sin+x", {{"sin", 5.0}}), std::runtime_error);
    }
    SECTION("P29: Wrong arg count: sin(1,2)") {
        REQUIRE_THROWS_AS(evaluate("sin(1,2)"), std::runtime_error);
    }
    SECTION("P30: Wrong arg count: log(2)") {
        REQUIRE_THROWS_AS(evaluate("log(2)"), std::runtime_error);
    }
}

// ============================================================================
// EVALUATOR TESTS (30 tests)
// ============================================================================

TEST_CASE("Evaluator: Computation and errors", "[evaluator]") {
    
    // ===== POSITIVE TESTS: Arithmetic (12) =====
    
    SECTION("E01: Addition") {
        REQUIRE(approx(evaluate("2+3"), 5.0));
    }
    SECTION("E02: Subtraction") {
        REQUIRE(approx(evaluate("10-4"), 6.0));
    }
    SECTION("E03: Multiplication") {
        REQUIRE(approx(evaluate("4*5"), 20.0));
    }
    SECTION("E04: Division") {
        REQUIRE(approx(evaluate("15/3"), 5.0));
    }
    SECTION("E05: Float division") {
        REQUIRE(approx(evaluate("7.5/2.5"), 3.0));
    }
    SECTION("E06: Power: integer") {
        REQUIRE(approx(evaluate("2^3"), 8.0));
    }
    SECTION("E07: Power: zero exponent") {
        REQUIRE(approx(evaluate("5^0"), 1.0));
    }
    SECTION("E08: Power: negative exponent") {
        REQUIRE(approx(evaluate("2^-1"), 0.5));
    }
    SECTION("E09: Mixed precedence") {
        REQUIRE(approx(evaluate("2+3*4-5"), 9.0));
    }
    SECTION("E10: Complex mixed") {
        REQUIRE(approx(evaluate("10/2+3*2"), 11.0));
    }
    SECTION("E11: Nested with vars") {
        std::unordered_map<std::string, double> vars = {{"x", 2.0}, {"y", 3.0}};
        REQUIRE(approx(evaluate("x*x+y*2", vars), 10.0));
    }
    SECTION("E12: Precision requirement") {
        double result = evaluate("1.0/3.0 * 3.0");
        REQUIRE(approx(result, 1.0, 1e-6));
    }
    
    // ===== POSITIVE TESTS: Functions (8) =====
    
    SECTION("E13: sin(0)") {
        REQUIRE(approx(evaluate("sin(0)"), 0.0));
    }
    SECTION("E14: cos(0)") {
        REQUIRE(approx(evaluate("cos(0)"), 1.0));
    }
    SECTION("E15: tan(0)") {
        REQUIRE(approx(evaluate("tan(0)"), 0.0));
    }
    SECTION("E16: sqrt(25)") {
        REQUIRE(approx(evaluate("sqrt(25)"), 5.0));
    }
    SECTION("E17: abs(-7)") {
        REQUIRE(approx(evaluate("abs(-7)"), 7.0));
    }
    SECTION("E18: exp(0)") {
        REQUIRE(approx(evaluate("exp(0)"), 1.0));
    }
    SECTION("E19: ln(1)") {
        REQUIRE(approx(evaluate("ln(1)"), 0.0));
    }
    SECTION("E20: lg(100)") {
        REQUIRE(approx(evaluate("lg(100)"), 2.0));
    }
    
    // ===== NEGATIVE TESTS: Domain errors (10) =====
    
    SECTION("E21: Division by zero") {
        REQUIRE_THROWS_AS(evaluate("5/0"), std::runtime_error);
    }
    SECTION("E22: Division by expression zero") {
        REQUIRE_THROWS_AS(evaluate("10/(3-3)"), std::runtime_error);
    }
    SECTION("E23: sqrt of negative") {
        REQUIRE_THROWS_AS(evaluate("sqrt(-1)"), std::runtime_error);
    }
    SECTION("E24: sqrt of negative expression") {
        REQUIRE_THROWS_AS(evaluate("sqrt(2-5)"), std::runtime_error);
    }
    SECTION("E25: ln(0)") {
        REQUIRE_THROWS_AS(evaluate("ln(0)"), std::runtime_error);
    }
    SECTION("E26: ln(negative)") {
        REQUIRE_THROWS_AS(evaluate("ln(-5)"), std::runtime_error);
    }
    SECTION("E27: log base 1") {
        REQUIRE_THROWS_AS(evaluate("log(1,5)"), std::runtime_error);
    }
    SECTION("E28: log negative argument") {
        REQUIRE_THROWS_AS(evaluate("log(2,-3)"), std::runtime_error);
    }
    SECTION("E29: asin out of domain") {
        REQUIRE_THROWS_AS(evaluate("asin(2)"), std::runtime_error);
    }
    SECTION("E30: Unknown variable") {
        REQUIRE_THROWS_AS(evaluate("z"), std::runtime_error);
    }
}

// ============================================================================
// INTEGRATION TESTS (Bonus: 5 tests)
// ============================================================================

TEST_CASE("Integration: Full pipeline", "[integration]") {
    
    SECTION("I01: Lab example: x*x+y*2") {
        std::unordered_map<std::string, double> vars = {{"x", 2.0}, {"y", 3.0}};
        REQUIRE(approx(evaluate("x*x+y*2", vars), 10.0));
    }
    
    SECTION("I02: Complex nested expression") {
        std::string expr = "sin(cos(0)) + sqrt(pow(3,2) + pow(4,2)) - log(2,8)";
        double expected = std::sin(std::cos(0)) + std::sqrt(9+16) - 3.0;
        REQUIRE(approx(evaluate(expr), expected));
    }
    
    SECTION("I03: Large expression (1000 additions)") {
        std::string expr = "1";
        for (int i = 0; i < 999; ++i) expr += "+1";
        REQUIRE(approx(evaluate(expr), 1000.0));
    }
    
    SECTION("I04: Error propagation in complex expr") {
        REQUIRE_THROWS_AS(
            evaluate("x + unknown * 2", {{"x", 5.0}}), 
            std::runtime_error
        );
    }
    
    SECTION("I05: Function with nested args") {
        REQUIRE(approx(
            evaluate("log(2, pow(2, 3))"),  // log_2(2^3) = 3
            3.0
        ));
    }
}
