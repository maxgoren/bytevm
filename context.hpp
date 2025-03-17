#ifndef context_hpp
#define context_hpp
#include <iostream>
#include "allocator.hpp"
#include "scope.hpp"
#include "stack.hpp"
using namespace std;

class Context {
    private:
        unordered_map<string, Object> globals;
        IndexedStack<Scope> callStack;
        Object nilObject;
        Allocator alloc;
    public:
        Context() {

        }
        IndexedStack<Scope>& getStack() {
            return callStack;
        }
        void openScope() {
            callStack.push(Scope());
        }
        void openScope(Scope& scope) {
            callStack.push(scope);
        }
        void closeScope() {
            if (!callStack.empty()) {
                callStack.pop();
                alloc.rungc(callStack, globals);
            }
        }
        Object& get(string name, int depth) {
            if (depth > -1) {
                return callStack.get(callStack.size()-1-depth).locals[name];
            }            
            return globals[name];
        }
        void put(string name, int depth, Object info) {
            if (depth > -1) {
                callStack.get(callStack.size()-1-depth).locals[name] = info;
            } else {
                globals[name] = info;
            }
        }
        void insert(string name, Object info) {
            if (!callStack.empty()) {
                callStack.top().locals[name] = info;
            } else {
                globals[name] = info;
            }
        }
        bool existsInScope(string name) {
            if (!callStack.empty()) {
                return callStack.top().locals.find(name) != callStack.top().locals.end();
            } else {
                return globals.find(name) != globals.end();
            }
        }
        Allocator& getAlloc() {
            return alloc;
        }
};

#endif