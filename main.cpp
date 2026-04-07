#include "include/lexer.hpp"
#include "include/parser.hpp"
#include "include/visitor.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <limits>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw std::runtime_error("ERROR [INSERT]: No arguments in command string");
        
        int count_of_vars = 0;
        if (!(std::cin >> count_of_vars) || count_of_vars < 0) {
            throw std::runtime_error("ERROR [INSERT]: Count of variables should be a non-negative integer");
        }
        
        std::vector<std::string> var_names(count_of_vars);
        std::unordered_map<std::string, double> variables;
        
        for (int i = 0; i < count_of_vars; ++i) {
            if (!(std::cin >> var_names[i])) throw std::runtime_error("ERROR [INSERT]: Invalid variable name");

            if (unforgivable_names_of_vars.find(var_names[i]) != unforgivable_names_of_vars.end()) {
                throw std::runtime_error("ERROR [INSERT]: Variable name '" + var_names[i] + "' is reserved and cannot be used.");
            }
        }

        for (int i = 0; i < count_of_vars; ++i) {
            double val;
            if (!(std::cin >> val)) throw std::runtime_error("ERROR [INSERT]: Invalid variable value");
            variables[var_names[i]] = val;
        }
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::string expression;
        std::getline(std::cin, expression);
        
        std::cout << "__________________________LEXER'S_PART__________________________\n";
        std::cout << lexer_flow_concatenator(expression);
        
        std::cout << "__________________________PARSER'S_PART__________________________\n";
        
        Lexer lexer(expression);
        Parser parser(lexer);
        AST tree = parser.parse();
        
        std::cout << print_ast(tree.get_root()) << "\n";
        
        
        std::string differentiation_variable = (argc >= 3) ? argv[2] : "x";

        if (std::string(argv[1]) == "evaluate"){
            std::cout << "__________________________EVALUATOR'S_PART__________________________\n";
            Evaluator evaluator(variables);
            tree.get_root()->accept(evaluator);
            std::cout << "Result: " << evaluator.get_result() << "\n";
            return 0;
        } else if (std::string(argv[1]) == "deriviative"){
            std::cout << "__________________________DERIVATIVE'S_PART__________________________\n";
            Derivative derivative(differentiation_variable);
            tree.get_root()->accept(derivative);
            AST derivative_tree = derivative.get_result();
            std::cout << "Derivative: " << print_ast(derivative_tree.get_root()) << "\n";
            return 0;
        } else if (std::string(argv[1]) == "evaluate_deriviative"){
            std::cout << "__________________________DERIVALUATOR'S_PART__________________________\n";
            Derivative derivative(differentiation_variable);
            tree.get_root()->accept(derivative);
            AST derivative_tree = derivative.get_result();
            std::cout << "Derivative: " << print_ast(derivative_tree.get_root()) << "\n";

            Evaluator evaluator(variables);
            derivative_tree.get_root()->accept(evaluator);
            std::cout << "Result: " << evaluator.get_result() << "\n";
            return 0;
        } else {
            throw std::runtime_error("ERROR [INSERT]: Unknown request. You should use: evaluate, deriviative, evaluate_deriviative");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
