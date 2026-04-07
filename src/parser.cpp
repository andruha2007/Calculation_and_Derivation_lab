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
    return (it != arg_counts.end()) ? it->second : -2;  // -2 = неизвестная функция
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

                else throw std::runtime_error("ERROR [Parser] Unknown operator \"" + op.get_value() + "\""); // по сути лексер собрал весь мусор от пользователя
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

        ////////////////////________ОШИБКИ________////////////////////
        
        if (first_call && (current.get_type() == lexem_t::OPERATOR && current.get_value() != "+" 
            && current.get_value() != "-")){
            throw std::runtime_error("ERROR [Parser] String can not start from " + current.get_value());
        } // Строка начинается с операции, не являющейся + или -
        if (current.get_type() == next.get_type() && next.get_value() != "-"
            && next.get_value() != "+" && current.get_type() != lexem_t::L_CIRCLE_PAPAREN){
            throw std::runtime_error("ERROR [Parser] " + current.get_value());
        } // 2 операции (не + -), числа, идентификатора подряд (но не скобки)
        if (!is_in_function && current.get_value() == ","){
            throw std::runtime_error("ERROR [Parser] Unexpected ,");
        }

        ////////////////////________РЕАЛИЗАИЯ_ЗАПИСИ_В_СТЕК________////////////////////
        
        if (current.get_type() == lexem_t::NUMBER){
            tree_stack.push_back(std::make_unique<NumberNode>(std::stod(current.get_value())));
        } // Число
        
        else if (current.get_type() == lexem_t::IDENTIFICATOR && next.get_type() != lexem_t::L_CIRCLE_PAPAREN){
            if (unforgivable_names_of_vars.find(current.get_value()) != unforgivable_names_of_vars.end()) {
                throw std::runtime_error("ERROR [Parser]: Identifier '" + current.get_value() + "' is a reserved function name and cannot be used as a variable.");
            }
            tree_stack.push_back(std::make_unique<VariableNode>(current.get_value()));
        } // Переменная
        
        else if(current.get_type() == lexem_t::IDENTIFICATOR && next.get_type() == lexem_t::L_CIRCLE_PAPAREN){
            OperatorType  op(current, false, true);

            stack_op.push_back(op);
            
            is_in_function = true;
            
        } // Функция
        
        else if (current.get_type() == lexem_t::OPERATOR && current.get_value() != ","){ 
            bool unary = first_call || (!first_call && memory.get_type() == lexem_t::OPERATOR) || memory.get_value() == "," || memory.get_value() == "(";
            OperatorType  op(current, unary, false);

            if (unary){
                stack_op.push_back(op); // забавно! смысла сравнивать унарный оператор нет никакого
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
        } // Оператор

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
                stack_of_args.push_back(std::move(tree_stack.back()));
                tree_stack.pop_back();
        } // Запятая

        else if(current.get_type() == lexem_t::L_CIRCLE_PAPAREN){
            if (next.get_type() == lexem_t::R_CIRCLE_PAPAREN && is_in_function) // если захотеть такое f() - функцию без аргументов, на крайний случай можно сделеать костыль
                throw(std::runtime_error("ERROR [Parser] Unexpected parametrs in function " + stack_op.back().get_function())); // дабы запомнить, что из стека дерева ничего таскать не надо
            stack_op.push_back(OperatorType(current,0,0)); 
        } // Левая скобка
        
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
                // Проверяем, является ли текущий оператор функцией
                if (!stack_op.empty() && stack_op.back().is_func()){
                    if (!tree_stack.empty()) {
                        stack_of_args.push_back(std::move(tree_stack.back()));
                        tree_stack.pop_back();
                    }
                    
                    std::string func_name = stack_op.back().get_function();
                    int expected = get_expected_arg_count(func_name);
                    int actual = static_cast<int>(stack_of_args.size());
                    
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
                    
                    tree_stack.push_back(std::make_unique<FunctionCallNode>(func_name, std::move(stack_of_args)));
                    is_in_function = false;
                    stack_op.pop_back();
                    stack_of_args.clear();
                }
                // Если это не функция, просто закрываем скобку и продолжаем
            }
        } // Правая скобка

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

std::string print_ast(const ASTNode* node) {
    if (!node) return "";
    if (auto* n = dynamic_cast<const NumberNode*>(node)) return fmt_num(n->value) + " ";
    if (auto* v = dynamic_cast<const VariableNode*>(node)) return v->name + " ";
    
    if (auto* u = dynamic_cast<const UnOpNode*>(node)) {
        std::string op_str = (u->op == OpSign::UPLUS) ? "u+" : "u-";
        return "( " + op_str + " " + print_ast(u->middle.get()) + ") ";
    }

    if (auto* b = dynamic_cast<const BinOpNode*>(node)) {
        const char* sym = nullptr;
        switch(b->op) {
            case OpSign::PLUS: sym = "+ "; break;
            case OpSign::MINUS: sym = "- "; break;
            case OpSign::MULTIPLY: sym = "* "; break;
            case OpSign::SUBDIVISION: sym = "/ "; break;
            case OpSign::DEGREE: sym = "^ "; break;
            default: sym = "? ";
        }
        return std::string("( ") + sym + print_ast(b->left.get()) + print_ast(b->right.get()) + ") ";
    }
    if (auto* f = dynamic_cast<const FunctionCallNode*>(node)) {
        std::string s = "( " + f->name + " ";
        for (const auto& arg : f->arguments) s += print_ast(arg.get());
        return s + ") ";
    }
    return "";
}
