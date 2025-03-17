#ifndef bytecode_hpp
#define bytecode_hpp
#include <iostream>
#include "../object.hpp"
using namespace std;

enum VMInstr {
    vm_label,
    vm_add, vm_sub, vm_mul, vm_div, vm_mod,
    vm_neg, vm_not,
    vm_equ, vm_neq, vm_lt, vm_gt, vm_lte, vm_gte,
    vm_const, vm_mklist, vm_apndlist, vm_listsize,
    vm_def, vm_call, vm_ret, vm_closure,
    vm_open_scope, vm_close_scope,
    vm_br, vm_brf, 
    vm_gload, vm_glda, vm_gstore, 
    vm_load, vm_lda, vm_store, 
    vm_fload, vm_flda, vm_fstore,
    vm_struct, vm_null, 
    vm_print, vm_println, vm_pop, 
    vm_halt
};

string VMInstrStr[] = {
    "label",
    "add", "sub", "mul", "div", "mod",
    "neg", "not", 
    "equ", "neq", "lt", "gt", "lte", "gte",
    "const", "mklist", "apndlist", "listsize",
    ".def","call", "ret", "closure",
    "open_scope", "close_scope",
    "br","brf",
    "gload", "glda", "gstore", 
    "load", "lda", "store", 
    "fload", "flda", "fstore",
    "struct", "null", 
    "print", "println", "pop",
    "halt"
};


struct ByteCodeInstruction {
    VMInstr instr;
    Object operand;
    ByteCodeInstruction(VMInstr in = vm_halt, Object obj = makeNil()) : instr(in), operand(obj) { }
};

void printInstruction(ByteCodeInstruction& bci) {
    cout<<"["<<VMInstrStr[bci.instr]<<", "<<bci.operand<<" ]"<<endl;
}

#endif