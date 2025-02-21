#ifndef object_hpp
#define object_hpp
#include <iostream>
#include <cmath>
using namespace std;

struct Function {
    string name;
    int args;
    int locals;
    int addr;
    Function(string n, int ag, int l, int adr) : name(n), args(ag), locals(l), addr(adr) { }
};

enum StoreAs {
    AS_INT, AS_REAL, AS_BOOL, AS_CHAR, AS_STRING, AS_FUNC, AS_CLOSURE, AS_LIST, AS_MAP, AS_NULL
};

struct List;
struct Closure;

struct Object {
    StoreAs type;
    union {
        int intval;
        double realval;
        bool boolval;
        char charval;
        string* strval;
        Function* funcval;
        List* listval;
        Closure* closureval;
    } data;
    Object(List* val) { type = AS_LIST; data.listval = val; }
    Object(Function* val) { type = AS_FUNC; data.funcval = val; }
    Object(string* val) { type = AS_STRING; data.strval = val; }
    Object(Closure* val) { type = AS_CLOSURE; data.closureval = val; }
    Object(char val) { type = AS_CHAR; data.charval = val; }
    Object(double val) { type = AS_REAL; data.realval = val; }
    Object(bool val) { type = AS_BOOL; data.boolval = val; }
    Object(int val) { type = AS_INT; data.intval = val; }
    Object() { type = AS_NULL; data.intval = 0; }
    Object(const Object& obj) {
        type = obj.type;
        switch (type) {
            case AS_LIST: data.listval = obj.data.listval; break;
            case AS_STRING: data.strval = obj.data.strval; break;
            case AS_FUNC: data.funcval = obj.data.funcval; break;
            case AS_CLOSURE: data.closureval = obj.data.closureval; break;
            case AS_CHAR: data.charval = obj.data.charval; break;
            case AS_BOOL: data.boolval = obj.data.boolval; break;
            case AS_INT: data.intval = obj.data.intval; break;
            case AS_REAL: data.realval = obj.data.realval; break;
            default:
                break;
        }
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
        case AS_STRING: str = *(obj.data.strval); break;
        case AS_FUNC:   str = obj.data.funcval->name; break;
        case AS_NULL:   str = "(null)"; break;
        case AS_LIST: {
            str = "[ ";
            for (ListNode* it = obj.data.listval->head; it != nullptr; it = it->next) {
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

Object makeBool(bool val) {
    return Object(val);
}

Object makeChar(char val) {
    return Object(val);
}

Object makeString(string val) {
    return Object(new string(val));
}

Object makeNil() {
    return Object();
}

Object makeList(List* list) {
    return Object(list);
}

StoreAs typeOf(Object obj) {
    return obj.type;
}

Object makeNumber(double val) {
    return makeReal(val);
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

Object add(Object lhs, Object rhs) {
    if (typeOf(lhs) == AS_STRING || typeOf(rhs) == AS_STRING) {
        return makeString(toString(lhs)+toString(rhs));
    }
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