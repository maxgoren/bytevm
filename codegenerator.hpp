#ifndef codegenerator_hpp
#define codegenerator_hpp
#include <iostream>
#include <vector>
#include <unordered_map>
#include "ast.hpp"
#include "bytecode.hpp"
#include "object.hpp"
#include "stringbuffer.hpp"
#include "tokenstream.hpp"
#include "symboltable.hpp"
using namespace std;


class CodeGen {
    private:
        ConstantPool* constPool;
        SymbolTable st;
        vector<ByteCodeInstruction> code;
        int codeIndex;
        int highCI;
        int labelnum;
        void emit(VMInstr instr) {
            emit(instr, makeInt(0));
        }
        void emit(VMInstr instr, Object operand) {
            code[codeIndex++] = {instr, operand };
            if (highCI < codeIndex) highCI = codeIndex;
        }
        string emitLabel() {
            string label = "LBL" + to_string(labelnum++);
            emit(vm_label, makeString(label));
            return label;
        }
        int getLabelAddr(string label) {
            int i = 0;
            while (i < highCI) {
                if (code[i].instr == vm_label) {
                    if (*(code[i].operand.data.strval) == label)
                        return i;
                }
                i++;
            }
            return -1;
        }
        int skipEmit(int spots) {
            int old = codeIndex;
            codeIndex += spots;
            if (highCI < codeIndex) highCI = codeIndex;
            return old;
        }
        void backUpEmit(int addr) {
            codeIndex = addr;
        }
        void restoreEmit() {
            codeIndex = highCI;
        }
        void genListExpr(astnode* node) {
            switch (node->token.symbol) {
                case TK_LB: {
                    emit(vm_mklist);
                    for (auto it = node->child[0]; it != nullptr; it = it->next) {
                        genCodeNS(it);
                        emit(vm_apndlist);
                    }
                } break;
                case TK_APPEND: {
                    genCode(node->child[0]);
                    genCode(node->child[1]);
                    emit(vm_apndlist);
                }break;
                case TK_SIZE: {
                    genCode(node->child[0]);
                    emit(vm_listsize);
                } break;
                case TK_EMPTY: {
                    genCode(node->child[0]);
                    emit(vm_listsize);
                    emit(vm_const, makeInt(0));
                    emit(vm_equ);
                } break;
                default:
                    break;
            }
        }
        bool isSub;
        void genExpr(astnode* node, bool isAddr) {
            switch (node->type.expr) {
                case ID_EXPR: {
                    if (isSub) {
                        emit(vm_load, makeInt(st.get(node->token.strval).location));
                    } else {
                        emit(isAddr ? vm_lda:vm_load, makeInt(st.get(node->token.strval).location));
                    }
                }   break;
                case CONST_EXPR: {
                        switch (node->token.symbol) {
                            case TK_NUM: 
                                emit(vm_const, makeInt(atoi(node->token.strval.data())));
                                break;
                            case TK_STR:
                                emit(vm_const, makeString(node->token.strval));
                                break;
                            case TK_TRUE:
                                emit(vm_const, makeBool(true));
                                break;
                            case TK_FALSE:
                                emit(vm_const, makeBool(false));
                            case TK_NIL:
                                emit(vm_null);
                                break;
                            default:
                                break;
                        }
                    } break;
                case BINOP_EXPR: {
                    genExpr(node->child[0], false);
                    genExpr(node->child[1], false);
                    switch (node->token.symbol) {
                        case TK_ADD: emit(vm_add); break;
                        case TK_SUB: emit(vm_sub); break;
                        case TK_MUL: emit(vm_mul); break;
                        case TK_DIV: emit(vm_div); break;
                        case TK_MOD: emit(vm_mod); break;
                        default: break;
                    }
                } break;
                case RELOP_EXPR: {
                    genExpr(node->child[0], false);
                    genExpr(node->child[1], false);
                    switch (node->token.symbol) {
                        case TK_LT: emit(vm_lt); break;
                        case TK_GT: emit(vm_gt); break;
                        case TK_LTE: emit(vm_lte); break;
                        case TK_GTE: emit(vm_gte); break;
                        case TK_EQU: emit(vm_equ); break;
                        case TK_NEQ: emit(vm_neq); break;
                        default:
                            break;
                    }
                } break;
                case LIST_EXPR: {
                    genListExpr(node);
                } break;
                case SUBSCRIPT_EXPR: {
                    genExpr(node->child[0], isAddr);
                    genExpr(node->child[1], false);
                    if (isAddr == false)
                        emit(vm_fload);
                } break;
                case UNOP_EXPR: {
                    genExpr(node->child[0], false);
                    switch (node->token.symbol) {
                        case TK_SUB:
                            emit(vm_neg);
                            break;
                        case TK_NOT:
                            emit(vm_not);
                            break;
                        default:
                            break;
                    }
                } break;
                case ASSIGN_EXPR: {
                    isSub = node->child[0]->type.expr == SUBSCRIPT_EXPR;
                    genExpr(node->child[0], true);
                    genExpr(node->child[1], false);
                    if (isSub) {
                        emit(vm_fstore);
                    } else {
                        emit(vm_store);
                    }
                } break;
                case FUNC_EXPR: {
                    genCode(node->child[1]);
                    emit(vm_call, makeInt(st.get(node->child[0]->token.strval).location));
                };
                default:
                    break;
            }
        }
        void genIfStmt(astnode* node) {
            genExpr(node->child[0], false);
            int s1 = skipEmit(1);
            genCode(node->child[1]);
            int s2 = skipEmit(1);
            int curr = skipEmit(0);
            backUpEmit(s1);
            emit(vm_brf, makeInt(curr));
            restoreEmit();
            genCode(node->child[2]);
            curr = skipEmit(0);
            backUpEmit(s2);
            emit(vm_br, makeInt(curr));
            restoreEmit();
        }
        void genWhileStmt(astnode* node) {
            string test_label = emitLabel();
            genExpr(node->child[0], false);
            int s1 = skipEmit(1);
            genCode(node->child[1]);
            emit(vm_br, makeInt(getLabelAddr(test_label)));
            string exit_label = emitLabel();
            backUpEmit(s1);
            emit(vm_brf, makeInt(getLabelAddr(exit_label)));
            restoreEmit();
        }
        void genFunctionDefinition(astnode* node) {
            int s1 = skipEmit(2);
            st.openFunctionScope(node->token.strval);
            emit(vm_def, makeInt(st.get(node->token.strval).location));
            genCode(node->child[1]);
            st.closeScope();
            emit(vm_ret);
            string end_label = emitLabel();
            backUpEmit(s1);
            emit(vm_br, makeInt(getLabelAddr(end_label)));
            string entrance = emitLabel();
            constPool->get(st.get(node->token.strval).location).data.funcval->addr = getLabelAddr(entrance);
            restoreEmit();
        }
        void genStmt(astnode* node) {
            switch (node->type.stmt) {
                case PRINT_STMT: {
                    genExpr(node->child[0], false);
                    emit(vm_print);
                    if (node->token.symbol == TK_PRINTLN) {
                        emit(vm_const, makeString("\n"));
                        emit(vm_print);
                    }
                } break;
                case EXPR_STMT: {
                    genExpr(node->child[0], false);
                } break;
                case LET_STMT: {
                    genExpr(node->child[0], false);
                } break;
                case IF_STMT: {
                    genIfStmt(node); 
                } break;
                case WHILE_STMT: {
                    genWhileStmt(node); 
                } break;
                case FUNC_DEF_STMT: {
                    genFunctionDefinition(node);
                } break;
                case RETURN_STMT: {
                    genCode(node->child[0]);
                    emit(vm_ret);
                } break;
                default:
                    break;
            }
        }
        void genCode(astnode* node) {
            if (node != nullptr) {
                switch (node->nk) {
                    case EXPR_NODE: genExpr(node, false); break;
                    case STMT_NODE: genStmt(node); break;
                    default:
                        break;
                }
                genCode(node->next);
            }
        }
        void genCodeNS(astnode* node) {
            if (node != nullptr) {
                switch (node->nk) {
                    case EXPR_NODE: genExpr(node, false); break;
                    case STMT_NODE: genStmt(node); break;
                    default:
                        break;
                }
            }
        }
        void buildSymbolTable(astnode* node) {
            if (node != nullptr) {
                bool sayLess = false;
                switch (node->nk) {
                    case EXPR_NODE:
                        switch (node->type.expr) {
                            case ID_EXPR: {
                                if (st.get(node->token.strval).type == EMPTY) {
                                    cout<<"Undeclared variable '"<<node->token.strval<<"' referenced"<<endl;
                                }
                            } break;
                            default:
                                break;
                        } break;
                    case STMT_NODE:
                        switch (node->type.stmt) {
                            case LET_STMT: {
                                astnode* x = node;
                                while (x != nullptr) {
                                    if (isExprType(x, ID_EXPR))
                                        break;
                                    x = x->child[0];
                                }
                                if (isExprType(x, ID_EXPR)) {
                                    if (st.get(x->token.strval).type == EMPTY) {
                                        STEntry ent; ent.type = LOCALVAR;
                                        ent.name = x->token.strval;
                                        st.insert(x->token.strval, ent);
                                    } else {
                                        cout<<"A variable with name: "<<x->token.strval<<" has already been declared in this scope."<<endl;
                                    }
                                }
                            } break;
                            case FUNC_DEF_STMT: { 
                                st.openFunctionScope(node->token.strval);
                                int na = 0;
                                for (astnode* params = node->child[0]; params != nullptr; params = params->next) {
                                    STEntry ent; ent.type = LOCALVAR;
                                    ent.name = params->token.strval;
                                    st.insert(params->token.strval, ent);
                                    na++;
                                }
                                Function* f = new Function(node->token.strval,na,0,0);
                                Object obj;
                                obj.type = AS_FUNC;
                                obj.data.funcval = f;
                                int addr = constPool->alloc(obj);
                                buildSymbolTable(node->child[1]);
                                constPool->get(addr).data.funcval->locals = st.numLocals()-na;
                                st.closeScope();
                                sayLess = true;
                            } break;
                            default:
                                break;
                        } break;
                    default:
                        break;
                }
                if (!sayLess) {
                    for (int i = 0; i < MAX_CHILD; i++)
                        buildSymbolTable(node->child[i]);
                }
                buildSymbolTable(node->next);
            }
        }
    public:
        CodeGen(ConstantPool* cp) {
            isSub = false;
            code= vector<ByteCodeInstruction>(255);
            labelnum = 0;
            highCI = 0;
            codeIndex = 0;
            constPool = cp;
        }
        vector<ByteCodeInstruction>& compile(astnode* ast) {
            if (codeIndex > 0) codeIndex--;
            cout<<"Building Symbol Table..";
            buildSymbolTable(ast);
            cout<<"Symbol Table Complete: "<<endl;
            st.show();
            cout<<"Generating PCode..."<<endl;
            genCode(ast);
            emit(vm_halt);
            cout<<"PCode Generation Complete."<<endl;
            return code;
        }
};

#endif