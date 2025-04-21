#ifndef parser_hpp
#define parser_hpp
#include <iostream>
#include "ast.hpp"
#include "tokenstream.hpp"
using namespace std;

class Parser {
    private:
        bool inListConstructor;
        TokenStream ts;
        Token& current();
        Token& advance();
        Symbol lookahead();
        bool expect(Symbol sym);
        bool match(Symbol sym);
        astnode* paramList();
        astnode* argList();
        astnode* primary();
        astnode* val();
        astnode* unopExpression();
        astnode* factor();
        astnode* term();
        astnode* range();
        astnode* relOpExpression();
        astnode* equOpExpression();
        astnode* logicOpExpression();
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
    inListConstructor = false;
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

astnode* Parser::argList() {
    match(TK_LET);
    astnode* node = expression();
    astnode* c = node;
    while (expect(TK_COMA)) {
        match(TK_COMA);
        match(TK_LET);
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
            node->child[0] = expression();
            node->child[1] = makeBlock();
        } break;
        case TK_FUNC: {
            node = makeStmtNode(FUNC_DEF_STMT, current());
            match(TK_FUNC);
            node->token = current();
            match(TK_ID);
            match(TK_LP);
            node->child[0] = argList();
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
        case TK_LC: {
            node = makeStmtNode(BLOCK_STMT, current());
            node->child[0] = makeBlock();
        } break;
        case TK_EOI: break;
        case TK_RC: break;
        case TK_LB:
        case TK_APPEND:
        case TK_LP:
        case TK_NUM:
        case TK_ID:
        default:
            node = makeStmtNode(EXPR_STMT, current());
            node->child[0] = expression();
            break;
    }
    return node;
}

astnode* Parser::expression() {
    astnode* node = logicOpExpression();
    while (expect(TK_ASSIGN)) {
        astnode* t = makeExprNode(ASSIGN_EXPR, current());
        match(TK_ASSIGN);
        t->child[0] = node;
        t->child[1] = logicOpExpression();
        node = t;
    }
    
    return node;
}

astnode* Parser::logicOpExpression() {
    astnode* node = equOpExpression();
    while (expect(TK_AND) || expect(TK_OR)) {
        astnode* t = makeExprNode(LOGIC_EXPR, current());
        match(lookahead());
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
        node = range();
    }
    return node;
}

astnode* Parser::range() {
    astnode* node = val();
    if (expect(TK_RANGE)) {
        astnode* t = makeExprNode(RANGE_EXPR, current());
        match(TK_RANGE);
        t->child[0] = node;
        t->child[1] = val();
        node = t;
    }
    if (expect(TK_PIPE)) {
        astnode* t = makeExprNode(ZF_EXPR, current());
        match(TK_PIPE);
        t->child[0] = node;
        t->child[1] = val();
        if (expect(TK_PIPE)) {
            match(TK_PIPE);
            t->child[2] = val();
        }
        node = t;
    }
    return node;
}

astnode* Parser::val() {
    astnode* node = primary();
    if (expect(TK_POST_DEC) || expect(TK_POST_INC)) {
        astnode* t = makeExprNode(UNOP_EXPR, current());
        match(lookahead());
        t->child[0] = node;
        node = t;
    }
    while (expect(TK_LB)) {
        astnode* t = makeExprNode(SUBSCRIPT_EXPR, current());
        match(TK_LB);
        t->child[0] = node;
        t->child[1] = expression();
        node = t;
        match(TK_RB);
    }
    while (expect(TK_POW)) {
        astnode* t = makeExprNode(BINOP_EXPR, current());
        match(TK_POW);
        t->child[0] = node;
        t->child[1] = expression();
        node = t;
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
    astnode* node = nullptr;
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
        inListConstructor = true;
        node->child[0] = paramList();
        inListConstructor = false;
        match(TK_RB);
    } else if (expect(TK_APPEND) || expect(TK_PUSH) || expect(TK_MAP) || expect(TK_FILTER) || expect(TK_REDUCE)) {
        node = makeExprNode(LIST_EXPR, current());
        match(lookahead());
        match(TK_LP);
        node->child[0] = expression();
        match(TK_COMA);
        node->child[1] = expression();
        match(TK_RP);
    } else if (expect(TK_SIZE) || expect(TK_EMPTY) || expect(TK_FIRST) || expect(TK_REST)) {
        node = makeExprNode(LIST_EXPR, current());
        match(lookahead());
        match(TK_LP);
        node->child[0] = expression();
        match(TK_RP);
    } else if (expect(TK_MATCHRE)) {
        node = makeExprNode(REG_EXPR, current());
        match(TK_MATCHRE);
        match(TK_LP);
        node->child[0] = expression();
        match(TK_COMA);
        node->child[1] = expression();
        match(TK_RP);
    } else if (expect(TK_LAMBDA)) {
        node = makeExprNode(LAMBDA_EXPR, current());
        match(TK_LAMBDA);
        node->child[0] = paramList();
        match(TK_RP);
        if (expect(TK_PRODUCES)) {
            match(TK_PRODUCES);
            node->child[1] = expression();
        } else {
            node->child[1] = makeBlock();
        }
    } else if (expect(TK_BLESS)) {
        node = makeExprNode(BLESS_EXPR, current());
        match(TK_BLESS);
        node->child[0] = primary();
    }
    return node;
}

#endif