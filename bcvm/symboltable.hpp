#ifndef symboltable_hpp
#define symboltable_hpp
#include <iostream>
#include "../object.hpp"
using namespace std;

const int MAX_LOCALS = 255;

struct Scope;

struct FuncVar {
    string name;
    Scope* scope;
    Function* func;
    FuncVar(string n, Scope* sc) : name(n), scope(sc) { } 
};

struct StructVar {
    string name;
    Scope* scope;
    StructVar(string n, Scope* sc) : name(n), scope(sc) { }
};


enum VarType {
    GLOBALVAR, LOCALVAR, FUNCVAR, STRUCTVAR, EMPTY
};

struct STEntry {
    string name;
    int depth;
    int location;
    VarType type;
    union {
        FuncVar* funcVar;
        StructVar* structVar;
    };
    STEntry() : type(EMPTY), depth(0) { name = "nil"; }
};

struct Scope {
    STEntry locals[MAX_LOCALS];
    Scope* enclosing;
    int num_locals;
    Scope(Scope* parent = nullptr) {
        num_locals = 0;
        enclosing = parent;
    }
    void insert(string name, STEntry entry) {
        entry.location = num_locals++;
        entry.name = name;
        if (entry.type == FUNCVAR)
            entry.funcVar->func->addr = entry.location;
        locals[entry.location] = entry;
    }
    STEntry& get(int i) {
        return locals[i];
    }
    int find(string name) {
        for (int i = num_locals - 1; i >= 0; i--) {
            if (locals[i].name == name)
                return i;
        }
        return -1;
    }
};

class SymbolTable {
    private:
        Scope* scope;
        Scope* global;
    public:
        SymbolTable() {
            global = new Scope();
            scope = global;
        }
        int numLocals() {
            return scope->num_locals;
        }
        void insert(string name, STEntry entry) {
            scope->insert(name,entry);
        }
        STEntry get(string name) {
            Scope* x = scope;
            int depth = 0;
            cout<<"Searching For: "<<name;
            while (x != global) {
                cout<<" . ";
                int loc = x->find(name);
                if (loc != -1) {
                    cout<<"Found at depth: "<<depth<<"!"<<endl;
                    return x->get(loc);
                }
                depth++;
                x = x->enclosing;
            }
            if (global->find(name) != -1) {
                cout<<" Found in global scope"<<endl;
                return global->get(global->find(name));
            }
            cout<<"Not found."<<endl;
            return STEntry();
        }
        void openFunctionScope(string name) {
            STEntry ent = get(name);
            switch (ent.type) {
                case EMPTY: {
                ent.type = FUNCVAR;
                ent.funcVar = new FuncVar(name, new Scope(scope));
                cout<<"Created new scope for function "<<name<<endl;
                ent.funcVar->scope->enclosing = scope;
                Function* f = new Function(name, 0, 0, 0);
                ent.funcVar->func = f;                
                scope->insert(name, ent);
                scope = ent.funcVar->scope;
                } break;
                case FUNCVAR: {
                    cout<<"Entered local scope for function: "<<name<<endl;
                    scope = ent.funcVar->scope;
                } break;
                default:
                    break;
            }
        }
        void closeScope() {
            if (scope->enclosing != nullptr) {
                cout<<"Leaving local scope."<<endl;
                scope = scope->enclosing;
            }
        }
        void print(Scope* sc, int d) {
            if (sc != nullptr) {
                for (int i = 0; i < sc->num_locals; i++) {
                    for (int i = 0; i < d; i++) cout<<"  ";
                    cout<<i<<": "<<sc->locals[i].name<<endl;
                    if (sc->locals[i].type == FUNCVAR)
                        print(sc->locals[i].funcVar->scope, d + 1);   
                }
            }
        }
        void show() {
            print(scope, 1);
        }
};

#endif