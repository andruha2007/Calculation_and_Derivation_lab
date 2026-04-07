#include "../include/lexer.hpp"


void Token::set_value(std::string& val) {value = val;};
std::string Token::get_value() const{return value;};

void Token::set_tok_pos(size_t pos) {tok_pos = pos;};
size_t Token::get_tok_pos() const{return tok_pos;};

void Token::set_type(lexem_t t){type = t;};
lexem_t Token::get_type() const{return type;};

std::string Token::view() const{return value;};


Lexer::Lexer(const std::string& expression_): expression(expression_){};

void Lexer::advance(){++position;}

char Lexer::peek_char() const{
    if (position >= expression.length()) return '\0';
    return expression[position];};

lexem_t Lexer::peek_type() const{
    char c = peek_char();

    if (c == '\0'){
        return lexem_t::EOEX;
    } if (std::isdigit(c) || c == '.'){
        return lexem_t::NUMBER;
    } if (std::isalpha(c) || c == '_'){
        return lexem_t::IDENTIFICATOR;
    } if (c == '+' || c == '-' || c == '^' || c == '*' || c == '/' || c == ','){ // c == '%' || c == '!'
        return lexem_t::OPERATOR;
    } if (c == '('){
        return lexem_t::L_CIRCLE_PAPAREN;
    } if (c == ')'){
        return lexem_t::R_CIRCLE_PAPAREN;
    } if (c == '['){
        return lexem_t::L_SQUARE_PAPAREN;
    } if (c == ']'){
        return lexem_t::R_SQUARE_PAPAREN;
    } if (c == '{'){
        return lexem_t::L_FIGURE_PAPAREN;
    } if (c == '}'){
        return lexem_t::R_FIGURE_PAPAREN;
    }
    return lexem_t::UNKNOWN;
}

bool Lexer::is_paren() const{
    lexem_t lex = peek_type();
    if (lex == lexem_t::L_CIRCLE_PAPAREN ||
        lex == lexem_t::R_CIRCLE_PAPAREN ||
        lex == lexem_t::L_SQUARE_PAPAREN ||
        lex == lexem_t::R_SQUARE_PAPAREN ||
        lex == lexem_t::L_FIGURE_PAPAREN ||
        lex == lexem_t::R_FIGURE_PAPAREN) return true;
    return false;
}

size_t Lexer::get_position() const { return position;};

Token Lexer::next(){
    if (flag){
        tok_pos = 0;
        position = 0;
        flag = false;
    }

    Token tok;
    
    if(peek_char() == ' ' && position == 0){
        throw std::runtime_error("ERROR [Lexer] Spaces are not available at the beginning");
    } if (peek_type() == lexem_t::EOEX){
        tok.set_type(lexem_t::EOEX);
        return tok;
    }

    while (peek_char() == ' '){
        advance();
    }

    lexem_t lex = peek_type();
    char c = peek_char();

    switch (lex)
    {
    case lexem_t::NUMBER:
        return scan_number();
    case lexem_t::IDENTIFICATOR:
        return scan_indentificator();
    case lexem_t::OPERATOR:
        return scan_operator();
    case lexem_t::L_CIRCLE_PAPAREN:
        tok.set_type(lexem_t::L_CIRCLE_PAPAREN); break;
    case lexem_t::R_CIRCLE_PAPAREN:
        tok.set_type(lexem_t::R_CIRCLE_PAPAREN); break;
    case lexem_t::L_SQUARE_PAPAREN:
        tok.set_type(lexem_t::L_SQUARE_PAPAREN); break;
    case lexem_t::R_SQUARE_PAPAREN:
        tok.set_type(lexem_t::R_SQUARE_PAPAREN); break;
    case lexem_t::L_FIGURE_PAPAREN:
        tok.set_type(lexem_t::L_FIGURE_PAPAREN); break;
    case lexem_t::R_FIGURE_PAPAREN:
        tok.set_type(lexem_t::R_FIGURE_PAPAREN); break;
    default:
        throw std::runtime_error("ERROR [Lexer] Unknown symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
        break;
    }

    tok.set_tok_pos(tok_pos);
    ++tok_pos;
    buffer += c;
    tok.set_value(buffer);
    buffer = "";
    advance();
    
    return tok;    
}

Token Lexer::scan_indentificator(){
    Token tok;
    tok.set_type(lexem_t::IDENTIFICATOR);
    tok.set_tok_pos(tok_pos);

    while (peek_type() != lexem_t::EOEX && peek_type() != lexem_t::OPERATOR && !is_paren() && peek_char() != ' '){        
        char c = peek_char();
        if (std::isalpha(c)) c = tolower(c);
        switch (state){
        case lexer_state_t::START:
            if (std::isalpha(c) || c == '_'){
                advance();
                state = lexer_state_t::ID_CHAR;
            } else{
                throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
            }
            break;
        case lexer_state_t::ID_CHAR:
            if(std::isdigit(c) || std::isalpha(c) || c =='_'){
                advance();
                state = lexer_state_t::ID_CHAR;
            } else{
                throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
            }
            break;
        default:
            break;
        }

        buffer += c;
    }

    ++tok_pos;
    tok.set_value((buffer));
    buffer = "";
    state = lexer_state_t::START;

    return tok;
}


Token Lexer::scan_operator(){
    Token tok;
    tok.set_type(lexem_t::OPERATOR);
    tok.set_tok_pos(tok_pos);

    char c = peek_char();
    buffer += c;
    advance();
    
    // if (c == '^' || c == '*' || c == '/' || c == '%' || c == '!'){
    //     buffer += c;
    //     advance();
    // } else if (c =='+' || c == '-'){
    //     buffer += c;
    //     advance();
    //     if (c == peek_char()){
    //         buffer += c;
    //         advance();
    //     }
    // }

    ++tok_pos;
    tok.set_value(buffer);
    buffer = "";
    return tok;

}

Token Lexer::scan_number(){
    Token tok;
    tok.set_type(lexem_t::NUMBER);
    tok.set_tok_pos(tok_pos);

    bool ignore_op = false;

    while (peek_type() != lexem_t::EOEX && (peek_type() != lexem_t::OPERATOR || ignore_op) && !is_paren() && peek_char() != ' '){
        char c = peek_char();
        switch (state){
            case lexer_state_t::START:
                if (std::isdigit(c) && c >= '1' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_INT;
                } else if(static_cast<int>(c) == 48){
                    advance();
                    state = lexer_state_t::NUM_WAIT_FOR_POINT;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_WAIT_FOR_POINT:
                if(c == '.'){
                    advance();
                    state = lexer_state_t::NUM_FRAC;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_INT:
                if (std::isdigit(c) && c >= '0' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_INT;
                } else if(c == '.'){
                    advance();
                    state = lexer_state_t::NUM_FRAC;
                } else if (c == 'e'){
                    advance();
                    ignore_op = true;
                    state = lexer_state_t::NUM_EXP;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_EXP:
                if (std::isdigit(c) && c >= '1' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_AFTER_EXP;
                }if (c == '+' || c == '-'){
                    advance();
                    state = lexer_state_t::NUM_SIGN_EXP;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_SIGN_EXP:
                if (std::isdigit(c) && c >= '1' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_AFTER_EXP;
                    ignore_op = false;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_AFTER_EXP:
                if (std::isdigit(c) && c >= '0' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_AFTER_FRAC;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            
            case lexer_state_t::NUM_FRAC:
                if (std::isdigit(c) && c >= '0' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_AFTER_FRAC;
                } else{
                    throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            case lexer_state_t::NUM_AFTER_FRAC:
                if (std::isdigit(c) && c >= '0' && c <= '9'){
                    advance();
                    state = lexer_state_t::NUM_AFTER_FRAC;
                } else if (c == 'e'){
                    ignore_op = true;
                    advance();
                    state = lexer_state_t::NUM_EXP;
                } else{
                   throw std::runtime_error("ERROR [Lexer] Unexpected symbol \"" + std::string(1, peek_char()) + "\" at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
                }
                break;
            
            default:
                throw std::runtime_error("ERROR [Lexer] Unexppected lexer status");
                break;
        }

        buffer += c;
    }

    if (state == lexer_state_t::NUM_FRAC || state == lexer_state_t::NUM_EXP || state == lexer_state_t::NUM_SIGN_EXP){
        throw std::runtime_error("ERROR [Lexer] Unexpected number "+ buffer + "at position " + std::to_string(position) + " token position " + std::to_string(tok_pos));
    }

    ++tok_pos;
    tok.set_value(buffer);
    buffer = "";
    state = lexer_state_t::START;

    return tok;
}

std::string lexer_flow_concatenator(const std::string& expression) {
    Lexer lexer(expression);
    std::string result;
    Token tok = lexer.next();
    while (tok.get_type() != lexem_t::EOEX) {
        result += tok.view() + "\n";
        tok = lexer.next();
    }
    return result;
}