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
        Scope* enclosingAt(int distance) {
            Scope* curr = callStack.top();
            while (distance > 0 && curr != nullptr) {
                curr = curr->enclosing;
                distance--;
            }
            return curr;
        }
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
           if (depth == GLOBAL_SCOPE_DEPTH) {
                return globals->bindings[name];
           }
           return enclosingAt(depth)->bindings[name];
        }
        void put(string name, int depth, Object info) {
            if (depth == GLOBAL_SCOPE_DEPTH) {
                globals->bindings[name] = info;
            } else {
                enclosingAt(depth)->bindings[name] = info;
            }
        }
        void insert(string name, Object info) {
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