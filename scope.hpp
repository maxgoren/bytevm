#ifndef scope_hpp
#define scope_hpp
#include <unordered_map>
#include "object.hpp"
#include "allocator.hpp"
using namespace std;

typedef unordered_map<string, Object> Environment;

struct ActivationRecord {
    Environment bindings;
    ActivationRecord* controlLink;
    ActivationRecord* accessLink;
    ActivationRecord(ActivationRecord* defining = nullptr, ActivationRecord* calling = nullptr) : accessLink(defining), controlLink(calling) { }
};


#endif