#ifndef bytecode_hpp
#define bytecode_hpp
#include <iostream>
#include "object.hpp"
using namespace std;

enum VMInstr {
    vm_iadd, vm_isub, vm_imul, vm_idiv, vm_imod,
    vm_fadd, vm_fsub, vm_fmul, vm_fdiv,
    vm_itof, vm_neg, vm_not, vm_fneg, vm_fnot,
    vm_iequ, vm_ineq, vm_ilt, vm_igt, vm_ilte, vm_igte,
    vm_fequ, vm_fneq, vm_flt, vm_fgt, vm_flte, vm_fgte,
    vm_iconst, vm_cconst, vm_sconst, vm_fconst, 
    vm_sconcat, vm_mklist, vm_apndlist, vm_listsize,
    vm_def, vm_call, vm_ret, vm_closure,
    vm_br, vm_brt, vm_brf, 
    vm_gload, vm_glda, vm_gstore, 
    vm_load, vm_lda, vm_store, 
    vm_fload, vm_flda, vm_fstore,
    vm_struct, vm_null, 
    vm_print, vm_pop, 
    vm_halt,
    vm_label, vm_skip
};

string VMInstrStr[] = {
    "iadd", "isub", "imul", "idiv", "imod",
    "fadd", "fsub", "fmul", "fdiv",
    "itof", "neg", "not", "fneg", "fnot",
    "iequ", "ineq", "ilt", "igt", "ilte", "igte",
    "fequ", "fneq", "flt", "fgt", "flte", "fgte",
    "iconst", "cconst", "sconst", "fconst", 
    "sconcat", "mklist", "apndlist", "listsize",
    ".def","call", "ret", "closure",
    "br", "brt", "brf",
    "gload", "glda", "gstore", 
    "load", "lda", "store", 
    "fload", "flda", "fstore",
    "struct", "null", 
    "print", "pop",
    "halt",
    "label","skip"

};


struct ByteCodeInstruction {
    VMInstr instr;
    Object operand;
    ByteCodeInstruction(VMInstr in = vm_halt, Object obj = makeNil()) : instr(in), operand(obj) { }
};

#endif