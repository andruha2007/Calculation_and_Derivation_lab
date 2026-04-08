#include "../include/visitor.hpp"
#include "../include/parser.hpp"
#include <cmath>

Evaluator::Evaluator(std::unordered_map<std::string, double>& vars): map_for_vars(vars){};

double Evaluator::get_result() const{
    if (stack_result.size() != 1){
        throw std::runtime_error("ERROR [Evaluator]: Stack contains " + std::to_string(stack_result.size()) + " items, expected 1");
    }
    return stack_result.top();
}

double Evaluator::call_func(const std::string& name, std::vector<double>& args) {
    if (name == "sin" && args.size() == 1) return std::sin(args[0]);
    if (name == "cos" && args.size() == 1) return std::cos(args[0]);
    if (name == "tan" && args.size() == 1) return std::tan(args[0]);
    if (name == "tg" && args.size() == 1) return std::tan(args[0]);
    if (name == "ctg" && args.size() == 1) {
        double t = std::tan(args[0]);
        if (std::abs(t) < 1e-12) throw std::runtime_error("ERROR [Evaluator]: Domain error: ctg");
        return 1.0 / t;
    }
    if (name == "asin" && args.size() == 1) {
        if (args[0] < -1.0 || args[0] > 1.0) throw std::runtime_error("ERROR [Evaluator]: Domain error: asin");
        return std::asin(args[0]);
    }
    if (name == "acos" && args.size() == 1) {
        if (args[0] < -1.0 || args[0] > 1.0) throw std::runtime_error("ERROR [Evaluator]: Domain error: acos");
        return std::acos(args[0]);
    }
    if (name == "atan" && args.size() == 1) return std::atan(args[0]);
    if (name == "exp" && args.size() == 1) return std::exp(args[0]);
    if (name == "ln" && args.size() == 1) {
        if (args[0] <= 0) throw std::runtime_error("ERROR [Evaluator]: Domain error: ln");
        return std::log(args[0]);
    }
    if (name == "lg" && args.size() == 1) {
        if (args[0] <= 0) throw std::runtime_error("ERROR [Evaluator]: Domain error: lg");
        return std::log10(args[0]);
    }
    if (name == "log" && args.size() == 2) {
        if (args[0] <= 0 || std::abs(args[0] - 1.0) < 1e-12 || args[1] <= 0)
            throw std::runtime_error("ERROR [Evaluator]: Domain error: log");
        return std::log(args[1]) / std::log(args[0]);
    }
    if (name == "sqrt" && args.size() == 1) {
        if (args[0] < 0) throw std::runtime_error("ERROR [Evaluator]: Domain error: sqrt");
        return std::sqrt(args[0]);
    }
    if (name == "abs" && args.size() == 1) return std::abs(args[0]);
    if (name == "pow" && args.size() == 2) {
        if (args[0] == 0.0 && args[1] < 0.0) throw std::runtime_error("ERROR [Evaluator]: Domain error: pow");
        return std::pow(args[0], args[1]);
    }
    if (name == "min" && !args.empty()) {
        double m = args[0];
        for (size_t i = 1; i < args.size(); ++i) m = std::min(m, args[i]);
        return m;
    }
    if (name == "max" && !args.empty()) {
        double m = args[0];
        for (size_t i = 1; i < args.size(); ++i) m = std::max(m, args[i]);
        return m;
    }
    throw std::runtime_error("ERROR [Evaluator]: Unknown function: " + name + " or wrong arguments count");
}

void Evaluator::visit(NumberNode &node){
    stack_result.push(node.value);
}

void Evaluator::visit(VariableNode &node){
    auto var = map_for_vars.find(node.name);
    if (var == map_for_vars.end()){
        throw(std::runtime_error("ERROR [Evaluator]: Unknown variable \"" + node.name + "\""));
    }

    stack_result.push(var->second);
}
void Evaluator::visit(BinOpNode &node) {
    if (stack_result.size() < 2)
        throw std::runtime_error("ERROR [Evaluator]: Not enough operands for binary operation");

    double right = stack_result.top(); stack_result.pop();
    double left  = stack_result.top(); stack_result.pop();
    double res = 0.0;

    switch (node.op) {
        case OpSign::PLUS: res = left + right; break;
        case OpSign::MINUS: res = left - right; break;
        case OpSign::MULTIPLY: res = left * right; break;
        case OpSign::SUBDIVISION:
            if (std::abs(right) < 1e-12) throw std::runtime_error("ERROR [Evaluator]: Division by zero");
            res = left / right; break;
        case OpSign::DEGREE:
            if (left == 0 && right < 0) throw std::runtime_error("ERROR [Evaluator]: Domain error: pow");
            res = std::pow(left, right); break;
        default:
            throw std::runtime_error("ERROR [Evaluator]: Unknown binary operator"); 
    }
    stack_result.push(res);
}

void Evaluator::visit(UnOpNode &node){
    if (stack_result.empty())
        throw std::runtime_error("ERROR [Evaluator]: Unary operation has not enough operands");

    double operand = stack_result.top();
    stack_result.pop();

    double result = (node.op == OpSign::UPLUS) ? operand : -operand;
    stack_result.push(result);
}

void Evaluator::visit(FunctionCallNode &node){
    size_t n = node.arguments.size();
    if (stack_result.size() < n)
        throw std::runtime_error("ERROR [Evaluator] Not enough arguments for function " + node.name);

    std::vector<double> args(n);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        args[i] = stack_result.top();
        stack_result.pop();
    }
    stack_result.push(call_func(node.name, args));
}
