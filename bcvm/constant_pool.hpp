#ifndef constant_pool_hpp
#define constant_pool_hpp
#include <iostream>
#include <vector>
#include "../allocator.hpp"
#include "../object.hpp"
#include "bytecode.hpp"
using namespace std;

const int LOCALS_MAX = 255;
const int CALLSTACK_MAX = 255;
const int MAX_CODE_SIZE = 1000;

class ConstantPool {
    private:
        int MAX_CONST;
        Object data[3500];
        int  n;
        int freeList[3500];
        int nf;
    public:
        ConstantPool() {
            n = 0;
            nf = 0;
            MAX_CONST = 3500;
        }
        int alloc(Object obj) {
            int addr = 0;
            if (nf > 0) {
                addr = freeList[--nf];
            } else if (n < MAX_CONST) {
                addr = n++;
            }
            data[addr] = obj;
            return addr;
        }
        void free(int addr) {
            freeList[nf++] = addr;
        }
        void updateAt(int addr, Object object) {
            data[addr] = object;
        }
        Object& get(int addr) {
            return data[addr];
        }
};

#endif