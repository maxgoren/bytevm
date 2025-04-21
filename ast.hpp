#ifndef ast_hpp
#define ast_hpp
#include "token.hpp"
#include <list>
using namespace std;


const int MAX_CHILD = 3;

enum NodeKind {
    EXPR_NODE, STMT_NODE
};

enum ExprType {
    ID_EXPR, CONST_EXPR, 
    UNOP_EXPR, BINOP_EXPR, RELOP_EXPR, LOGIC_EXPR,
    REG_EXPR, LAMBDA_EXPR, FUNC_EXPR, BLESS_EXPR,
    ASSIGN_EXPR, SUBSCRIPT_EXPR, RANGE_EXPR,
    LIST_EXPR, ZF_EXPR
};

enum StmtType {
    PRINT_STMT, EXPR_STMT, BLOCK_STMT, 
    IF_STMT, WHILE_STMT, LET_STMT, 
    FUNC_DEF_STMT, STRUCT_DEF_STMT, RETURN_STMT
};

struct astnode {
    NodeKind nk;
    union {
        ExprType expr;
        StmtType stmt;
    } type;
    Token token;
    astnode* child[MAX_CHILD];
    astnode* next;
    astnode(NodeKind kind, Token t) : nk(kind), token(t), next(nullptr) { 
        for (int i = 0; i < MAX_CHILD; i++)
            child[i] = nullptr;
    }
};



void preorder(astnode* expr, int d) {
    if (expr != nullptr) {
        for (int i = 0; i < d; i++) cout<<" ";
        switch(expr->nk) {
            case EXPR_NODE:
                switch (expr->type.expr) {
                    case ID_EXPR:    cout<<"[id expr]"; break;
                    case CONST_EXPR: cout<<"[cosnt expr]"; break;
                    case UNOP_EXPR:  cout<<"[unop expr]"; break;
                    case BINOP_EXPR: cout<<"[binop expr]"; break;
                    case RELOP_EXPR: cout<<"[relop expr]"; break;
                    case FUNC_EXPR:  cout<<"[func expr]"; break;
                    case ASSIGN_EXPR: cout<<"[assign expr]"; break;
                    case LIST_EXPR: cout<<"[list expr]"; break;
                    case SUBSCRIPT_EXPR: cout<<"[subscript expr]"; break;
                    case RANGE_EXPR: cout<<"[range expr]"; break;
                    case ZF_EXPR: cout<<"[list comprehension]"; break;
                    case BLESS_EXPR: cout<<"[bless expr]"; break;
                    default:
                        break;
                } break;
            case STMT_NODE:
                switch (expr->type.stmt) {
                    case EXPR_STMT: 
                        cout<<"[expr stmt]";
                        break;
                    case PRINT_STMT:
                        cout<<"[print stmt]";
                        break;
                    case WHILE_STMT:
                        cout<<"[while stmt]";
                        break;
                    case FUNC_DEF_STMT:
                        cout<<"[def stmt]";
                        break;
                    case IF_STMT: 
                        cout<<"[if stmt]";
                        break;
                    case STRUCT_DEF_STMT:
                        cout<<"[struct definition stmt]";
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
        printToken(expr->token);
        for (int i = 0; i < MAX_CHILD; i++)
            preorder(expr->child[i], d+1);
        preorder(expr->next, d);
    }
}

astnode* makeExprNode(ExprType type, Token tk) {
    astnode* node = new astnode(EXPR_NODE, tk);
    node->type.expr = type;
    return node;
}

astnode* makeStmtNode(StmtType type, Token tk) {
    astnode* node = new astnode(STMT_NODE, tk);
    node->type.stmt = type;
    return node;
}

astnode* copyTree(astnode* node) {
    if (node == nullptr)
        return nullptr;
    astnode* t = new astnode(node->nk, node->token);
    t->type = node->type;
    for (int i = 0; i < MAX_CHILD; i++)
        t->child[i] = copyTree(node->child[i]);
    t->next = copyTree(node->next);
    return t;
}

void cleanUpTree(astnode* node) {
    if (node == nullptr)
        return;
    for (int i = 0; i < MAX_CHILD; i++)
        cleanUpTree(node->child[i]);
    cleanUpTree(node->next);
    delete node;
}

bool isExprType(astnode* node, ExprType type) {
    if (node == nullptr)
        return false;
    return node->nk == EXPR_NODE && node->type.expr == type;
}

bool isStmtType(astnode* node, StmtType type) {
    if (node == nullptr)
        return false;
    return node->nk == STMT_NODE && node->type.stmt == type;
}

#endif