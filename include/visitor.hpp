#pragma once
#include "parser.hpp"

#include <unordered_map>
#include <stack>
#include <string>

class NumberNode;
class VariableNode;
class BinOpNode;
class UnOpNode;
class FunctionCallNode;

class Visitor{
    public:
        virtual ~Visitor() = default;

        virtual void visit(NumberNode &) = 0;
        virtual void visit(VariableNode &) = 0;
        virtual void visit(BinOpNode &) = 0;
        virtual void visit(UnOpNode &) = 0;
        virtual void visit(FunctionCallNode &) = 0;
};

class Evaluator final: public Visitor{
    std::unordered_map<std::string, double> map_for_vars;
    std::stack<double> stack_result;

    double call_func(const std::string& name, std::vector<double>& args);

    public:

        double get_result() const;

        void visit(NumberNode &) override;
        void visit(VariableNode &) override;
        void visit(BinOpNode &) override;
        void visit(UnOpNode &) override;
        void visit(FunctionCallNode &) override;

        Evaluator(std::unordered_map<std::string, double>& vars);
};

/*
class Derivative final: public Visitor{
    std::unordered_map<std::string, double> map_for_vars;
    std::stack<double> stack_result;

    double call_func(const std::string& name, std::vector<double>& args);

    public:

        double get_result() const;

        void visit(NumberNode &) override;
        void visit(VariableNode &) override;
        void visit(BinOpNode &) override;
        void visit(UnOpNode &) override;
        void visit(FunctionCallNode &) override;

        Derivative(std::unordered_map<std::string, double>& vars);
};
*/