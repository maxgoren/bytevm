#ifndef allocator_hpp
#define allocator_hpp
#include <iostream>
#include <unordered_set>
#include <set>
#include "stack.hpp"
#include "object.hpp"
#include "scope.hpp"
using namespace std;

class Allocator {
    private:
        int NEXT_GC_LIMIT;
        set<GCObject*> liveObjects;
        bool isCollectable(Object& m);
        void markObject(Object& obj);
        void mark(ActivationRecord* callStack, IndexedStack<Object>& rtStack);
        void sweep();
        void destroyList(List* list);
        void destroyStruct(Struct* obj);
        void destroyObject(GCObject* obj);
        void registerObject(GCObject* obj);
    public:
        Allocator();
        Object makeString(string val);
        Object makeList(List* list);
        Object makeFunction(Function* func);
        Object makeStruct(Struct* st);
        void rungc(ActivationRecord* callStack, IndexedStack<Object>& rtStack);
        int liveCount();
        int nextGC();
};

Allocator::Allocator() {
    NEXT_GC_LIMIT = 150;
}

int Allocator::nextGC() {
    return NEXT_GC_LIMIT;
}
int Allocator::liveCount() {
    return liveObjects.size();
}

bool Allocator::isCollectable(Object& m) {
    switch (m.type) {
        case AS_FUNC:
        case AS_LIST:
        case AS_STRING:
        case AS_STRUCT:
            return m.data.gcobj != nullptr;
        default:
            break;
    }
    return false;
}

void Allocator::registerObject(GCObject* object) {
    object->marked = false;
    liveObjects.insert(object);
}

Object Allocator::makeString(string val) {
    Object m;
    m.type = AS_STRING;
    /*
        To really make this feasable would need: a bi-directional map to quickly query string to address by value,
        and we would probably want to have separate live lists for string/list/struct instead of one for GCObject
    /*
        for (auto x : liveObjects) {
            if (x->type == GC_STRING && *x->strval == val) {
                cout<<val<<" can be reused!"<<endl;
                m.data.gcobj = x;
                m.data.gcobj->marked = false;
                return m;
            }
        }
  /*  */
    m.data.gcobj = new GCObject(new string(val));   
    m.data.gcobj->marked = false;
    registerObject(m.data.gcobj);
    return m;
}

Object Allocator::makeFunction(Function* func) {
    Object m;
    m.type = AS_FUNC;
    m.data.gcobj = new GCObject(func);
    registerObject(m.data.gcobj);
    return m;
}

Object Allocator::makeStruct(Struct* st) {
    Object m; 
    m.type = AS_STRUCT;
    m.data.gcobj = new GCObject(st);
    registerObject(m.data.gcobj);
    return m;
}

Object Allocator::makeList(List* list) {
    Object m;
    m.type = AS_LIST;
    m.data.gcobj = new GCObject(list);
    registerObject(m.data.gcobj);
    return m;
}

void Allocator::rungc(ActivationRecord* callStack, IndexedStack<Object>& rtStack) {
    //cout<<"[GC Starting.]"<<endl;
    mark(callStack, rtStack);
    sweep();
    NEXT_GC_LIMIT = 1.5*NEXT_GC_LIMIT;
}

void Allocator::markObject(Object& object) {
    object.data.gcobj->marked = true;
    if (object.type == AS_LIST) {
        for (ListNode* ln = getList(object)->head; ln != nullptr; ln = ln->next) {
            if (isCollectable(ln->info))
                markObject(ln->info);
        }
    } else if (object.type == AS_STRUCT && getStruct(object)->blessed) {
        for (auto & m : getStruct(object)->fields) {
            if (isCollectable(m.second))
                markObject(m.second);
        }
    }
}

void Allocator::mark(ActivationRecord* callStack, IndexedStack<Object>& rtStack) {
    for (int i = 0; i < rtStack.size(); i++) {
        if (isCollectable(rtStack.get(i)))
            markObject(rtStack.get(i));
    }
    for (ActivationRecord* z = callStack; z != nullptr; z = z->controlLink) {
        for (auto & m : z->bindings) {
            if (isCollectable(m.second) && m.second.data.gcobj->marked == false) {
                markObject(m.second);
            }
            ActivationRecord* x = z->accessLink;
            while (x != nullptr) {
                for (auto m : x->bindings) {
                    if (isCollectable(m.second) && m.second.data.gcobj->marked == false)
                        markObject(m.second);
                }
                x = x->accessLink;
            }
        }
    }
}

void Allocator::destroyList(List* list) {
    if (list == nullptr) return;
    while (list->head != nullptr) {
        ListNode* x = list->head;
        list->head = list->head->next;
        if (isCollectable(x->info))
            destroyObject(x->info.data.gcobj);
        delete x;
    }
    delete list;
}

void Allocator::destroyStruct(Struct* sobj) {
    if (sobj == nullptr) return;
    for (auto & m : sobj->fields) {
        if (isCollectable(m.second))
            destroyObject(m.second.data.gcobj);
    }
    delete sobj;
}

void Allocator::destroyObject(GCObject* x) {
    switch (x->type) {
        //case GC_FUNC:   { delete x->funcval; delete x; } break;
        case GC_STRING: { delete x->strval;  delete x; } break;
        case GC_LIST:   { destroyList(x->listval); delete x; } break;
        case GC_STRUCT: { destroyStruct(x->structval); delete x; } break;
    }
}

void Allocator::sweep() {
    set<GCObject*> next;
    set<GCObject*> kill;
    for (auto & m : liveObjects) {
        if (m->marked == false) {
            auto x = m;
            kill.insert(x);
        } else {
            m->marked = false;
            next.insert(m);
        }
    }
    auto old = liveObjects;
    liveObjects = next;
    //cout<<kill.size()<<" objects have become unreachable with "<<next.size()<<" remaining in scope."<<endl;
    for (auto & x : kill) {
        destroyObject(x);
    }
    //cout<<"[GC Done.]"<<endl;
}

#endif
