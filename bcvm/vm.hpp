#ifndef vm_hpp
#define vm_hpp
#include <iostream>
#include <vector>
#include "../allocator.hpp"
#include "constant_pool.hpp"
#include "../object.hpp"
#include "bytecode.hpp"
using namespace std;

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
        Allocator alloc;
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
        void listAppend() {
            Object item = pop();
            peek(0).data.gcobj->listval = appendList(peek(0).data.gcobj->listval, item);
            
        }
        void listSize() {
            int size = pop().data.gcobj->listval->count;
            push(makeInt(size));
        }
        void executeBinaryOperation(ByteCodeInstruction& current) {
            Object r = pop();
            Object l = pop();
            switch (current.instr) {
                case vm_add: { push(add(l, r)); } break;
                case vm_sub: { push(sub(l, r)); } break;
                case vm_mul: { push(mul(l, r)); } break;
                case vm_div: { push(div(l, r)); } break;
                case vm_mod: { push(mod(l, r)); } break;
                case vm_lt:  { push(lt(l, r));  } break;
                case vm_gt:  { push(gt(l, r));  } break;
                case vm_lte: { push(lte(l, r)); } break;
                case vm_gte: { push(gte(l, r)); } break;
                case vm_equ: { push(equ(l, r)); } break;
                case vm_neq: { push(neq(l, r)); } break;
                default:
                    break;
            }
        }
        void executeInstruction(ByteCodeInstruction& current) {
            switch (current.instr) {
                case vm_halt:   { running = false; } break;
                case vm_const:  { push(current.operand); } break;
                case vm_not:    { peek(0).data.boolval = !(peek(0).data.boolval); } break;
                case vm_neg:    { peek(0).data.intval = -(peek(0).data.intval); } break;
                case vm_null:   { push(nilObject); } break;
                case vm_gload:  { push(constPool.get(current.operand.data.intval)); } break;
                case vm_gstore: { constPool.updateAt(current.operand.data.intval, pop());  } break;
                case vm_lda:    { push(makeInt(current.operand.data.intval)); } break;
                case vm_load:   { doLoadOp(current.operand.data.intval); } break;
                case vm_store:  { doStoreOp(); } break;
                case vm_fload:  { doFieldLoadOp(); } break;
                case vm_fstore: { doStoreFieldOp(); } break;
                case vm_print:  { doPrintOp(false); } break;
                case vm_println:{ doPrintOp(true); } break;
                case vm_pop:    { sp--; } break;
                case vm_open_scope: { pushStackFrame(nullptr, current.operand.data.intval); } break;
                case vm_close_scope: { popStackFrame(); } break;
                case vm_call:   { doCall(current.operand.data.intval); } break;
                case vm_ret:    { doReturn(); } break;
                case vm_mklist: { push(alloc.makeList(new List())); } break;
                case vm_br:     { branchUnconditional(current); } break;
                case vm_brf:    { branchConditional(current); } break;
                case vm_apndlist: { listAppend(); } break;
                case vm_listsize: { listSize(); } break;
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
            fp++;
            cout<<(fp-1)<<", Function: "<<function->name<<", retAddr: "<<retAddr<<endl;
            sf[fp-1] = StackFrame(function, retAddr);
            for (int i = function->args-1; i >= 0 && sp > 0; i--) {
                sf[fp-1].locals[i] = pop();
            }
        }
        void popStackFrame() {
            if (fp-1 >= 0)
                fp--;
        }
        void branchConditional(ByteCodeInstruction& current) {
            if (pop().data.boolval == false) {
                ip = current.operand.data.intval;
            }
        }
        void branchUnconditional(ByteCodeInstruction& current) {
            ip = current.operand.data.intval;
        }
        void doCall(int functionIndex) {
            Function* func = constPool.get(functionIndex).data.gcobj->funcval;
            pushStackFrame(func, ip);
            ip = func->addr;
        }
        void doReturn() {
            ip = sf[fp-1].retAddr; 
            fp--;
        }
        void doStoreFieldOp() {
            Object obj = pop();
            int index = pop().data.intval;
            List* list = pop().data.gcobj->listval;
            list = updateListAt(list, index, obj);
        }
        void doFieldLoadOp() {
            int index = pop().data.intval;
            List* list = pop().data.gcobj->listval;
            ListNode* it = getListItemAt(list, index);
            if (it != nullptr) {
                push(it->info);
            } else {
                cout<<"Subscript out of range."<<endl;
            }
        }
        void doLoadOp(int addr) {
            push(sf[fp-1].locals[addr]);
        }
        void doStoreOp() { 
            sf[fp-1].locals[operands[sp-2].data.intval] = operands[sp-1];
            sp -= 2;
        }
        void doPrintOp(bool newline) {
            cout<<pop();
            if (newline) { cout<<endl; }
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
        void push(Object info) {
            operands[sp++] = info;
        }
        Object pop() {
            return operands[--sp];
        }
        Object& peek(int offset) {
            return operands[(sp-1-offset)];
        }
    public:
        VM(bool debug = true) {
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
            if (ip > 0) ip--;
        }
        void exec() {
            cpu();
        }
        ConstantPool* getConstantPool() {
            return &constPool;
        }
        Allocator* getAllocator() {
            return &alloc;
        }
};



#endif