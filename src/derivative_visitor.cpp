#include "../include/visitor.hpp"
#include "../include/parser.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double simplifier_epsilon = 1e-12; 

ASTN_ptr make_number(double value) {
    return std::make_unique<NumberNode>(value);
}

ASTN_ptr make_variable(const std::string& name) {
    return std::make_unique<VariableNode>(name);
}

ASTN_ptr make_unary(OpSign op, ASTN_ptr middle) {
    return std::make_unique<UnOpNode>(op, std::move(middle));
}

ASTN_ptr make_binary(OpSign op, ASTN_ptr left, ASTN_ptr right) {
    return std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
}

ASTN_ptr make_function(const std::string& name, std::vector<ASTN_ptr>&& args) {
    return std::make_unique<FunctionCallNode>(name, std::move(args));
}

bool is_close(double left, double right) {
    return std::abs(left - right) < simplifier_epsilon;
}

bool get_number_value(const ASTNode* node, double& value) {
    if (auto number = dynamic_cast<const NumberNode*>(node)) {
        value = number->value;
        return true;
    }
    return false;
}

bool is_zero_node(const ASTNode* node) {
    double value = 0.0;
    return get_number_value(node, value) && is_close(value, 0.0);
}

bool is_one_node(const ASTNode* node) {
    double value = 0.0;
    return get_number_value(node, value) && is_close(value, 1.0);
}

bool is_minus_one_node(const ASTNode* node) {
    double value = 0.0;
    return get_number_value(node, value) && is_close(value, -1.0);
}

ASTN_ptr clone_node(const ASTNode* node) {
    if (node == nullptr) {
        return nullptr;
    }
    if (auto number = dynamic_cast<const NumberNode*>(node)) {
        return make_number(number->value);
    }
    if (auto variable = dynamic_cast<const VariableNode*>(node)) {
        return make_variable(variable->name);
    }
    if (auto unary = dynamic_cast<const UnOpNode*>(node)) {
        return make_unary(unary->op, clone_node(unary->middle.get()));
    }
    if (auto binary = dynamic_cast<const BinOpNode*>(node)) {
        return make_binary(binary->op, clone_node(binary->left.get()), clone_node(binary->right.get()));
    }
    if (auto function = dynamic_cast<const FunctionCallNode*>(node)) {
        std::vector<ASTN_ptr> args;
        args.reserve(function->arguments.size());
        for (const auto& argument : function->arguments) {
            args.push_back(clone_node(argument.get()));
        }
        return make_function(function->name, std::move(args));
    }
    throw std::runtime_error("ERROR [Derivative]: Unknown node type");
}

ASTN_ptr make_ln(ASTN_ptr argument) {
    std::vector<ASTN_ptr> args;
    args.push_back(std::move(argument));
    return make_function("ln", std::move(args));
}

ASTN_ptr make_sqrt(ASTN_ptr argument) {
    std::vector<ASTN_ptr> args;
    args.push_back(std::move(argument));
    return make_function("sqrt", std::move(args));
}

ASTN_ptr make_abs(ASTN_ptr argument) {
    std::vector<ASTN_ptr> args;
    args.push_back(std::move(argument));
    return make_function("abs", std::move(args));
}

ASTN_ptr make_pow(ASTN_ptr left, ASTN_ptr right) {
    return make_binary(OpSign::DEGREE, std::move(left), std::move(right));
}

ASTN_ptr make_func_pow(ASTN_ptr left, ASTN_ptr right) {
    std::vector<ASTN_ptr> args;
    args.push_back(std::move(left));
    args.push_back(std::move(right));
    return make_function("pow", std::move(args));
}

ASTN_ptr make_one_minus_square(const ASTNode* node) {
    return make_binary(
        OpSign::MINUS,
        make_number(1.0),
        make_pow(clone_node(node), make_number(2.0))
    );
}

ASTN_ptr simplify_unary_node(OpSign op, ASTN_ptr middle) {
    if (op == OpSign::UPLUS) {
        return std::move(middle);
    }
    if (op != OpSign::UMINUS) {
        throw std::runtime_error("ERROR [Simplifier]: Unknown unary operator");
    }

    double value = 0.0;
    if (get_number_value(middle.get(), value)) {
        return make_number(-value);
    }

    if (auto unary = dynamic_cast<UnOpNode*>(middle.get())) {
        if (unary->op == OpSign::UMINUS) {
            ASTN_ptr result = std::move(unary->middle);
            middle.release();
            return result;
        }
    }

    return make_unary(OpSign::UMINUS, std::move(middle));
}

void collect_add_terms(ASTN_ptr node, int sign, double& constant, std::vector<std::pair<int, ASTN_ptr>>& terms) {
    if (!node) {
        return;
    }

    double value = 0.0;
    if (get_number_value(node.get(), value)) {
        constant += sign * value;
        return;
    }

    if (auto unary = dynamic_cast<UnOpNode*>(node.get())) {
        if (unary->op == OpSign::UMINUS) {
            ASTN_ptr middle = std::move(unary->middle);
            node.reset();
            collect_add_terms(std::move(middle), -sign, constant, terms);
            return;
        }
    }

    if (auto binary = dynamic_cast<BinOpNode*>(node.get())) {
        if (binary->op == OpSign::PLUS || binary->op == OpSign::MINUS) {
            ASTN_ptr left = std::move(binary->left);
            ASTN_ptr right = std::move(binary->right);
            OpSign op = binary->op;
            node.reset();

            collect_add_terms(std::move(left), sign, constant, terms);
            collect_add_terms(std::move(right), (op == OpSign::PLUS) ? sign : -sign, constant, terms);
            return;
        }
    }

    terms.push_back({sign, std::move(node)});
}

void collect_mul_factors(ASTN_ptr node, double& constant, int& sign, std::vector<ASTN_ptr>& factors) {
    if (!node) {
        return;
    }

    double value = 0.0;
    if (get_number_value(node.get(), value)) {
        constant *= value;
        return;
    }

    if (auto unary = dynamic_cast<UnOpNode*>(node.get())) {
        if (unary->op == OpSign::UMINUS) {
            ASTN_ptr middle = std::move(unary->middle);
            node.reset();
            sign *= -1;
            collect_mul_factors(std::move(middle), constant, sign, factors);
            return;
        }
    }

    if (auto binary = dynamic_cast<BinOpNode*>(node.get())) {
        if (binary->op == OpSign::MULTIPLY) {
            ASTN_ptr left = std::move(binary->left);
            ASTN_ptr right = std::move(binary->right);
            node.reset();

            collect_mul_factors(std::move(left), constant, sign, factors);
            collect_mul_factors(std::move(right), constant, sign, factors);
            return;
        }
    }

    factors.push_back(std::move(node));
}

int factor_rank(const ASTNode* node) {
    if (dynamic_cast<const VariableNode*>(node)) {
        return 0;
    }
    if (auto binary = dynamic_cast<const BinOpNode*>(node)) {
        if (binary->op == OpSign::DEGREE) {
            return 1;
        }
        return 3;
    }
    if (dynamic_cast<const FunctionCallNode*>(node)) {
        return 2;
    }
    if (dynamic_cast<const UnOpNode*>(node)) {
        return 4;
    }
    return 5;
}

ASTN_ptr build_product(double constant, int sign, std::vector<ASTN_ptr>& factors) {
    double coefficient = constant * static_cast<double>(sign);

    if (is_close(coefficient, 0.0)) {
        return make_number(0.0);
    }

    std::stable_sort(
        factors.begin(),
        factors.end(),
        [](const ASTN_ptr& left, const ASTN_ptr& right) {
            return factor_rank(left.get()) < factor_rank(right.get());
        }
    );

    ASTN_ptr result;
    for (auto& factor : factors) {
        if (!result) {
            result = std::move(factor);
        } else {
            result = make_binary(OpSign::MULTIPLY, std::move(result), std::move(factor));
        }
    }

    if (!result) {
        return make_number(coefficient);
    }

    if (is_close(coefficient, 1.0)) {
        return result;
    }
    if (is_close(coefficient, -1.0)) {
        return simplify_unary_node(OpSign::UMINUS, std::move(result));
    }
    if (coefficient < 0.0) {
        return simplify_unary_node(
            OpSign::UMINUS,
            make_binary(OpSign::MULTIPLY, make_number(-coefficient), std::move(result))
        );
    }

    return make_binary(OpSign::MULTIPLY, make_number(coefficient), std::move(result));
}

ASTN_ptr build_sum(double constant, std::vector<std::pair<int, ASTN_ptr>>& terms) {
    ASTN_ptr result;

    if (!is_close(constant, 0.0)) {
        result = make_number(constant);
    }

    for (auto& term : terms) {
        if (term.first > 0) {
            if (!result) {
                result = std::move(term.second);
            } else {
                result = make_binary(OpSign::PLUS, std::move(result), std::move(term.second));
            }
        } else {
            if (!result) {
                result = simplify_unary_node(OpSign::UMINUS, std::move(term.second));
            } else {
                result = make_binary(OpSign::MINUS, std::move(result), std::move(term.second));
            }
        }
    }

    if (!result) {
        return make_number(0.0);
    }

    return result;
}

ASTN_ptr simplify_binary_node(OpSign op, ASTN_ptr left, ASTN_ptr right) {
    double left_value = 0.0;
    double right_value = 0.0;

    switch (op) {
        case OpSign::PLUS:
        case OpSign::MINUS: {
            double constant = 0.0;
            std::vector<std::pair<int, ASTN_ptr>> terms;

            collect_add_terms(std::move(left), 1, constant, terms);
            collect_add_terms(std::move(right), (op == OpSign::PLUS) ? 1 : -1, constant, terms);

            return build_sum(constant, terms);
        }
        case OpSign::MULTIPLY: {
            double constant = 1.0;
            int sign = 1;
            std::vector<ASTN_ptr> factors;

            collect_mul_factors(std::move(left), constant, sign, factors);
            collect_mul_factors(std::move(right), constant, sign, factors);

            return build_product(constant, sign, factors);
        }
        case OpSign::SUBDIVISION:
            if (is_zero_node(left.get())) {
                return make_number(0.0);
            }
            if (is_one_node(right.get())) {
                return std::move(left);
            }
            if (is_minus_one_node(right.get())) {
                return simplify_unary_node(OpSign::UMINUS, std::move(left));
            }
            if (get_number_value(left.get(), left_value) && get_number_value(right.get(), right_value) && !is_close(right_value, 0.0)) {
                return make_number(left_value / right_value);
            }
            return make_binary(OpSign::SUBDIVISION, std::move(left), std::move(right));
        case OpSign::DEGREE:
            if (get_number_value(right.get(), right_value)) {
                if (is_close(right_value, 0.0)) {
                    return make_number(1.0);
                }
                if (is_close(right_value, 1.0)) {
                    return std::move(left);
                }
                if (is_zero_node(left.get()) && right_value > 0.0) {
                    return make_number(0.0);
                }
                if (is_one_node(left.get())) {
                    return make_number(1.0);
                }
            }
            if (get_number_value(left.get(), left_value) && get_number_value(right.get(), right_value)) {
                return make_number(std::pow(left_value, right_value));
            }
            return make_binary(OpSign::DEGREE, std::move(left), std::move(right));
        default:
            throw std::runtime_error("ERROR [Simplifier]: Unknown binary operator");
    }
}
}

Derivative::Derivative(std::string variable_name): differentiation_variable(std::move(variable_name)){};

AST Derivative::get_result() {
    if (stack_result.size() != 1) {
        throw std::runtime_error("ERROR [Derivative]: Stack contains " + std::to_string(stack_result.size()) + " items, expected 1");
    }
    ASTN_ptr result = std::move(stack_result.top());
    stack_result.pop();

    AST derivative_tree(std::move(result));
    Simplifier simplifier;
    derivative_tree.get_root()->accept(simplifier);
    return simplifier.get_result();
}

void Derivative::visit(NumberNode &) {
    stack_result.push(make_number(0.0));
}

void Derivative::visit(VariableNode &node) {
    stack_result.push(make_number(node.name == differentiation_variable ? 1.0 : 0.0));
}

void Derivative::visit(UnOpNode &node) {
    if (stack_result.empty()) {
        throw std::runtime_error("ERROR [Derivative]: Unary operation has not enough operands");
    }

    ASTN_ptr middle = std::move(stack_result.top());
    stack_result.pop();

    if (node.op == OpSign::UPLUS) {
        stack_result.push(std::move(middle));
        return;
    }
    if (node.op == OpSign::UMINUS) {
        stack_result.push(make_unary(OpSign::UMINUS, std::move(middle)));
        return;
    }

    throw std::runtime_error("ERROR [Derivative]: Unknown unary operator");
}

void Derivative::visit(BinOpNode &node) {
    if (stack_result.size() < 2) {
        throw std::runtime_error("ERROR [Derivative]: Not enough operands for binary operation");
    }

    ASTN_ptr right_derivative = std::move(stack_result.top());
    stack_result.pop();
    ASTN_ptr left_derivative = std::move(stack_result.top());
    stack_result.pop();

    ASTN_ptr result;

    switch (node.op) {
        case OpSign::PLUS:
            result = make_binary(OpSign::PLUS, std::move(left_derivative), std::move(right_derivative));
            break;
        case OpSign::MINUS:
            result = make_binary(OpSign::MINUS, std::move(left_derivative), std::move(right_derivative));
            break;
        case OpSign::MULTIPLY:
            result = make_binary(
                OpSign::PLUS,
                make_binary(OpSign::MULTIPLY, std::move(left_derivative), clone_node(node.right.get())),
                make_binary(OpSign::MULTIPLY, clone_node(node.left.get()), std::move(right_derivative))
            );
            break;
        case OpSign::SUBDIVISION:
            result = make_binary(
                OpSign::SUBDIVISION,
                make_binary(
                    OpSign::MINUS,
                    make_binary(OpSign::MULTIPLY, std::move(left_derivative), clone_node(node.right.get())),
                    make_binary(OpSign::MULTIPLY, clone_node(node.left.get()), std::move(right_derivative))
                ),
                make_pow(clone_node(node.right.get()), make_number(2.0))
            );
            break;
        case OpSign::DEGREE:
            if (auto number = dynamic_cast<const NumberNode*>(node.right.get())) {
                result = make_binary(
                    OpSign::MULTIPLY,
                    make_binary(
                        OpSign::MULTIPLY,
                        make_number(number->value),
                        make_pow(clone_node(node.left.get()), make_number(number->value - 1.0))
                    ),
                    std::move(left_derivative)
                );
            } else if (dynamic_cast<const NumberNode*>(node.left.get()) != nullptr) {
                result = make_binary(
                    OpSign::MULTIPLY,
                    make_pow(clone_node(node.left.get()), clone_node(node.right.get())),
                    make_binary(OpSign::MULTIPLY, make_ln(clone_node(node.left.get())), std::move(right_derivative))
                );
            } else {
                result = make_binary(
                    OpSign::MULTIPLY,
                    make_pow(clone_node(node.left.get()), clone_node(node.right.get())),
                    make_binary(
                        OpSign::PLUS,
                        make_binary(OpSign::MULTIPLY, std::move(right_derivative), make_ln(clone_node(node.left.get()))),
                        make_binary(
                            OpSign::MULTIPLY,
                            clone_node(node.right.get()),
                            make_binary(OpSign::SUBDIVISION, std::move(left_derivative), clone_node(node.left.get()))
                        )
                    )
                );
            }
            break;
        default:
            throw std::runtime_error("ERROR [Derivative]: Unknown binary operator");
    }

    stack_result.push(std::move(result));
}

void Derivative::visit(FunctionCallNode &node) {
    size_t arg_count = node.arguments.size();
    if (stack_result.size() < arg_count) {
        throw std::runtime_error("ERROR [Derivative]: Not enough arguments for function " + node.name);
    }

    std::vector<ASTN_ptr> derivatives(arg_count);
    for (int i = static_cast<int>(arg_count) - 1; i >= 0; --i) {
        derivatives[i] = std::move(stack_result.top());
        stack_result.pop();
    }

    ASTN_ptr result;

    if (node.name == "sin" && arg_count == 1) {
        std::vector<ASTN_ptr> args;
        args.push_back(clone_node(node.arguments[0].get()));
        result = make_binary(OpSign::MULTIPLY, make_function("cos", std::move(args)), std::move(derivatives[0]));
    } else if ((node.name == "cos") && arg_count == 1) {
        std::vector<ASTN_ptr> args;
        args.push_back(clone_node(node.arguments[0].get()));
        result = make_binary(
            OpSign::MULTIPLY,
            make_unary(OpSign::UMINUS, make_function("sin", std::move(args))),
            std::move(derivatives[0])
        );
    } else if ((node.name == "tan" || node.name == "tg") && arg_count == 1) {
        std::vector<ASTN_ptr> cos_args;
        cos_args.push_back(clone_node(node.arguments[0].get()));
        result = make_binary(
            OpSign::SUBDIVISION,
            std::move(derivatives[0]),
            make_pow(make_function("cos", std::move(cos_args)), make_number(2.0))
        );
    } else if (node.name == "ctg" && arg_count == 1) {
        std::vector<ASTN_ptr> sin_args;
        sin_args.push_back(clone_node(node.arguments[0].get()));
        result = make_unary(
            OpSign::UMINUS,
            make_binary(
                OpSign::SUBDIVISION,
                std::move(derivatives[0]),
                make_pow(make_function("sin", std::move(sin_args)), make_number(2.0))
            )
        );
    } else if (node.name == "asin" && arg_count == 1) {
        result = make_binary(
            OpSign::SUBDIVISION,
            std::move(derivatives[0]),
            make_sqrt(make_one_minus_square(node.arguments[0].get()))
        );
    } else if (node.name == "acos" && arg_count == 1) {
        result = make_unary(
            OpSign::UMINUS,
            make_binary(
                OpSign::SUBDIVISION,
                std::move(derivatives[0]),
                make_sqrt(make_one_minus_square(node.arguments[0].get()))
            )
        );
    } else if (node.name == "atan" && arg_count == 1) {
        result = make_binary(
            OpSign::SUBDIVISION,
            std::move(derivatives[0]),
            make_binary(
                OpSign::PLUS,
                make_number(1.0),
                make_pow(clone_node(node.arguments[0].get()), make_number(2.0))
            )
        );
    } else if (node.name == "exp" && arg_count == 1) {
        std::vector<ASTN_ptr> args;
        args.push_back(clone_node(node.arguments[0].get()));
        result = make_binary(OpSign::MULTIPLY, make_function("exp", std::move(args)), std::move(derivatives[0]));
    } else if (node.name == "ln" && arg_count == 1) {
        result = make_binary(OpSign::SUBDIVISION, std::move(derivatives[0]), clone_node(node.arguments[0].get()));
    } else if (node.name == "lg" && arg_count == 1) {
        result = make_binary(
            OpSign::SUBDIVISION,
            std::move(derivatives[0]),
            make_binary(OpSign::MULTIPLY, clone_node(node.arguments[0].get()), make_ln(make_number(10.0)))
        );
    } else if (node.name == "sqrt" && arg_count == 1) {
        result = make_binary(
            OpSign::SUBDIVISION,
            std::move(derivatives[0]),
            make_binary(OpSign::MULTIPLY, make_number(2.0), make_sqrt(clone_node(node.arguments[0].get())))
        );
    } else if (node.name == "abs" && arg_count == 1) {
        result = make_binary(
            OpSign::MULTIPLY,
            make_binary(OpSign::SUBDIVISION, clone_node(node.arguments[0].get()), make_abs(clone_node(node.arguments[0].get()))),
            std::move(derivatives[0])
        );
    } else if ((node.name == "pow" || node.name == "log") && arg_count == 2) {
        if (node.name == "pow") {
            result = make_binary(
                OpSign::MULTIPLY,
                make_func_pow(clone_node(node.arguments[0].get()), clone_node(node.arguments[1].get())),
                make_binary(
                    OpSign::PLUS,
                    make_binary(OpSign::MULTIPLY, std::move(derivatives[1]), make_ln(clone_node(node.arguments[0].get()))),
                    make_binary(
                        OpSign::MULTIPLY,
                        clone_node(node.arguments[1].get()),
                        make_binary(OpSign::SUBDIVISION, std::move(derivatives[0]), clone_node(node.arguments[0].get()))
                    )
                )
            );
        } else {
            result = make_binary(
                OpSign::MINUS,
                make_binary(
                    OpSign::SUBDIVISION,
                    std::move(derivatives[1]),
                    make_binary(OpSign::MULTIPLY, clone_node(node.arguments[1].get()), make_ln(clone_node(node.arguments[0].get())))
                ),
                make_binary(
                    OpSign::SUBDIVISION,
                    make_binary(OpSign::MULTIPLY, make_ln(clone_node(node.arguments[1].get())), std::move(derivatives[0])),
                    make_binary(
                        OpSign::MULTIPLY,
                        clone_node(node.arguments[0].get()),
                        make_pow(make_ln(clone_node(node.arguments[0].get())), make_number(2.0))
                    )
                )
            );
        }
    } else if ((node.name == "min" || node.name == "max") && arg_count >= 1) {
        throw std::runtime_error("ERROR [Derivative]: Derivative for variadic function '" + node.name + "' is undefined");
    } else {
        throw std::runtime_error("ERROR [Derivative]: Unknown function: " + node.name + " or wrong arguments count");
    }

    stack_result.push(std::move(result));
}

AST Simplifier::get_result() {
    if (stack_result.size() != 1) {
        throw std::runtime_error("ERROR [Simplifier]: Stack contains " + std::to_string(stack_result.size()) + " items, expected 1");
    }
    ASTN_ptr result = std::move(stack_result.top());
    stack_result.pop();
    return AST(std::move(result));
}

void Simplifier::visit(NumberNode &node) {
    stack_result.push(make_number(node.value));
}

void Simplifier::visit(VariableNode &node) {
    stack_result.push(make_variable(node.name));
}

void Simplifier::visit(UnOpNode &node) {
    if (stack_result.empty()) {
        throw std::runtime_error("ERROR [Simplifier]: Unary operation has not enough operands");
    }

    ASTN_ptr middle = std::move(stack_result.top());
    stack_result.pop();
    stack_result.push(simplify_unary_node(node.op, std::move(middle)));
}

void Simplifier::visit(BinOpNode &node) {
    if (stack_result.size() < 2) {
        throw std::runtime_error("ERROR [Simplifier]: Not enough operands for binary operation");
    }

    ASTN_ptr right = std::move(stack_result.top());
    stack_result.pop();
    ASTN_ptr left = std::move(stack_result.top());
    stack_result.pop();

    stack_result.push(simplify_binary_node(node.op, std::move(left), std::move(right)));
}

void Simplifier::visit(FunctionCallNode &node) {
    size_t arg_count = node.arguments.size();
    if (stack_result.size() < arg_count) {
        throw std::runtime_error("ERROR [Simplifier]: Not enough arguments for function " + node.name);
    }

    std::vector<ASTN_ptr> arguments(arg_count);
    for (int i = static_cast<int>(arg_count) - 1; i >= 0; --i) {
        arguments[i] = std::move(stack_result.top());
        stack_result.pop();
    }

    stack_result.push(make_function(node.name, std::move(arguments)));
}
