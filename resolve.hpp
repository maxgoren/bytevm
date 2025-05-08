#ifndef resolve_hpp
#define resolve_hpp
#include "ast.hpp"
#include "stack.hpp"
#include <unordered_map>
#include <iostream>
using namespace std;

class ScopeLevelResolver {
    private:
        bool loud;
        typedef unordered_map<string, bool> ScopeMap;
        IndexedStack<ScopeMap> scopes;
        unordered_map<astnode*, int> depthmap;
        void declareVarName(string id);
        void defineVarName(string id);
        void openScope();
        void closeScope();
        void resolveBlockStatement(astnode* node);
        void resolveLetStatement(astnode* node);
        void resolveDefStatement(astnode* node);
        void resolveVariableDepth(astnode* node, string id);
        void resolveExpressionScope(astnode* node);
        void resolveStatementScope(astnode* node);
        void resolve(astnode* node);
    public:
        ScopeLevelResolver(bool debug = false);
        astnode* resolveScope(astnode* node);
};

ScopeLevelResolver::ScopeLevelResolver(bool debug) {
    loud = debug;
}

astnode* ScopeLevelResolver::resolveScope(astnode* node) {
    scopes.clear(); 
    resolve(node);
    return node;
}

void ScopeLevelResolver::resolve(astnode* node) {
    if (node != nullptr) {
        switch (node->nk) {
            case EXPR_NODE:
                resolveExpressionScope(node);
                break;
            case STMT_NODE:
                resolveStatementScope(node);
                break;
            default:
                break;
        }
        resolve(node->next);
    }
}

void ScopeLevelResolver::resolveExpressionScope(astnode* node) {                                                
    switch (node->type.expr) {
        case ID_EXPR: 
            resolveVariableDepth(node, node->token.strval);
            break;
        case FUNC_EXPR:
            resolveVariableDepth(node, node->token.strval);
            break;
        case ASSIGN_EXPR: {
            resolve(node->child[0]);
            resolve(node->child[1]);
        } break;
        case REF_EXPR: {
            resolve(node->child[0]); 
        } break;
        case LAMBDA_EXPR: {
            openScope();
            for (auto it = node->child[0]; it != nullptr; it = it->next) {
                declareVarName(it->token.strval);
                defineVarName(it->token.strval);
            }
            resolve(node->child[1]);
            closeScope();
            return;
        } break;
        default:
            break;
    }
    for (int i = 0; i < 3; i++)
        if (node->child[i] != nullptr)
            resolve(node->child[i]);
}

void ScopeLevelResolver::resolveStatementScope(astnode* node) {
    switch (node->type.stmt) {
        case BLOCK_STMT: resolveBlockStatement(node); break;
        case FUNC_DEF_STMT: resolveDefStatement(node); break;
        case LET_STMT:  resolveLetStatement(node); break;
        case IF_STMT:
            for (int i = 0; i < 3; i++)
                resolve(node->child[i]);
            break;
        case WHILE_STMT:
            resolve(node->child[0]);
            resolve(node->child[1]);
            break;
        case EXPR_STMT:
            resolve(node->child[0]);
            break;
        case RETURN_STMT: 
            resolve(node->child[0]);
            break;
        case PRINT_STMT:
            resolve(node->child[0]);
            break;
        default:
            break;
    }
}

void ScopeLevelResolver::resolveBlockStatement(astnode* node) {
    openScope();
    resolve(node->child[0]);
    closeScope();
}

void ScopeLevelResolver::resolveLetStatement(astnode* node) {
    astnode* x = node->child[0];
    while (x != nullptr) {
        if (isExprType(x, ID_EXPR))
            break;
        x = x->child[0];
    }
    declareVarName(x->token.strval);
    resolve(node->child[0]);
    defineVarName(x->token.strval);
}

void ScopeLevelResolver::resolveDefStatement(astnode* node) {
    declareVarName(node->token.strval);
    defineVarName(node->token.strval);
    openScope();
    for (auto it = node->child[0]; it != nullptr; it = it->next) {
        if (isExprType(it, REF_EXPR)) {
            declareVarName(it->child[0]->token.strval);
            defineVarName(it->child[0]->token.strval);
        } else {
            declareVarName(it->token.strval);
            defineVarName(it->token.strval);
        }
    }
    resolve(node->child[1]);
    closeScope();
}

void ScopeLevelResolver::resolveVariableDepth(astnode* node, string id) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        if (scopes.get(i).find(id) != scopes.get(i).end()) {
            node->token.depth = scopes.size() - 1 - i;
            cout<<"Resolve: "<<id<<" at nest depth "<<node->token.depth<<endl;
            return;
        }
    }
    node->token.depth = -1;
}

void ScopeLevelResolver::openScope() {
    scopes.push(ScopeMap());
}

void ScopeLevelResolver::closeScope() {
    scopes.pop();
}

void ScopeLevelResolver::declareVarName(string id) {
    if (scopes.empty())
        return;
    if (scopes.top().find(id) != scopes.top().end()) {
        cout<<"That name already exists in this scope."<<endl;
        return;
    }
    cout<<"Declare: "<<id<<" (scope: "<<scopes.size()<<")"<<endl;
    scopes.top()[id] = false;
}

void ScopeLevelResolver::defineVarName(string id) {
    if (scopes.empty())
        return;
    cout<<"Define: "<<id<<" (scope: "<<scopes.size()<<")"<<endl;
    scopes.top()[id] = true;
}

#endif