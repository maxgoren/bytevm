#ifndef object_hpp
#define object_hpp
#include <iostream>
#include <cmath>
#include <unordered_map>
#include "ast.hpp"
using namespace std;


enum StoreAs {
    AS_INT, AS_REAL, AS_BOOL, AS_CHAR, AS_STRING, AS_FUNC, AS_CLOSURE, AS_LIST, AS_MAP, AS_NULL
};

struct List;
struct Closure;
struct Function;

struct GCObject;

struct Object {
    StoreAs type;
    bool marked;
    union {
        int intval;
        double realval;
        bool boolval;
        char charval;
        GCObject* gcobj;
    } data;
    Object(char val) { type = AS_CHAR; data.charval = val; marked = false; }
    Object(double val) { type = AS_REAL; data.realval = val; marked = false; }
    Object(bool val) { type = AS_BOOL; data.boolval = val; marked = false; }
    Object(int val) { type = AS_INT; data.intval = val; marked = false; }
    Object() { type = AS_NULL; data.intval = 0; marked = false; }
    Object(const Object& obj) {
        type = obj.type;
        switch (type) {
            case AS_LIST: data.gcobj = obj.data.gcobj; break;
            case AS_STRING: data.gcobj = obj.data.gcobj; break;
            case AS_FUNC: data.gcobj = obj.data.gcobj; break;
            case AS_CLOSURE: data.gcobj = obj.data.gcobj; break;
            case AS_CHAR: data.charval = obj.data.charval; break;
            case AS_BOOL: data.boolval = obj.data.boolval; break;
            case AS_INT: data.intval = obj.data.intval; break;
            case AS_REAL: data.realval = obj.data.realval; break;
            default:
                break;
        }
    }
};

struct Function {
    string name;
    int args;
    int locals;
    int addr;
    astnode* body;
    astnode* params;
    unordered_map<string, Object> freeVars;
    Function(string n, int ag, int l, int adr) : name(n), args(ag), locals(l), addr(adr) { }
    Function(astnode* par, astnode* code) : params(par), body(code) { }
    Function() {
        args = 0; locals = 0; addr = 0;
        name = "nil";
    }
};

struct ListNode {
    Object info;
    ListNode* next;
    ListNode(Object obj = Object(), ListNode* n = nullptr) : info(obj), next(n) { }
};


struct List {
    ListNode* head;
    ListNode* tail;
    int count;
    List() : head(nullptr), tail(nullptr) {
        count = 0;
    }
};

enum GC_TYPE {
    GC_LIST, GC_STRING, GC_FUNC
};

struct GCObject {
    GC_TYPE type;
    bool marked;
    union {
        string* strval;
        Function* funcval;
        List* listval;
        Closure* closureval;
    };
    GCObject(string* s) : strval(s), marked(false), type(GC_STRING) { }
    GCObject(string s) : strval(new string(s)), marked(false), type(GC_STRING) { }
    GCObject(List* l) : listval(l), marked(false), type(GC_LIST) { }
    GCObject(Function* f) : funcval(f), marked(false), type(GC_FUNC) { }
    GCObject(Closure* c) : closureval(c), marked(false), type(GC_FUNC) { }
};

void printGCObject(GCObject* x) {
    switch (x->type) {
        case GC_FUNC:   cout<<x->funcval->name<<endl; break;
        case GC_LIST:   cout<<"(list)"<<endl; break;
        case GC_STRING: cout<<*x->strval<<endl; break;
    }
}

List* appendList(List* list, Object obj) {
    ListNode* t = new ListNode(obj, nullptr);
    if (list->head == nullptr) {
        list->head = t;
    } else {
        list->tail->next = t;
    }
    list->tail = t;
    list->count += 1;
    return list;
}

bool listEmpty(List* list) {
    return list->count == 0;
}

int listSize(List* list) {
    return list == nullptr ? -1:list->count;
}

List* pushList(List* list, Object obj) {
    ListNode* t = new ListNode(obj, list->head);
    if (listEmpty(list)) {
        list->tail = t;
    }
    list->head = t;
    list->count += 1;
    return list;
}

List* updateListAt(List* list, int index, Object obj) {
    int i = 0;
    ListNode* it = list->head; 
    while (it != nullptr && i < index) {
        it = it->next;
        i++;
    }
    it->info = obj;
    return list;
}

ListNode* getListItemAt(List* list, int index) {
    int i = 0;
    ListNode* it = list->head; 
    while (it != nullptr && i < index) {
        it = it->next;
        i++;
    }
    return it;
}


string toString(Object obj) {
    string str;
    switch (obj.type) {
        case AS_INT:    str = to_string(obj.data.intval); break;
        case AS_REAL:   str = to_string(obj.data.realval); break;
        case AS_BOOL:   str = obj.data.boolval ? "true":"false"; break;
        case AS_STRING: str = *(obj.data.gcobj->strval); break;
        case AS_FUNC:   str = obj.data.gcobj->funcval->name; break;
        case AS_NULL:   str = "(null)"; break;
        case AS_LIST: {
            str = "[ ";
            for (ListNode* it = obj.data.gcobj->listval->head; it != nullptr; it = it->next) {
                str += toString(it->info);
                if (it->next != nullptr) 
                    str += ", ";
            }
            str += " ]";
        } break;
        default: break;
    }
    return str;
}

ostream& operator<<(ostream& os, const Object& obj) {
    os<<toString(obj);
    return os;
}


StoreAs typeOf(Object obj) {
    return obj.type;
}

bool compareOrdinal(Object obj) {
    switch (obj.type) {
        case AS_REAL: 
        case AS_BOOL: 
        case AS_INT:  return true;
        default:
            break;
    }
    return false;
}

double getPrimitive(Object obj) {
    double a = 0;
    switch (obj.type) {
        case AS_REAL: { a = obj.data.realval;  }break;
        case AS_BOOL: { a = obj.data.boolval; } break;
        case AS_INT:  { a = obj.data.intval; } break;
        default:
            break;
    }
    return a;
}

Object makeInt(int val) {
    Object m;
    m.type = AS_INT;
    m.data.intval = val;
    return m;
}

Object makeReal(double val) {
    Object m;
    if (std::floor(val) == val) {
        m.type = AS_INT;
        m.data.intval = (int)val;
        return m;
    }
    m.type = AS_REAL;
    m.data.realval = val;
    return m;
}

Object makeNumber(double val) {
    return makeReal(val);
}

Object makeBool(bool val) {
    return Object(val);
}

Object makeNil() {
    return Object();
}

Object add(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    return makeReal(lhn+rhn);
}

Object sub(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    return makeReal(lhn-rhn);
}

Object div(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    if (rhn == 0.0) {
        cout<<"Error: divide by 0"<<endl;
        return makeReal(0);
    }
    return makeReal(lhn/rhn);
}

Object mod(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    return makeInt((int)lhn % (int)rhn);
}

Object mul(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    return makeNumber(lhn*rhn);
} 

Object pow(Object lhs, Object rhs) {
    double lhn = getPrimitive(lhs);
    double rhn = getPrimitive(rhs);
    return makeNumber(pow(lhn, rhn));
}

Object neg(Object lhs) {
    double val = getPrimitive(lhs);
    return makeNumber(-val);
}

Object lt(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn < rhn);
    }
    return makeBool(toString(lhs) < toString(rhs));
}

Object lte(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn <= rhn);
    }
    return makeBool(toString(lhs) <= toString(rhs));
}

Object gt(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn > rhn);
    }
    return makeBool(toString(lhs) > toString(rhs));
}

Object gte(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn >= rhn);
    }
    return makeBool(toString(lhs) >= toString(rhs));
}

Object equ(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn == rhn);
    }
    return makeBool(toString(lhs) == toString(rhs));
}

Object neq(Object lhs, Object rhs) {
    if (compareOrdinal(lhs) && compareOrdinal(rhs)) {
        double lhn = getPrimitive(lhs);
        double rhn = getPrimitive(rhs);
        return makeBool(lhn != rhn);
    }
    return makeBool(toString(lhs) != toString(rhs));
}

Object logicAnd(Object lhs, Object rhs) {
    return makeBool(getPrimitive(lhs) && getPrimitive(rhs));
}

Object logicOr(Object lhs, Object rhs) {
    return makeBool(getPrimitive(lhs) || getPrimitive(rhs));
}

#endif