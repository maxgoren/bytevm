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
        ActivationRecord* globals;
        ActivationRecord* current;
        Object nilObject;
        Allocator alloc;
        IndexedStack<Object> operands;
        ActivationRecord* enclosingAt(int distance) {
            ActivationRecord* curr = current;
            while (distance > 0 && curr != nullptr) {
                curr = curr->accessLink;
                distance--;
            }
            if (distance > 0 || curr == nullptr) {
                cout<<"Hmm... Something wrong with scope distance."<<endl;
            }
            return curr;
        }
    public:
        Context() {
            globals = new ActivationRecord(nullptr);
            current = globals;
            nilObject = makeNil();
        }
        ActivationRecord*& getCallStack() {
            return current;
        }
        IndexedStack<Object>& getOperandStack() {
            return operands;
        }
        void addStructType(Struct* st) {
            objects[st->typeName] = st;
        }
        Struct* getInstanceType(string name) {
            return objects[name];
        }
        void openScope() {
            ActivationRecord* sf = new ActivationRecord(current, current);
            current = sf;
        }
        void openScope(ActivationRecord* scope) {
            current = scope;
        }
        void closeScope() {
            if (current != globals) {
                current = current->controlLink;
            }
            if (alloc.liveCount() > alloc.nextGC()) {
                alloc.rungc(current, operands);
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
            if (alloc.liveCount() > alloc.nextGC()) {
                alloc.rungc(current, operands);
            }
        }
        void insert(string name, Object info) {
            current->bindings[name] = info;
        }
        bool existsInScope(string name) {
            return current->bindings.find(name) != current->bindings.end();
        }
        Allocator& getAlloc() {
            return alloc;
        }
        Object& nil() {
            return nilObject;
        }
};

#endif