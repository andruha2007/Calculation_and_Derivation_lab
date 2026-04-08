// parser.cpp

#include "../include/parser.hpp"

NumberNode::NumberNode(double v): value(v){};

VariableNode::VariableNode(std::string n): name(n){};

FunctionCallNode::FunctionCallNode(std::string n, std::vector<ASTN_ptr>&& args): name(std::move(n)), arguments(std::move(args)) {}

BinOpNode::BinOpNode(OpSign op_, ASTN_ptr l, ASTN_ptr r): op(op_), left(std::move(l)), right(std::move(r)){};

UnOpNode::UnOpNode(OpSign op_, ASTN_ptr m): op(op_), middle(std::move(m)){};

AST::AST(ASTN_ptr r): root(std::move(r)){};

ASTNode* AST::get_root() const{ return root.get();};

void NumberNode::accept(Visitor &v){ v.visit(*this);};

void VariableNode::accept(Visitor &v){ v.visit(*this);};

void FunctionCallNode::accept(Visitor &v){
    for (auto& arg : arguments) {
        if (arg) arg->accept(v);
    }
    v.visit(*this);
};

void BinOpNode::accept(Visitor &v){
    if (left) left->accept(v);
    if (right) right->accept(v);
    v.visit(*this);
};

void UnOpNode::accept(Visitor &v){
    if(middle)
        middle->accept(v);
    v.visit(*this);
};

int get_expected_arg_count(const std::string& name) {

    auto it = arg_counts.find(name);
    return (it != arg_counts.end()) ? it->second : -2;  // -2 = unknown function
}

OperatorType::OperatorType(Token op, bool is_unary, bool is_func): is_function(is_func){
            if (is_function){
                priority = 5;
                function = op.get_value();
            }else if (is_unary){
                    priority = 3;
                    if (op.get_value() == "+"){symbol = OpSign::UPLUS;}
                    if (op.get_value() == "-"){symbol = OpSign::UMINUS;}
            } else{
                if (op.get_value() == "+"){symbol = OpSign::PLUS; priority = 1;}
                else if (op.get_value() == "-"){symbol = OpSign::MINUS; priority = 1;}
                else if (op.get_value() == "*"){symbol = OpSign::MULTIPLY; priority = 2;}
                else if (op.get_value() == "/"){symbol = OpSign::SUBDIVISION; priority = 2;}
                else if (op.get_value() == "^"){symbol = OpSign::DEGREE; priority = 4; is_left_assotiativity = false;}
                else if (op.get_value() == "("){symbol = OpSign::L_PAREN; priority = 0;}

                else throw std::runtime_error("ERROR [Parser]: Unknown operator \"" + op.get_value() + "\""); // ПО СУТИ ВСЕ ЭТО ЛОВИТ ЛЕКСЕР
            }

};

bool OperatorType::is_func() const{return is_function;};
int OperatorType::get_priority() const{ return priority;};
OpSign OperatorType::get_symbol() const {return symbol;};
std::string OperatorType::get_function() const{return function;};

Parser::Parser(Lexer lex): lexer(lex){
    current = lexer.next();
};

AST Parser::parse(){
    Token next = lexer.next();
    Token memory = {0, "", lexem_t::EOEX};
    while(current.get_type() != lexem_t::EOEX){

        ////////////////////________ОШИБКА________////////////////////

        if (first_call && (current.get_type() == lexem_t::OPERATOR && current.get_value() != "+"
            && current.get_value() != "-")){
            throw std::runtime_error("ERROR [Parser]: String can not start from " + current.get_value());
        }
        if (current.get_type() == next.get_type() && next.get_value() != "-"
            && next.get_value() != "+" && current.get_type() != lexem_t::L_CIRCLE_PAPAREN
            && current.get_type() != lexem_t::R_CIRCLE_PAPAREN){
            throw std::runtime_error("ERROR [Parser]: " + current.get_value());
        } // 2 ОПЕРАТОРА ПОДРЯД
        if (!is_in_function && current.get_value() == ","){
            throw std::runtime_error("ERROR [Parser]: Unexpected ,");
        }

        ////////////////////________ЗАПИСЬ_В_СТЕК________////////////////////

        if (current.get_type() == lexem_t::NUMBER){
            tree_stack.push_back(std::make_unique<NumberNode>(std::stod(current.get_value())));
        } // ЧИСЕЛКО

        else if (current.get_type() == lexem_t::IDENTIFICATOR && next.get_type() != lexem_t::L_CIRCLE_PAPAREN){
            if (unforgivable_names_of_vars.find(current.get_value()) != unforgivable_names_of_vars.end()) {
                throw std::runtime_error("ERROR [Parser]: Identifier '" + current.get_value() + "' is a reserved function name and cannot be used as a variable.");
            }
            tree_stack.push_back(std::make_unique<VariableNode>(current.get_value()));
        } // ПЕРЕМЕННАЯ

        else if(current.get_type() == lexem_t::IDENTIFICATOR && next.get_type() == lexem_t::L_CIRCLE_PAPAREN){
            OperatorType  op(current, false, true);

            stack_op.push_back(op);
            stack_of_args_sizes.push_back(stack_of_args.size());

            is_in_function = true;

        } // ФУНКЦИЯ

        else if (current.get_type() == lexem_t::OPERATOR && current.get_value() != ","){
            bool unary = first_call || (!first_call && memory.get_type() == lexem_t::OPERATOR) || memory.get_value() == "," || memory.get_value() == "(";
            OperatorType  op(current, unary, false);

            if (unary){
                stack_op.push_back(op);
            } else if (stack_op.empty()){
                stack_op.push_back(op);
            } else if (op.get_priority() > stack_op.back().get_priority() || stack_op.back().is_func()
                || (stack_op.back().get_symbol() == OpSign::DEGREE && current.get_value() == "^")){
                stack_op.push_back(op);
            } else if (op.get_priority() <= stack_op.back().get_priority()){
                while (!stack_op.empty() && op.get_priority() <= stack_op.back().get_priority()){
                    OperatorType op_st = stack_op.back();
                    stack_op.pop_back();

                    if (op_st.get_priority() == 3){
                        if (tree_stack.empty())
                            throw std::runtime_error("ERROR [Parser] unary operator without operand");

                        ASTN_ptr middle = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        tree_stack.push_back(std::make_unique<UnOpNode>(op_st.get_symbol(), std::move(middle)));
                    } else{
                        if (tree_stack.size() < 2)
                            throw(std::runtime_error("ERROR [Parser] not enough operands to make operation"));

                        ASTN_ptr right = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        ASTN_ptr left = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        tree_stack.push_back(std::make_unique<BinOpNode>(op_st.get_symbol(), std::move(left), std::move(right)));
                    }
                }
                stack_op.push_back(op);
            }
        } // ОПЕРАТОР

        else if (current.get_value() == "," && is_in_function){
                if (next.get_value() == ")"){
                    throw(std::runtime_error("ERROR [Parser] Unexpected ,)"));
                }
                if (next.get_value() == ","){
                    throw(std::runtime_error("ERROR [Parser] Unexpected ,,"));
                }
                while (!stack_op.empty() && stack_op.back().get_symbol() != OpSign::L_PAREN){

                    if (stack_op.back().get_priority() == 3){
                        if (tree_stack.empty())
                            throw std::runtime_error("ERROR [Parser] unary operator without operand");
                        ASTN_ptr middle = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        tree_stack.push_back(std::make_unique<UnOpNode>(stack_op.back().get_symbol(), std::move(middle)));
                    } else{
                        if (tree_stack.size() < 2)
                            throw(std::runtime_error("ERROR [Parser] not enough operands to make operation"));

                        ASTN_ptr right = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        ASTN_ptr left = std::move(tree_stack.back());
                        tree_stack.pop_back();

                        tree_stack.push_back(std::make_unique<BinOpNode>(stack_op.back().get_symbol(), std::move(left), std::move(right)));
                    }
                    stack_op.pop_back();
                }
                if (tree_stack.empty()){
                    throw(std::runtime_error("ERROR [Parser] Unexpected ,"));
                }
                stack_of_args.push_back(std::move(tree_stack.back()));
                tree_stack.pop_back();
        } // ЗАПЯТАЯ

        else if(current.get_type() == lexem_t::L_CIRCLE_PAPAREN){
            if (next.get_type() == lexem_t::R_CIRCLE_PAPAREN && !stack_op.empty() && stack_op.back().is_func()) 
                throw(std::runtime_error("ERROR [Parser] Unexpected parametrs in function " + stack_op.back().get_function())); 
            stack_op.push_back(OperatorType(current,0,0));
        } // ЛЕВАЯ СКОБКА

        else if (current.get_type() == lexem_t::R_CIRCLE_PAPAREN){
            if (first_call)
                throw(std::runtime_error("ERROR [Parser] Unexpected symbol )"));
            if (!stack_op.empty() && stack_op.back().get_symbol() == OpSign::L_PAREN && !is_in_function)
                throw(std::runtime_error("ERROR [Parser] ()"));
            while(!stack_op.empty() && stack_op.back().get_symbol() != OpSign::L_PAREN){
                if (stack_op.back().get_priority() == 3){
                    if (tree_stack.empty())
                            throw std::runtime_error("ERROR [Parser] unary operator without operand");
                    ASTN_ptr middle = std::move(tree_stack.back());
                    tree_stack.pop_back();

                    tree_stack.push_back(std::make_unique<UnOpNode>(stack_op.back().get_symbol(), std::move(middle)));
                } else{
                    if (tree_stack.size() < 2)
                        throw(std::runtime_error("ERROR [Parser] not enough operands to make operation"));

                    ASTN_ptr right = std::move(tree_stack.back());
                    tree_stack.pop_back();

                    ASTN_ptr left = std::move(tree_stack.back());
                    tree_stack.pop_back();

                    tree_stack.push_back(std::make_unique<BinOpNode>(stack_op.back().get_symbol(), std::move(left), std::move(right)));
                }
                stack_op.pop_back();

            }
            if (stack_op.empty()){
                throw(std::runtime_error("ERROR [Parser] Unexpected symbol )"));
            } else if (stack_op.back().get_symbol() == OpSign::L_PAREN){
                stack_op.pop_back();
            }
            if (is_in_function){
                if (!stack_op.empty() && stack_op.back().is_func()){
                    if (stack_of_args_sizes.empty()){
                        throw std::runtime_error("ERROR [Parser] invalid function state");
                    }
                    if (!tree_stack.empty()) {
                        stack_of_args.push_back(std::move(tree_stack.back()));
                        tree_stack.pop_back();
                    }

                    std::string func_name = stack_op.back().get_function();
                    int expected = get_expected_arg_count(func_name);
                    int actual = static_cast<int>(stack_of_args.size() - stack_of_args_sizes.back());

                    if (expected == -2) {
                        throw std::runtime_error("ERROR [Parser] Unknown function: " + func_name);
                    } else if (expected == -1) {
                        if (actual < 1) {
                            throw std::runtime_error("ERROR [Parser] Function '" + func_name + "' requires at least 1 argument, got " + std::to_string(actual));
                        }
                    } else if (actual != expected) {
                        throw std::runtime_error("ERROR [Parser] Function '" + func_name + "' expects " +
                                               std::to_string(expected) + " argument(s), got " + std::to_string(actual));
                    }

                    std::vector<ASTN_ptr> arguments;
                    for (size_t i = stack_of_args_sizes.back(); i < stack_of_args.size(); ++i){
                        arguments.push_back(std::move(stack_of_args[i]));
                    }
                    stack_of_args.resize(stack_of_args_sizes.back());

                    tree_stack.push_back(std::make_unique<FunctionCallNode>(func_name, std::move(arguments)));
                    stack_op.pop_back();
                    stack_of_args_sizes.pop_back();
                    is_in_function = !stack_of_args_sizes.empty();
                }
            }
        } // ПРАВАЯ СКОБКА

        memory = current;
        current = next;
        first_call = false;
        next = lexer.next();
    }

    while(!stack_op.empty()){

        if (stack_op.back().get_symbol() == OpSign::L_PAREN)
            throw(std::runtime_error("ERROR [Parser] Unexpected symbol ("));

        if (stack_op.back().get_priority() == 3){
            if (tree_stack.empty())
                throw std::runtime_error("ERROR [Parser] unary operator without operand");
            ASTN_ptr middle = std::move(tree_stack.back());
            tree_stack.pop_back();

            tree_stack.push_back(std::make_unique<UnOpNode>(stack_op.back().get_symbol(), std::move(middle)));
        } else{
            if (tree_stack.size() < 2)
                throw(std::runtime_error("ERROR [Parser] not enough operands to make operation " ));

            ASTN_ptr right = std::move(tree_stack.back());
            tree_stack.pop_back();

            ASTN_ptr left = std::move(tree_stack.back());
            tree_stack.pop_back();

            tree_stack.push_back(std::make_unique<BinOpNode>(stack_op.back().get_symbol(), std::move(left), std::move(right)));
        }
        stack_op.pop_back();

    }

    if (tree_stack.size() != 1)
        throw std::runtime_error("ERROR [Parser] invalid expression");

    AST ast_tree(std::move(tree_stack.back()));

    return ast_tree;
}

std::string fmt_num(double v) {
    std::string s = std::to_string(v);
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

namespace {
int get_node_priority(const ASTNode* node) {
    if (dynamic_cast<const NumberNode*>(node) || dynamic_cast<const VariableNode*>(node) || dynamic_cast<const FunctionCallNode*>(node)) {
        return 5;
    }
    if (auto* unary = dynamic_cast<const UnOpNode*>(node)) {
        if (unary->op == OpSign::UPLUS || unary->op == OpSign::UMINUS) {
            return 3;
        }
    }
    if (auto* binary = dynamic_cast<const BinOpNode*>(node)) {
        if (binary->op == OpSign::PLUS || binary->op == OpSign::MINUS) {
            return 1;
        }
        if (binary->op == OpSign::MULTIPLY || binary->op == OpSign::SUBDIVISION) {
            return 2;
        }
        if (binary->op == OpSign::DEGREE) {
            return 4;
        }
    }
    return 0;
}

bool is_binary_operation(const ASTNode* node, OpSign op) {
    auto* binary = dynamic_cast<const BinOpNode*>(node);
    return binary != nullptr && binary->op == op;
}

bool is_negative_unary(const ASTNode* node) {
    auto* unary = dynamic_cast<const UnOpNode*>(node);
    return unary != nullptr && unary->op == OpSign::UMINUS;
}

std::string print_ast_impl(const ASTNode* node);

std::string wrap_if_needed(const ASTNode* node, const std::string& value, bool needed) {
    if (!node) {
        return value;
    }
    return needed ? "(" + value + ")" : value;
}

std::string print_function_arguments(const std::vector<ASTN_ptr>& arguments) {
    std::string result;
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += print_ast_impl(arguments[i].get());
    }
    return result;
}

std::string print_ast_impl(const ASTNode* node) {
    if (!node) {
        return "";
    }

    if (auto* number = dynamic_cast<const NumberNode*>(node)) {
        return fmt_num(number->value);
    }
    if (auto* variable = dynamic_cast<const VariableNode*>(node)) {
        return variable->name;
    }
    if (auto* function = dynamic_cast<const FunctionCallNode*>(node)) {
        return function->name + "(" + print_function_arguments(function->arguments) + ")";
    }
    if (auto* unary = dynamic_cast<const UnOpNode*>(node)) {
        std::string middle = print_ast_impl(unary->middle.get());
        bool need_parentheses = get_node_priority(unary->middle.get()) < get_node_priority(node);
        if (is_negative_unary(unary->middle.get())) {
            need_parentheses = true;
        }
        if (unary->op == OpSign::UPLUS) {
            return "+" + wrap_if_needed(unary->middle.get(), middle, need_parentheses);
        }
        if (unary->op == OpSign::UMINUS) {
            return "-" + wrap_if_needed(unary->middle.get(), middle, need_parentheses);
        }
        return middle;
    }
    if (auto* binary = dynamic_cast<const BinOpNode*>(node)) {
        std::string left = print_ast_impl(binary->left.get());
        std::string right = print_ast_impl(binary->right.get());

        bool need_left_parentheses = false;
        bool need_right_parentheses = false;
        int current_priority = get_node_priority(node);
        int left_priority = get_node_priority(binary->left.get());
        int right_priority = get_node_priority(binary->right.get());

        if (binary->op == OpSign::PLUS) {
            need_left_parentheses = left_priority < current_priority;
            need_right_parentheses = right_priority < current_priority;
            return wrap_if_needed(binary->left.get(), left, need_left_parentheses) + " + " +
                   wrap_if_needed(binary->right.get(), right, need_right_parentheses);
        }
        if (binary->op == OpSign::MINUS) {
            need_left_parentheses = left_priority < current_priority;
            need_right_parentheses = right_priority <= current_priority;
            return wrap_if_needed(binary->left.get(), left, need_left_parentheses) + " - " +
                   wrap_if_needed(binary->right.get(), right, need_right_parentheses);
        }
        if (binary->op == OpSign::MULTIPLY) {
            need_left_parentheses = left_priority < current_priority;
            need_right_parentheses = right_priority < current_priority || is_negative_unary(binary->right.get());
            return wrap_if_needed(binary->left.get(), left, need_left_parentheses) + "*" +
                   wrap_if_needed(binary->right.get(), right, need_right_parentheses);
        }
        if (binary->op == OpSign::SUBDIVISION) {
            need_left_parentheses = left_priority < current_priority;
            need_right_parentheses = right_priority <= current_priority || is_negative_unary(binary->right.get());
            return wrap_if_needed(binary->left.get(), left, need_left_parentheses) + "/" +
                   wrap_if_needed(binary->right.get(), right, need_right_parentheses);
        }
        if (binary->op == OpSign::DEGREE) {
            need_left_parentheses = left_priority < current_priority || is_binary_operation(binary->left.get(), OpSign::DEGREE);
            need_right_parentheses = right_priority <= current_priority;
            return wrap_if_needed(binary->left.get(), left, need_left_parentheses) + "^" +
                   wrap_if_needed(binary->right.get(), right, need_right_parentheses);
        }
    }

    return "";
}
}

std::string print_ast(const ASTNode* node) {
    return print_ast_impl(node);
}
