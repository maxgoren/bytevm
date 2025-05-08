#ifndef context_hpp
#define context_hpp
#include <iostream>
#include "allocator.hpp"
#include "scope.hpp"
#include "stack.hpp"
using namespace std;

const int GLOBAL_SCOPE_DEPTH = -1;

class Context {
    private:
        unordered_map<string, Struct*> objects;
        Scope* globals;
        IndexedStack<Scope*> callStack;
        Object nilObject;
        Allocator alloc;
    public:
        Context() {
            globals = new Scope(nullptr);
            nilObject = makeNil();
            callStack.push(globals);
        }
        IndexedStack<Scope*>& getStack() {
            return callStack;
        }
        void addStructType(Struct* st) {
            objects[st->typeName] = st;
        }
        Struct* getInstanceType(string name) {
            return objects[name];
        }
        void openScope() {
            callStack.push(new Scope(callStack.top()));
        }
        void openScope(Scope* scope) {
            callStack.push(scope);
        }
        void closeScope() {
            if (callStack.size() > 1) {
                callStack.pop();
                alloc.rungc(callStack);
            }
        }
        Object& get(string name, int depth) {
            if (depth > GLOBAL_SCOPE_DEPTH) {
                Scope* curr = callStack.get(callStack.size()-1-depth);
                if (curr != nullptr) {
                    if (curr->bindings.find(name) != curr->bindings.end()) {
                        return curr->bindings[name];
                    } else if (curr->enclosing != nullptr && curr->enclosing->bindings.find(name) != curr->enclosing->bindings.end()) {
                        cout<<"Went with enclosing on get"<<endl;
                        return curr->enclosing->bindings[name];
                    }
                }
                return nilObject;
            }          
            return globals->bindings[name];
        }
        void put(string name, int depth, Object info) {
            if (depth > GLOBAL_SCOPE_DEPTH) {
                Scope* curr = callStack.get(callStack.size()-1-depth);
                if (curr->bindings.find(name) != curr->bindings.end()) {
                    curr->bindings[name] = info;
                } else if (curr->enclosing != nullptr && curr->enclosing->bindings.find(name) != curr->enclosing->bindings.end()) {
                    cout<<"Went with enclosing on put."<<endl;
                    curr->enclosing->bindings[name] = info;
                } else {
                    cout<<"Adding new binding: "<<name<<" - "<<toString(info)<<endl;
                    curr->bindings.emplace(name, info);
                }
            } else {
                globals->bindings[name] = info;
            }
        }
        void insert(string name, Object info) {
            //callStack.top()->bindings.emplace(name,info);           
            callStack.top()->bindings[name] = info;
        }
        bool existsInScope(string name) {
            return callStack.top()->bindings.find(name) != callStack.top()->bindings.end();
        }
        Allocator& getAlloc() {
            return alloc;
        }
        Object& nil() {
            return nilObject;
        }
};

#endif