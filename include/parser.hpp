#pragma once

#include "lexer.hpp"
#include "visitor.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>


class Visitor;

const std::unordered_set<std::string> unforgivable_names_of_vars = {
    "sin", "cos", "tan", "ctg",
    "asin", "acos", "atan",
    "exp", "log", "ln", "lg",
    "sqrt", "abs", "pow",
    "min", "max"
};

const std::unordered_map<std::string, int> arg_counts = {
        {"sin", 1}, {"cos", 1}, {"tan", 1}, {"tg", 1}, {"ctg", 1},
        {"asin", 1}, {"acos", 1}, {"atan", 1},
        {"exp", 1}, {"ln", 1}, {"lg", 1},
        {"sqrt", 1}, {"abs", 1},
        {"log", 2}, {"pow", 2},
        {"min", -1}, {"max", -1}  // -1 означает "1 или больше"
};


enum class OpSign{
    PLUS,
    MINUS,
    UPLUS,
    UMINUS,
    MULTIPLY,
    SUBDIVISION,
    DEGREE,
    L_PAREN,
    R_PAREN
};

struct ASTNode{
    virtual ~ASTNode() = default;
    //virtual ASTN_ptr clone() const = 0;

    virtual void accept(Visitor &v) = 0;
};

using ASTN_ptr = std::unique_ptr<ASTNode>;

struct NumberNode final : ASTNode {
    double value;
    explicit NumberNode(double v);

    void accept(Visitor &v) override;
};

struct VariableNode final: ASTNode{
    std::string name;
    // конструктор и clone
    explicit VariableNode(std::string n);
    
    void accept(Visitor &v) override;
};

struct FunctionCallNode final: ASTNode {
    std::string name;
    std::vector<ASTN_ptr> arguments;
    explicit FunctionCallNode(std::string n, std::vector<ASTN_ptr>&& args);
    
    void accept(Visitor &v) override;
};

struct BinOpNode final: ASTNode {
    OpSign op;
    std::unique_ptr<ASTNode> left, right;
    
    explicit BinOpNode(OpSign op_, ASTN_ptr l, ASTN_ptr r);
    
    void accept(Visitor &v) override;
    // ASTN_ptr clone() const override{
    //     return std::make_unique<BinOpNode>(op, left->clone(), right->clone()); // как оно работает? 1+1+1+1+1+1+....+1 10000 операций, стек переполнен, иттеративное клонирование налево, направо, посетили
    // }
};

struct UnOpNode final: ASTNode {
    OpSign op;
    std::unique_ptr<ASTNode> middle;
    
    explicit UnOpNode(OpSign op_, ASTN_ptr m);

    void accept(Visitor &v) override;
    // ASTN_ptr clone() const override{
    //     return std::make_unique<BinOpNode>(op, left->clone(), right->clone()); // как оно работает? 1+1+1+1+1+1+....+1 10000 операций, стек переполнен, иттеративное клонирование налево, направо, посетили
    // }
};

class AST{
    ASTN_ptr root;
    //void accept(Visitor &v) override;

    public:
        AST(ASTN_ptr r);

        ASTNode* get_root() const;
};

class OperatorType{ // по-хорошему надо сделать по-хорошему, чтобы класс не офигевал от того, что он хранит
    int priority; // нужно также как и с ASTNode разбить всё на мелочи
    OpSign symbol; // тогда поучится разное управление над функцией, скобкой, бинарным и унарным операторами 
    std::string function = "[empty]"; // как костыль на время сойдёт
    bool is_left_assotiativity = true;
    bool is_function = false;

    public:
        OperatorType(Token op, bool is_unary, bool is_func);

        bool is_func() const;
        int get_priority() const;
        OpSign get_symbol() const;
        std::string get_function() const;
};

class Parser{
    Lexer lexer;
    class operation;
    std::vector<OperatorType> stack_op;
    std::vector<ASTN_ptr> tree_stack;
    std::vector<ASTN_ptr> stack_of_args;
    Token current;
    bool first_call = true;
    bool is_in_function = false;

    public:
    Parser (Lexer); // конструктор
    AST parse();

};

std::string fmt_num(double v);

std::string print_ast(const ASTNode* node);