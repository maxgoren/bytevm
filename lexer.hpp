#ifndef lexer_hpp
#define lexer_hpp
#include <vector>
#include "token.hpp"
#include "tokenstream.hpp"
#include "stringbuffer.hpp"
using namespace std;

class Lexer {
    private:
        void skipWhiteSpace(StringBuffer& sb) {
            while (!sb.done()) {
                if (sb.get() == ' ' || sb.get() == '\t' || sb.get() == '\r') {
                    sb.advance();
                } else break;
            }
        }
        void skipComments(StringBuffer& sb) {
            if (sb.get() == '{') {
                sb.advance();
                if (sb.get() == '*') {
                    sb.advance();
                    while (!sb.done()) {
                        if (sb.get() == '*') {
                            sb.advance();
                            if (sb.get() == '}') {
                                sb.advance();
                                skipWhiteSpace(sb); 
                                return;
                            }
                        }
                        sb.advance();
                    }
                }
                sb.rewind();
            }
        }
        Token extractNumber(StringBuffer& sb) {
            string num;
            while (!sb.done()) {
                if (isdigit(sb.get()) || sb.get() == '.') {
                    num.push_back(sb.get());
                    sb.advance();
                } else break;
            }
            return Token(TK_NUM, num);
        }
        Token extractId(StringBuffer& sb) {
            string id;
            while (!sb.done()) {
                if (isalpha(sb.get()) || isdigit(sb.get())) {
                    id.push_back(sb.get());
                    sb.advance();
                } else break;
            }
            return checkReserved(id);
        }
        Token extractString(StringBuffer& sb) {
            string str;
            sb.advance();
            while (!sb.done()) {
                if (sb.get() == '"') 
                    break;
                str.push_back(sb.get());
                sb.advance();
            }
            if (sb.get() == '"') {
                sb.advance();
            } else {
                cout<<"Error: unterminated string."<<endl;
            }
            return Token(TK_STR, str);
        }
        Token checkReserved(string id) {
            if (id == "if")      return Token(TK_IF, "if");
            if (id == "else")    return Token(TK_ELSE, "else");
            if (id == "let")     return Token(TK_LET, "let");
            if (id == "var")     return Token(TK_LET, "var");
            if (id == "print")   return Token(TK_PRINT, "print");
            if (id == "println") return Token(TK_PRINTLN, "println");
            if (id == "return")  return Token(TK_RETURN, "return");
            if (id == "while")   return Token(TK_WHILE, "while");
            if (id == "func")    return Token(TK_FUNC, "func");
            if (id == "def")     return Token(TK_FUNC, "def");
            if (id == "struct")  return Token(TK_STRUCT, "struct");
            if (id == "push")    return Token(TK_PUSH, "push");
            if (id == "append")  return Token(TK_APPEND, "append"); 
            if (id == "size")    return Token(TK_SIZE, "size");
            if (id == "empty")   return Token(TK_EMPTY, "empty");
            if (id == "first")   return Token(TK_FIRST, "first");
            if (id == "rest")    return Token(TK_REST, "rest");
            if (id == "map")     return Token(TK_MAP, "map");
            if (id == "filter")  return Token(TK_FILTER, "filter");
            if (id == "reduce")  return Token(TK_REDUCE, "reduce");
            if (id == "nil")     return Token(TK_NIL, "nil");
            if (id == "true")    return Token(TK_TRUE, "true");
            if (id == "false")   return Token(TK_FALSE, "false");
            if (id == "matchre") return Token(TK_MATCHRE, "matchre");
            if (id == "bless")   return Token(TK_BLESS, "bless");
            return Token(TK_ID, id);
        }
        Token checkSpecials(StringBuffer& sb) {
            switch (sb.get()) {
                case '%': return Token(TK_MOD, "%");
                case '/': return Token(TK_DIV, "/");
                case '(': return Token(TK_LP, "(");
                case ')': return Token(TK_RP, ")");
                case '{': return Token(TK_LC, "{");
                case '}': return Token(TK_RC, "}");
                case '[': return Token(TK_LB, "[");
                case ']': return Token(TK_RB, "]");
                case ',': return Token(TK_COMA, ",");
                case ';': return Token(TK_SEMI, ";");
                default: break;
            }
            if (sb.get() == '+') {
                sb.advance();
                if (sb.get() == '+') {
                    return Token(TK_POST_INC,"++");
                }
                sb.rewind();
                return Token(TK_ADD, "+");
            }
            if (sb.get() == '|') {
                sb.advance();
                if (sb.get() == '|') {
                    return Token(TK_OR, "||");
                }
                sb.rewind();
                return Token(TK_PIPE, "|");
            }
            if (sb.get() == '*') {
                sb.advance();
                if (sb.get() == '*') {
                    return Token(TK_POW, "**");
                }
                sb.rewind();
                return Token(TK_MUL, "*");
            }
            if (sb.get() == '-') {
                sb.advance(); 
                if (sb.get() == '>') {
                    return Token(TK_PRODUCES, "->");
                } else if (sb.get() == '-') {
                    return Token(TK_POST_DEC, "--");
                }
                sb.rewind();
                return Token(TK_SUB, "-");
            }
            if (sb.get() == '.') {
                sb.advance();
                if (sb.get() == '.') {
                    return Token(TK_RANGE, "..");
                }
                sb.rewind();
                return Token(TK_PERIOD, ".");
            }
            if (sb.get() == '<') {
                sb.advance();
                if (sb.get() == '=') {
                    return Token(TK_LTE, "<=");
                }
                sb.rewind();
                return Token(TK_LT, "<");
            }
            if (sb.get() == '>') {
                sb.advance();
                if (sb.get() == '=') {
                    return Token(TK_GTE, ">=");
                }
                sb.rewind();
                return Token(TK_GT, ">");
            }
            if (sb.get() == '=') {
                sb.advance();
                if (sb.get() == '=') {
                    return Token(TK_EQU, "==");
                }
                sb.rewind();
            }
            if (sb.get() == '!') {
                sb.advance();
                if (sb.get() == '=') {
                    return Token(TK_NEQ, "!=");
                }
                sb.rewind();
                return Token(TK_NOT, "!");
            }
            if (sb.get() == ':') {
                sb.advance();
                if (sb.get() == '=') {
                    return Token(TK_ASSIGN, ":=");
                }
                sb.rewind();
                return Token(TK_COLON, ":");
            }
            if (sb.get() == '&') {
                sb.advance();
                if (sb.get() == '(') {
                    return Token(TK_LAMBDA, "&(");
                } else if (sb.get() == '&') {
                    return Token(TK_AND, "&&");
                }
                sb.rewind();
                return Token(TK_AMPER, "&");
            }
            return Token(TK_ERR, "err");
        }
    public:
        Lexer() {

        }
        TokenStream lex(StringBuffer sb) {
            TokenStream ts;
            while (!sb.done()) {
                skipWhiteSpace(sb);
                skipComments(sb);
                if (isdigit(sb.get())) {
                    ts.append(extractNumber(sb));
                } else if (isalpha(sb.get())) {
                    ts.append(extractId(sb));
                } else if (sb.get() == '"') {
                    ts.append(extractString(sb));
                } else {
                    ts.append(checkSpecials(sb));
                    sb.advance();
                }
            }
            ts.append(Token(TK_EOI, "<fin.>"));
            return ts;
        }
};

#endif