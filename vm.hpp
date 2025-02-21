#ifndef vm_hpp
#define vm_hpp
#include <iostream>
#include <vector>
#include "object.hpp"
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

struct StackFrame {
    Function* sym;
    int retAddr;
    Object locals[LOCALS_MAX];
    StackFrame(Function* f = nullptr, int ra = 0) : sym(f), retAddr(ra) { }
    StackFrame(const StackFrame& sf) {
        sym = sf.sym;
        retAddr = sf.retAddr;
        for (int i = 0; i < LOCALS_MAX; i++)
            locals[i] = sf.locals[i];
    }
};

class VM {
    private:
        bool loud;
        bool running;
        Object nilObject;
        StackFrame sf[CALLSTACK_MAX];
        int fp; // frame pointer into stackframe stack
        Object operands[CALLSTACK_MAX];
        int sp; // stack pointer into operands stack
        Function* mainFunction;
        vector<ByteCodeInstruction> codePage;
        int ip;
        ConstantPool constPool;
        void displayInternalState() {
            if (!loud)
                return;
            if (sp > 0) {
                cout<<"Operand Stack: ";
                for (int i = 0; i < sp; i++) {
                    cout<<"["<<i<<": "<<operands[i]<<" ]";
                }
                cout<<endl;
            }
            cout<<"----"<<endl;
            if (fp > 0) {
                cout<<"Call Stack Frames: ";
                for (int j = 0; j < fp; j++) {
                    cout<<"{"<<j<<": "<<sf[j].sym->name<<" ";
                    for (int i = 0; i < sf[j].sym->args; i++) {
                        cout<<"["<<i<<": "<<sf[j].locals[i]<<"] ";
                    }
                    cout<<"} ";
                }
                cout<<endl;
            }
            cout<<"----------------------------------------"<<endl;
        }
        ByteCodeInstruction& fetchInstruction() {
            return codePage[ip++];
        }
        void executeBinaryOperation(ByteCodeInstruction& current) {
            switch (current.instr) {
                case vm_add: {
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = add(l, r);
                } break;
                case vm_sub: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = sub(l, r);
                } break;
                case vm_mul: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = mul(l, r);
                } break;
                case vm_div: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = div(l, r);
                } break;
                case vm_mod: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = mod(l, r);
                } break;
                case vm_lt: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = lt(l, r);
                } break;
                case vm_gt: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = gt(l, r);
                } break;
                case vm_lte: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = lte(l, r);
                } break;
                case vm_gte: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = gte(l, r);
                } break;
                case vm_equ: { 
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = equ(l, r);
                } break;
                case vm_neq: {
                    Object r = operands[--sp];
                    Object l = operands[--sp];
                    operands[sp++] = neq(l, r);
                } break;
                default:
                    break;
            }
        }
        void executeInstruction(ByteCodeInstruction& current) {
            switch (current.instr) {
                case vm_halt:   { running = false; } break;
                case vm_const:  { operands[sp++] = current.operand; } break;
                case vm_not:    { operands[sp-1].data.boolval = !(operands[sp-1].data.boolval); } break;
                case vm_neg:    { operands[sp-1].data.intval = -operands[sp-1].data.intval;} break;
                case vm_null:   { operands[sp++] = nilObject; } break;
                case vm_gload:  { operands[sp++] = constPool.get(current.operand.data.intval); } break;
                case vm_gstore: { constPool.updateAt(current.operand.data.intval, operands[--sp]);  } break;
                case vm_lda:    { operands[sp++] = makeInt(current.operand.data.intval); } break;
                case vm_load:   { doLoadOp(current.operand.data.intval); } break;
                case vm_store:  { doStoreOp(); } break;
                case vm_fload:  { doFieldLoadOp(); } break;
                case vm_fstore: { doStoreFieldOp(); } break;
                case vm_print:  { doPrintOp(); } break;
                case vm_pop:    { sp--; } break;
                case vm_call:   { doCall(current.operand.data.intval); } break;
                case vm_ret:    { doReturn(); } break;
                case vm_mklist: { operands[sp++] = makeList(new List()); } break;
                case vm_br:    {  ip = current.operand.data.intval; } break;
                case vm_brt:  { 
                    if (operands[--sp].data.boolval) {
                        ip = current.operand.data.intval;
                    }
                } break;
                case vm_brf:  { 
                    if (operands[--sp].data.boolval == false) {
                        ip = current.operand.data.intval;
                    }
                } break;
                case vm_apndlist: {
                    operands[sp-2].data.listval = appendList(operands[sp-2].data.listval, operands[sp-1]);
                    sp--;
                } break;
                case vm_listsize: {
                    int size = operands[sp-1].data.listval->count;
                    operands[sp-1] = makeInt(size);
                } break;
                case vm_def: { } break;
                case vm_label: { } break;
                case vm_struct: { } break;
                default:
                    executeBinaryOperation(current);
                    break;
            }
        }
        void pushStackFrame(Function* function, int retAddr) {
            if (fp+1 == CALLSTACK_MAX) {
                cout<<"Calltack Exhausted."<<endl;
            }
            sf[fp++] = StackFrame(function, retAddr);
            for (int i = function->args-1; i >= 0 && sp > 0; i--) {
                sf[fp-1].locals[i] = operands[--sp];
            }
        }
        void doCall(int functionIndex) {
            Function* func = constPool.get(functionIndex).data.funcval;
            pushStackFrame(func, ip);
            ip = func->addr;
        }
        void doReturn() {
            ip = sf[fp-1].retAddr; 
            fp--;
        }
        void doStoreFieldOp() {
            int index = operands[sp-2].data.intval;
            List* list = operands[sp-3].data.listval;
            Object obj = operands[sp-1];
            sp -= 2;
            operands[--sp].data.listval = updateListAt(list, index, obj);
        }
        void doFieldLoadOp() {
            List* list = operands[sp-2].data.listval;
            int index = operands[sp-1].data.intval;
            sp -= 2;
            ListNode* it = getListItemAt(list, index);
            if (it != nullptr) {
                operands[sp++] = it->info;
            } else {
                cout<<"Subscript out of range."<<endl;
            }
        }
        void doLoadOp(int addr) {
            operands[sp++] = sf[fp-1].locals[addr];
        }
        void doStoreOp() { 
            sf[fp-1].locals[operands[sp-2].data.intval] = operands[sp-1];
            sp -= 2;
        }
        void doPrintOp() {
            cout<<operands[--sp];
        }
        void cpu() {
            running = true;
            ByteCodeInstruction current;
            while (running) {
                current = fetchInstruction();
                if (loud) {
                    cout<<"Instruction ("<<ip-1<<"): ";
                    printInstruction(current);
                }
                executeInstruction(current);
                displayInternalState();
            }
        }
    public:
        VM(bool debug = false) {
            loud = debug;
            codePage.resize(3000);
            ip = 0;
            fp = 0;
            mainFunction = new Function("main", 0, 0, 0);
            pushStackFrame(mainFunction, 0);            
        }
        void init(vector<ByteCodeInstruction>& code) {
            for (int i = 0; i < code.size(); i++) {
                codePage[i] = code[i];
                cout<<i<<": "; printInstruction(codePage[i]);
                if (code[i].instr == vm_halt) break;
            }
        }
        void exec() {
            cpu();
        }
        ConstantPool* getConstantPool() {
            return &constPool;
        }
};



#endif