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
        Object nilObject;
        StackFrame sf[CALLSTACK_MAX];
        int fp; // frame pointer into stackframe stack
        Object operands[CALLSTACK_MAX];
        int sp; // stack pointer into operands stack
        Function* mainFunction;
        vector<ByteCodeInstruction> codePage;
        int ip;
        ConstantPool constPool;
        ByteCodeInstruction& fetchInstruction() {
            return codePage[ip++];
        }
        void performIntegerOperation(ByteCodeInstruction& current) {
            switch (current.instr) {
                case vm_iadd: {
                    operands[sp-2].data.intval = operands[sp-2].data.intval + operands[sp-1].data.intval;
                    //operands[sp-2] = add(operands[sp-2], operands[sp-1]);
                    sp--;
                } break;
                case vm_isub: { 
                    operands[sp-2].data.intval = operands[sp-2].data.intval - operands[sp-1].data.intval;
                    sp--;
                } break;
                case vm_imul: { 
                    operands[sp-2].data.intval = operands[sp-2].data.intval * operands[sp-1].data.intval;
                    sp--;
                } break;
                case vm_idiv: { 
                    operands[sp-2].data.intval = operands[sp-2].data.intval / operands[sp-1].data.intval;
                    sp--;
                } break;
                case vm_imod: { 
                    operands[sp-2].data.intval = operands[sp-2].data.intval % operands[sp-1].data.intval;
                    sp--;
                } break;
                case vm_itof: { 
                    int tmp = operands[sp-1].data.intval; 
                    operands[sp-1].type = AS_REAL;
                    operands[sp-1].data.realval = (double)tmp;
                } break;
                case vm_neg: {
                    operands[sp-1].data.intval = -operands[sp-1].data.intval;
                } break;
                case vm_ilt: { 
                    operands[sp-2] = makeBool(operands[sp-2].data.intval < operands[sp-1].data.intval);
                    sp--;
                } break;
                case vm_igt: { 
                    operands[sp-2] = gt(operands[sp-2], operands[sp-1]);
                    sp--;
                } break;
                case vm_ilte: { 
                    operands[sp-2] = lte(operands[sp-2],operands[sp-1]);
                    sp--;
                } break;
                case vm_igte: { 
                    operands[sp-2] = gte(operands[sp-2], operands[sp-1]);
                    sp--;
                } break;
                case vm_iequ: { 
                    operands[sp-2] = equ(operands[sp-2], operands[sp-1]);
                    sp--;
                } break;
                case vm_ineq: {
                    operands[sp-2] = neq(operands[sp-2], operands[sp-1]);
                    sp--;
                } break;
                default: break;
            }
        }
        void doCall(int functionIndex) {
            Function* func = constPool.get(functionIndex).data.funcval;
            pushStackFrame(func, ip);
            for (int i = func->args-1; i >= 0 && sp > 0; i--) {
                sf[fp-1].locals[i] = operands[sp-1];
                sp--;
            }
            ip = func->addr;
        }
        void doStoreFieldOp() {
            int i = 0;
            ListNode* it = operands[sp-3].data.listval->head; 
            while (it != nullptr && i < operands[sp-3].data.listval->count) {
                if (i == operands[sp-2].data.intval)
                    break;
                it = it->next;
                i++;
            }
            it->info = operands[sp-1];
            sp -= 3;
        }
        void doFieldLoadOp() {
            int i = 0;
            ListNode* it = operands[sp-2].data.listval->head; 
            while (it != nullptr && i < operands[sp-2].data.listval->count) {
                if (i == operands[sp-1].data.intval)
                    break;
                it = it->next;
                i++;
            }
            sp -= 2;
            if (it != nullptr) {
                operands[sp++] = it->info;
            } else {
                cout<<"Subscript out of range."<<endl;
            }
        }
        void printInstruction(ByteCodeInstruction& bci) {
            cout<<"["<<VMInstrStr[bci.instr]<<", "<<bci.operand<<" ]"<<endl;
        }
        bool loud;
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
        void cpu() {
            bool running = true;
            ByteCodeInstruction current;
            while (running) {
                current = fetchInstruction();
                if (loud) {
                    cout<<"Instruction: ["<<(ip-1)<<"] ";
                    printInstruction(current);
                }
                switch (current.instr) {
                    case vm_halt: {
                        running = false;
                    } break;
                    case vm_cconst:
                    case vm_sconst:
                    case vm_fconst:
                    case vm_iconst: { operands[sp++] = current.operand; } break;
                    case vm_iadd:
                    case vm_isub:
                    case vm_imul:
                    case vm_idiv:
                    case vm_neg:
                    case vm_ilt: 
                    case vm_igt: 
                    case vm_iequ: 
                    case vm_ineq:
                    case vm_itof: { performIntegerOperation(current); } break;
                    case vm_not: {
                        operands[sp-1].data.boolval = !(operands[sp-1].data.boolval);
                    } break;
                    case vm_sconcat: {
                        Object lhs = operands[--sp];
                        Object rhs = operands[--sp];
                        string lstr = toString(lhs) + toString(rhs);
                        operands[sp++] = makeString(lstr);
                    } break;
                    case vm_struct: { } break;
                    case vm_null: {
                        operands[sp++] = nilObject;
                    } break;
                    case vm_print: {
                        cout<<operands[sp-1];
                        sp--;
                    } break;
                    case vm_pop: {
                        sp--;
                    } break;
                    case vm_call: { 
                        doCall(current.operand.data.intval);
                    } break;
                    case vm_ret:  { 
                        ip = sf[fp-1].retAddr; 
                        fp--;
                    } break;
                    case vm_br:   { 
                        ip = current.operand.data.intval;
                    } break;
                    case vm_brt:  { 
                        if (operands[sp-1].data.boolval) {
                            ip = current.operand.data.intval;
                        }
                        sp--;
                    } break;
                    case vm_brf:  { 
                        if (operands[sp-1].data.boolval == false) {
                            ip = current.operand.data.intval;
                        }
                        sp--;
                    } break;
                    case vm_gload: { 
                        operands[sp++] = constPool.get(current.operand.data.intval);
                    } break;
                    case vm_gstore: { 
                        constPool.updateAt(current.operand.data.intval, operands[sp-1]);
                        sp--;
                    } break;
                    case vm_load:  {
                        operands[sp++] = sf[fp-1].locals[current.operand.data.intval];
                    } break;
                    case vm_lda:  {
                        operands[sp++] = makeInt(current.operand.data.intval);
                    } break;
                    case vm_fload: {
                       doFieldLoadOp();
                    } break;
                    case vm_fstore: {
                        doStoreFieldOp();
                    } break;
                    case vm_store:  { 
                        sf[fp-1].locals[operands[sp-2].data.intval] = operands[sp-1];
                        sp -= 2;
                    } break;
                    case vm_def: {

                    } break;
                    case vm_label: {

                    } break;
                    case vm_mklist: {
                        operands[sp++] = makeList(new List());
                    } break;
                    case vm_apndlist: {
                        Object toAdd = operands[sp-1];
                        sp--;
                        operands[sp-1].data.listval = appendList(operands[sp-1].data.listval, toAdd);
                    } break;
                    case vm_listsize: {
                        int size = operands[sp-1].data.listval->count;
                        operands[sp-1] = makeInt(size);
                    } break;
                    default:
                        break;
                }
                displayInternalState();
            }
        }
        void pushStackFrame(Function* function, int retAddr) {
            if (fp+1 == CALLSTACK_MAX) {
                cout<<"Calltack Exhausted."<<endl;
            }
            sf[fp] = StackFrame(function, retAddr);
            fp++;
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