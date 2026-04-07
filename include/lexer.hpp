#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <limits>

enum class lexem_t{
    NUMBER,
    IDENTIFICATOR,
    OPERATOR,

    L_CIRCLE_PAPAREN,
    R_CIRCLE_PAPAREN,
    L_SQUARE_PAPAREN,
    R_SQUARE_PAPAREN,
    L_FIGURE_PAPAREN,
    R_FIGURE_PAPAREN,

    EOEX,
    UNKNOWN
};

enum class lexer_state_t{
    START,

    NUM_INT, // 0,1,2,3,4,5,6,7,8,9
    NUM_FRAC, // .
    NUM_EXP, // 0.5e8 0.5e+8 0.5e-8
    NUM_WAIT_FOR_POINT,
    NUM_SIGN_EXP,
    NUM_AFTER_EXP,
    NUM_AFTER_FRAC,

    ID_CHAR
};

struct Token{
    size_t tok_pos = 0;
    std::string value = "";
    lexem_t type;

    public:

        //Token (std::string value_, lexem_t type_):  value(value_), type(type_){};
        void set_value(std::string& val);
        std::string get_value() const;

        void set_tok_pos(size_t pos);
        size_t get_tok_pos() const;

        void set_type(lexem_t t);
        lexem_t get_type() const;

        std::string view() const;

};

class Lexer{
    std::string expression;
    size_t position;
    size_t tok_pos;
    std::string buffer = "";
    //std::vector<std::string> list_of_toks;
    lexer_state_t state = lexer_state_t::START;

    bool flag = true;

    Token scan_indentificator(); // не публичный !!!
    Token scan_number(); // не публичный !!!
    Token scan_operator();
    //Token scan(); // не публичный !!!

    public:
        Token next(); // переписать на peek(), next() вызывает peek() и двигает 
        //Token peek(); // сделать вектор токенов, возможно понадобится, проверка. если после индефикатора скобка - это функция 

        lexem_t peek_type() const;

        char peek_char() const;
        bool is_paren() const;
        size_t get_position() const;

        //std::string get_expr() const{return expression;};

        Lexer(const std::string&);

        void advance(); // продвижение
};

std::string lexer_flow_concatenator(const std::string& expression);