#ifndef token_hpp
#define token_hpp
#include <iostream>
using namespace std;

enum Symbol {
    TK_ID, TK_NUM, TK_STR, TK_TRUE, TK_FALSE, TK_NIL, TK_LP, TK_RP, TK_LC, TK_RC, TK_LB, TK_RB,
    TK_ADD, TK_MUL, TK_SUB, TK_DIV, TK_MOD, TK_POW, TK_SQRT, TK_POST_INC, TK_POST_DEC,
    TK_LT, TK_GT, TK_LTE, TK_GTE, TK_EQU, TK_NEQ, TK_NOT, TK_AND, TK_OR, TK_BLESS,
    TK_PERIOD, TK_COMA, TK_SEMI, TK_COLON, TK_AMPER, TK_QM, TK_REF, TK_LAMBDA, TK_RANGE,
    TK_ASSIGN, TK_QUOTE, TK_FUNC, TK_PRODUCES, TK_STRUCT, TK_NEW, TK_FREE,
    TK_LET, TK_VAR,  TK_PRINT, TK_PRINTLN, TK_WHILE, TK_RETURN, TK_IF, TK_ELSE,
    TK_PUSH, TK_APPEND, TK_EMPTY, TK_SIZE, TK_FIRST, TK_REST, TK_MAP, TK_FILTER, TK_REDUCE,
    TK_SORT, TK_PIPE, TK_MATCHRE, TK_TYPEOF,
    TK_ERR, TK_EOI

};

string symbolStr[] = {
    "TK_ID", "TK_NUM", "TK_STR", "TK_TRUE", "TK_FALSE", "TK_NIL", "TK_LP", "TK_RP", "TK_LC", "TK_RC", "TK_LB", "TK_RB",
    "TK_ADD", "TK_MUL", "TK_SUB", "TK_DIV", "TK_MOD", "TK_POW", "TK_SQRT", "TK_POST_INC", "TK_POST_DEC",
    "TK_LT", "TK_GT", "TK_LTE", "TK_GTE", "TK_EQU", "TK_NEQ", "TK_NOT", "TK_AND", "TK_OR", "TK_BLESS",
    "TK_PERIOD", "TK_COMA", "TK_SEMI", "TK_COLON", "TK_AMPER", "TK_QM", "TK_REF","TK_LAMBDA", "TK_RANGE",
    "TK_ASSIGN", "TK_QUOTE", "TK_FUNC", "TK_PRODUCES", "TK_STRUCT", "TK_NEW", "TK_FREE",
    "TK_LET", "TK_VAR", "TK_PRINT", "TK_PRINTLN", "TK_WHILE", "TK_RETURN", "TK_IF", "TK_ELSE",
    "TK_PUSH", "TK_APPEND", "TK_EMPTY", "TK_SIZE", "TK_FIRST", "TK_REST", "TK_MAP", "TK_FILTER", 
    "TK_REDUCE", "TK_SORT", "TK_PIPE", "TK_MATCHRE", "TK_TYPEOF", "TK_ERR", "TK_EOI"
};

struct Token {
    Symbol symbol;
    string strval;
    int depth;
    Token(Symbol s = TK_EOI, string st = " ", int d = -1) : symbol(s), strval(st), depth(d) { }
};

void printToken(Token tk) {
    cout<<"["<<symbolStr[tk.symbol]<<", "<<tk.strval<<"]"<<endl;
}


#endif