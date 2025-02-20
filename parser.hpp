#ifndef parser_hpp
#define parser_hpp
#include <iostream>
#include "ast.hpp"
#include "tokenstream.hpp"
using namespace std;

class Parser {
    private:
        TokenStream ts;
        Token& current();
        Token& advance();
        Symbol lookahead();
        bool expect(Symbol sym);
        bool match(Symbol sym);
        astnode* paramList();
        astnode* primary();
        astnode* val();
        astnode* unopExpression();
        astnode* factor();
        astnode* term();
        astnode* relOpExpression();
        astnode* equOpExpression();
        astnode* expression();
        astnode* makeBlock();
        astnode* statement();
        astnode* statementList();
        astnode* program();
    public:
        Parser();
        astnode* parse(TokenStream& tokens);
};

Parser::Parser() {

}

astnode* Parser::parse(TokenStream& tokens) {
    ts = tokens;
    ts.start();
    astnode* ast = program();
    match(TK_EOI);
    return ast;
}

Token& Parser::current() {
    return ts.get();
}
Token& Parser::advance() {
    ts.advance();
    return ts.get();
}
Symbol Parser::lookahead() {
    return ts.get().symbol;
}
bool Parser::expect(Symbol sym) {
    return sym == ts.get().symbol;
}
bool Parser::match(Symbol sym) {
    if (sym == ts.get().symbol) {
        ts.advance();
        return true;
    }
    cout<<"Error: unexpected token "<<ts.get().strval<<endl;
    return false;
}

astnode* Parser::program() {
    astnode* node = statementList();
    return node;
}

astnode* Parser::statementList() {
    astnode* node = statement();
    astnode* m = node;
    while (!expect(TK_RC) && !expect(TK_EOI)) {
        if (expect(TK_SEMI))
            match(TK_SEMI);
        astnode* t = statement();
        if (m == nullptr) {
            node = m = t;
        } else {
            m->next = t;
            m = t;
        }
    }
    return node;
}

astnode* Parser::paramList() {
    astnode* node = expression();
    astnode* c = node;
    while (expect(TK_COMA)) {
        match(TK_COMA);
        c->next = expression();
        c = c->next;
    }
    return node;
}

astnode* Parser::makeBlock() {
    match(TK_LC);
    astnode* node = statementList();
    match(TK_RC);
    return node;
}

astnode* Parser::statement() {
    astnode* node = nullptr;
    switch (lookahead()) {
        case TK_PRINTLN:
        case TK_PRINT: {
            node = makeStmtNode(PRINT_STMT, current());
            match(lookahead());
            node->child[0] = expression();
        } break;
        case TK_LET: {
            node = makeStmtNode(LET_STMT, current());
            match(TK_LET);
            node->child[0] = expression();
        } break;
        case TK_STRUCT: {
            node = makeStmtNode(STRUCT_DEF_STMT, current());
            match(TK_STRUCT);
            node->child[0] = makeBlock();
        } break;
        case TK_FUNC: {
            node = makeStmtNode(FUNC_DEF_STMT, current());
            match(TK_FUNC);
            node->token = current();
            match(TK_ID);
            match(TK_LP);
            node->child[0] = paramList();
            match(TK_RP);
            node->child[1] = makeBlock();
        } break;
        case TK_RETURN: {
            node = makeStmtNode(RETURN_STMT, current());
            match(TK_RETURN);
            node->child[0] = expression();
        } break;
        case TK_IF: {
            node = makeStmtNode(IF_STMT, current());
            match(TK_IF);
            match(TK_LP);
            node->child[0] = expression();
            match(TK_RP);
            node->child[1] = makeBlock();
            if (expect(TK_ELSE)) {
                match(TK_ELSE);
                node->child[2] = makeBlock();
            }
        } break;
        case TK_WHILE: {
            node = makeStmtNode(WHILE_STMT, current());
            match(TK_WHILE);
            match(TK_LP);
            node->child[0] = expression();
            match(TK_RP);
            node->child[1] = makeBlock();
        } break;
        case TK_LB:
        case TK_APPEND:
        case TK_LP:
        case TK_NUM:
        case TK_ID:
            node = makeStmtNode(EXPR_STMT, current());
            node->child[0] = expression();
            break;
        default:
            break;
    }
    return node;
}

astnode* Parser::expression() {
    astnode* node = equOpExpression();
    while (expect(TK_ASSIGN)) {
        astnode* t = makeExprNode(ASSIGN_EXPR, current());
        match(TK_ASSIGN);
        t->child[0] = node;
        t->child[1] = equOpExpression();
        node = t;
    }
    return node;
}

astnode* Parser::equOpExpression() {
    astnode* node = relOpExpression();
    while (expect(TK_EQU) || expect(TK_NEQ)) {
        astnode* t = makeExprNode(RELOP_EXPR, current());
        match(lookahead());
        t->child[0] = node;
        t->child[1] = relOpExpression();
        node = t;
    }
    return node;
}

astnode* Parser::relOpExpression() {
    astnode* node = term();
    while (expect(TK_LT) || expect(TK_GT) || expect(TK_LTE) || expect(TK_GTE)) {
        astnode* t = makeExprNode(RELOP_EXPR, current());
        match(lookahead());
        t->child[0] = node;
        t->child[1] = term();
        node = t;
    }
    return node;
}

astnode* Parser::term() {
    astnode* node = factor();
    while (expect(TK_ADD) || expect(TK_SUB)) {
        astnode* t = makeExprNode(BINOP_EXPR, current());
        match(lookahead());
        t->child[0] = node;
        t->child[1] = factor();
        node = t;
    }
    return node;
}


astnode* Parser::factor() {
    astnode* node = unopExpression();
    while (expect(TK_MUL) || expect(TK_DIV) || expect(TK_MOD)) {
        astnode* t = makeExprNode(BINOP_EXPR, current());
        match(lookahead());
        t->child[0] = node;
        t->child[1] = unopExpression();
        node = t;
    }
    return node;
}

astnode* Parser::unopExpression() {
    astnode* node = nullptr;
    if (expect(TK_SUB)) {
        node = makeExprNode(UNOP_EXPR, current());
        match(TK_SUB);
        node->child[0] = unopExpression();
    } else if (expect(TK_NOT)) {
        node = makeExprNode(UNOP_EXPR, current());
        match(TK_NOT);
        node->child[0] = unopExpression();
    } else {
        node = val();
    }
    return node;
}

astnode* Parser::val() {
    astnode* node = primary();
    while (expect(TK_LB)) {
        astnode* t = makeExprNode(SUBSCRIPT_EXPR, current());
        match(TK_LB);
        t->child[0] = node;
        t->child[1] = expression();
        node = t;
        match(TK_RB);
    }
    if (expect(TK_LP)) {
        astnode* t = makeExprNode(FUNC_EXPR, current());
        t->child[0] = node;
        match(TK_LP);
        t->child[1] = paramList();
        match(TK_RP);
        node = t;
    }
    return node;
}

astnode* Parser::primary() {
    astnode* node;
    if (expect(TK_NUM) || expect(TK_STR) || expect(TK_TRUE) || expect(TK_FALSE) || expect(TK_NIL)) {
        node = makeExprNode(CONST_EXPR, current());
        match(lookahead());
    } else if (expect(TK_ID)) {
        node = makeExprNode(ID_EXPR, current());
        match(TK_ID);
    } else if (expect(TK_LP)) {
        match(TK_LP);
        node = expression();
        match(TK_RP);
    } else if (expect(TK_LB)) {
        node = makeExprNode(LIST_EXPR, current());
        match(TK_LB);
        node->child[0] = paramList();
        match(TK_RB);
    } else if (expect(TK_APPEND) || expect(TK_PUSH)) {
        node = makeExprNode(LIST_EXPR, current());
        match(lookahead());
        match(TK_LP);
        node->child[0] = expression();
        match(TK_COMA);
        node->child[1] = expression();
        match(TK_RP);
    } else if (expect(TK_SIZE) || expect(TK_EMPTY)) {
        node = makeExprNode(LIST_EXPR, current());
        match(lookahead());
        match(TK_LP);
        node->child[0] = expression();
        match(TK_RP);
    }
    return node;
}

#endif