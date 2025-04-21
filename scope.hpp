#ifndef scope_hpp
#define scope_hpp
#include <unordered_map>
#include "object.hpp"
#include "allocator.hpp"
using namespace std;

struct Scope {
    unordered_map<string, Object> locals;
    Scope() { }
};


#endif