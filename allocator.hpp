#ifndef allocator_hpp
#define allocator_hpp
#include <iostream>
#include <unordered_set>
#include "stack.hpp"
#include "object.hpp"
#include "scope.hpp"
using namespace std;

class Allocator {
    private:
        unordered_set<GCObject*> liveObjects;
        bool isCollectable(Object& m);
        void markObject(Object& obj);
        void mark(IndexedStack<Scope*>& callStack);
        void sweep();
        void destroyList(List* list);
        void destroyObject(GCObject* obj);
    public:
        Object makeString(string val);
        Object makeList(List* list);
        Object makeFunction(Function* func);
        Object makeStruct(Struct* st);
        void rungc(IndexedStack<Scope*>& callStack);
};

bool Allocator::isCollectable(Object& m) {
    switch (m.type) {
        case AS_LIST:
        case AS_FUNC:
        case AS_STRING:
            return true;
        default:
            break;
    }
    return false;
}

Object Allocator::makeString(string val) {
    Object m;
    m.type = AS_STRING;
    m.data.gcobj = new GCObject(new string(val));   
    m.data.gcobj->marked = false;
    //liveObjects.insert(m.data.gcobj);
    return m;
}

Object Allocator::makeFunction(Function* func) {
    Object m;
    m.type = AS_FUNC;
    m.data.gcobj = new GCObject(func);
    m.data.gcobj->marked = false;
    //liveObjects.insert(m.data.gcobj);
    return m;
}

Object Allocator::makeStruct(Struct* st) {
    Object m; 
    m.type = AS_STRUCT;
    m.data.gcobj = new GCObject(st);
    m.data.gcobj->marked = false;
    //liveObjects.insert(m.data.gcobj);
    return m;
}

Object Allocator::makeList(List* list) {
    Object m;
    m.type = AS_LIST;
    m.data.gcobj = new GCObject(list);
    m.data.gcobj->marked = false;
    //liveObjects.insert(m.data.gcobj);
    return m;
}

void Allocator::rungc(IndexedStack<Scope*>& callStack) {
    //mark(callStack);
    //sweep();
}

void Allocator::markObject(Object& object) {
    object.data.gcobj->marked = true;
    if (object.type == AS_LIST) {
        for (ListNode* ln = object.data.gcobj->listval->head; ln != nullptr; ln = ln->next) {
            if (isCollectable(ln->info))
                markObject(ln->info);
        }
    }
}

void Allocator::mark(IndexedStack<Scope*>& callStack) {
    for (int i = callStack.size() - 1; i >= 0; i--) {
        for (auto & m : callStack.get(i)->bindings) {
            if (isCollectable(m.second)) {
                markObject(m.second);
            }
        }
    }
}

void Allocator::destroyList(List* list) {
    if (list == nullptr) return;
    while (list->head != nullptr) {
        ListNode* x = list->head;
        list->head = list->head->next;
        delete x;
    }
    delete list;
}

void Allocator::destroyObject(GCObject* x) {
    switch (x->type) {
        case GC_FUNC:   {  } break;
        case GC_LIST:   { destroyList(x->listval); } break;
        case GC_STRING: { delete x->strval; } break;
    }
    delete x;
}

void Allocator::sweep() {
    unordered_set<GCObject*> next;
    unordered_set<GCObject*> kill;
    for (auto & m : liveObjects) {
        if (m->marked == false) {
            auto x = m;
            cout<<"Collecting: "<<x<<" - ";
            switch (x->type) {
                case GC_FUNC:   cout<<x->funcval->name<<endl; break;
                case GC_LIST:   cout<<"(list)"<<endl; break;
                case GC_STRING: cout<<*x->strval<<endl; break;
            }
            kill.insert(x);
        } else {
            m->marked = false;
            next.insert(m);
        }
    }
    auto old = liveObjects;
    liveObjects = next;
    cout<<kill.size()<<" objects have become unreachable with "<<next.size()<<" remaining in scope."<<endl;
    for (auto & m : kill) {
        destroyObject(m);
    }
}

#endif
