#include <iostream>
#include <vector>
#include "allocator.hpp"
#include "astbuilder.hpp"
#include "context.hpp"
#include "object.hpp"
#include "stringbuffer.hpp"
#include "resolve.hpp"
#include "tokenstream.hpp"
#include "regex/patternmatcher.hpp"
#include <unordered_map>
using namespace std;


class TWVM {
    private:
        bool bailout;
        bool loud;
        int rd;
        void say(string s) {
            if (loud) {
                for (int i = 0;i < rd; i++) cout<<"  ";
                cout<<s<<endl;
            }
        }
        void enter(string s) {
            rd++;
            if (loud) say(s);
        }
        void leave() {
            rd--;
        }
        Context cxt;
        IndexedStack<Object> rtStack;
        void push(Object info) {
            rtStack.push(info);
        }
        Object pop() {
            if (rtStack.empty()) {
                cout<<"Error: Stack Underflow."<<endl;
                return cxt.nil();
            }
            return rtStack.pop();
        }
        Object& peek(int spaces) {
            return rtStack.get(rtStack.size()-1-spaces);
        }
        /* statement handlers */
        void letStatement(astnode* node) {
            enter("[let statement]");
            string id = node->child[0]->child[0]->token.strval;
            cout<<id<<endl;
            evalExpr(node->child[0]);
            leave();
        }
        void ifStatement(astnode* node) {
            enter("[if statement]");
            evalExpr(node->child[0]);
            if (pop().data.boolval) {
                exec(node->child[1]);
            } else {
                exec(node->child[2]);
            }
            leave();
        }
        void whileStatement(astnode* node) {
            enter("[while statement]");
            evalExpr(node->child[0]);
            while (pop().data.boolval) {
                exec(node->child[1]);
                exec(node->child[0]);
            }
            leave();
        }
        void printStatement(astnode* node) {
            enter("[print statement]");
            evalExpr(node->child[0]);
            cout<<toString(pop());
            if (node->token.symbol == TK_PRINTLN)
                cout<<endl;
            leave();
        }
        void defineFunction(astnode* node) {
            enter("[function definition statement: " + node->token.strval + "]");
            Function* func = new Function(copyTree(node->child[0]), copyTree(node->child[1]));
            func->name = node->token.strval;
            func->freeVars = nullptr;
            if (!cxt.getStack().empty()) {
                for (auto fv : cxt.getStack().top().locals) {
                    func->freeVars = new VarList(fv.first, fv.second, func->freeVars);
                }
            }
            Object m = cxt.getAlloc().makeFunction(func);
            cxt.insert(func->name, m);
            leave();
        }
        void defineStruct(astnode* node) {
            enter("[struct definition statement]");
            Struct* st = new Struct(node->child[0]->token.strval);
            for (astnode* it = node->child[1]; it != nullptr; it = it->next) {
                st->fields[it->child[0]->token.strval] = makeNil();
            }
            cxt.addStructType(st);
            leave();
        }
        void blockStatement(astnode* node) {
            enter("[block statement]");
            cxt.openScope();
            exec(node->child[0]);
            cxt.closeScope();
            leave();
        }
        void expressionStatement(astnode* node) {
            enter("[expression statement]");
            evalExpr(node->child[0]);
            leave();
        }
        void returnStatement(astnode* node) {
            enter("[return statement]");
            evalExpr(node->child[0]);
            bailout = true;
            leave();
        }
        /* Expression Handlers */
        void constExpr(astnode* node) {
            enter("[const expression - (" + node->token.strval + ")]");
            switch (node->token.symbol) {
                case TK_TRUE: push(makeBool(true)); break;
                case TK_FALSE: push(makeBool(false)); break;
                case TK_NUM: push(makeNumber(stod(node->token.strval))); break;
                case TK_STR: push(cxt.getAlloc().makeString(node->token.strval)); break;
                case TK_NIL: push(cxt.nil()); break;
                default: 
                    break;
            }
            leave();
        }
        void rangeExpression(astnode* node) {
            evalExpr(node->child[0]);
            Object lhs = pop();
            int l = lhs.data.intval;
            evalExpr(node->child[1]);
            Object rhs = pop();
            int r = rhs.data.intval;
            List* nl = new List();
            if (l < r) {
                for (int i = l; i <= r; i++)
                    nl = appendList(nl, makeInt(i));
            } else {
                for (int i = l; i >= r; i--)
                    nl = appendList(nl, makeInt(i));
            }
            push(cxt.getAlloc().makeList(nl));
        }
        void idExpr(astnode* node) {
            enter("[id expression: " + node->token.strval + ", scope depth: " + to_string(node->token.depth) + "]");
            push(cxt.get(node->token.strval, node->token.depth));
            leave();
        }
        void unaryOperation(astnode* node) {
            enter("[unary operation]");
            evalExpr(node->child[0]);
            switch (node->token.symbol) {
                case TK_NOT: push(makeBool(!pop().data.boolval)); break;
                case TK_SUB: push(neg(pop())); break;
                case TK_POST_DEC: {
                    Object m = pop();
                    if (m.type == AS_INT) {
                        m.data.intval -= 1;
                    } else if (m.type == AS_REAL) {
                        m.data.realval -= 1;
                    }
                    cxt.put(node->child[0]->token.strval, node->child[0]->token.depth, m);
                } break;
                case TK_POST_INC: {
                    Object m = pop();
                    if (m.type == AS_INT) {
                        m.data.intval += 1;
                    } else if (m.type == AS_REAL) {
                        m.data.realval += 1;
                    }
                    cxt.put(node->child[0]->token.strval, node->child[0]->token.depth, m);
                } break;
                default: break;
            }
            leave();
        }
        void binaryOperation(astnode* node) {
            enter("[binary operation (" + node->token.strval + ")]");
            evalExpr(node->child[0]);
            evalExpr(node->child[1]);
            Object rhs = pop();
            Object lhs = pop();
            if (node->token.symbol == TK_ADD && (typeOf(lhs) == AS_STRING || typeOf(rhs) == AS_STRING)) {
                string newstr = toString(lhs) + toString(rhs);
                push(cxt.getAlloc().makeString(newstr));
                return;
            }
            switch (node->token.symbol) {
                case TK_ADD: push(add(lhs, rhs)); break;
                case TK_SUB: push(sub(lhs, rhs)); break;
                case TK_MUL: push(mul(lhs, rhs)); break;
                case TK_DIV: push(div(lhs, rhs)); break;
                case TK_MOD: push(mod(lhs, rhs)); break;
                case TK_EQU: push(equ(lhs, rhs)); break;
                case TK_NEQ: push(neq(lhs, rhs)); break;
                case TK_POW: push(pow(lhs, rhs)); break;
                case TK_LT:  push(lt(lhs, rhs)); break;
                case TK_LTE: push(lte(lhs, rhs)); break;
                case TK_GT:  push(gt(lhs, rhs)); break;
                case TK_GTE: push(gte(lhs, rhs)); break;
                default:
                    break;
            }
            leave();
        }
        void assignExpr(astnode* node) {
            enter("[assignment expression]");
            evalExpr(node->child[1]);
            if (isExprType(node->child[0], ID_EXPR)) {
                cxt.put(node->child[0]->token.strval, node->child[0]->token.depth, pop());
            } else if (isExprType(node->child[0], SUBSCRIPT_EXPR)) {
                string id = node->child[0]->child[0]->token.strval;
                int depth = node->child[0]->child[0]->token.depth;
                Object obj = cxt.get(id, depth);
                if (obj.type == AS_LIST) {
                    evalExpr(node->child[0]->child[1]);
                    int idx = pop().data.intval;
                    ListNode* it = cxt.get(id, depth).data.gcobj->listval->head;
                    int i = 0;
                    while (it != nullptr && i < idx) {
                        it = it->next;
                        i++;
                    }
                    if (it != nullptr)  it->info = pop();
                } else if (obj.type == AS_STRUCT) {
                    string fieldname = node->child[0]->child[1]->token.strval;
                    cout<<"Assigning to field "<<fieldname<<endl;
                    Struct* st = obj.data.gcobj->structval;
                    if (st->fields.find(fieldname) != st->fields.end()) {
                        st->fields[fieldname] = pop();
                    } else {
                        cout<<"No field named '"<<fieldname<<"' in object "<<id<<" of type "<<st->typeName<<endl;
                    }
                }
            }
            leave();
        }
        void functionCall(astnode* node) {
            enter("[Function call]");
            Object m;
            if (node->child[0]->token.strval == "_rc") {
                m = cxt.getStack().top().locals["_rc"];
            } else { 
                m = cxt.get(node->child[0]->token.strval, node->child[0]->token.depth);
            }
            if (m.type != AS_FUNC) {
                cout<<"Couldn't find function named: "<<node->child[0]->token.strval<<endl;
                return;
            }
            Function* func = m.data.gcobj->funcval;
            funcExpression(func, node->child[1]);
            leave();
        }
        void lambdaExpression(astnode* node) {
            enter("[lambda expr]");
            Function* func = new Function(copyTree(node->child[0]), copyTree(node->child[1]));
            func->name = "(lambda)";
            func->freeVars = nullptr;
            if (!cxt.getStack().empty()) {
                for (auto fv : cxt.getStack().top().locals) {
                    func->freeVars = new VarList(fv.first, fv.second, func->freeVars);
                }
            }
            push(cxt.getAlloc().makeFunction(func));
            leave();
        }
        void evalFunctionArguments(astnode* args, astnode* params, Scope& env) {
            say("[Evaluating Arguments]");
            int i = 0;
            while (params != nullptr && args != nullptr) {
                evalExpr(args);
                env.locals[params->token.strval] = pop();
                params = params->next;
                args = args->next;
                i++;
            }
            say("[ " + to_string(i) + " arguments evaluated.]");
        }
        void funcExpression(Function* func, astnode* params) {
            enter("[Function Expression]");
            for (auto m = func->freeVars; m != nullptr; m = m->next) {
                cxt.insert(m->key, m->m);
            }
            Scope env;
            evalFunctionArguments(params, func->params, env);
            cxt.openScope(env);
            cxt.insert("_rc", cxt.getAlloc().makeFunction(func));
            say("[Applying Function]");
            exec(func->body);
            bailout = false;
            cxt.closeScope();
            for (auto m = func->freeVars; m != nullptr; m = m->next) {
                m->m = cxt.get(m->key, 0);
            }
            leave();
        }
        void listExpression(astnode* node) {
            enter("[list expression: " + node->token.strval + "]");
            switch (node->token.symbol) {
                case TK_LB: makeAnonymousList(node); break;
                case TK_SIZE: getListSize(node); break;
                case TK_EMPTY: getListEmpty(node); break;
                case TK_APPEND: doAppendList(node); break;
                case TK_PUSH:   doPushList(node); break;
                case TK_FIRST:  getFirstListElement(node); break;
                case TK_REST:   getRestOfList(node); break;
                case TK_MAP:    doMap(node);break;
                case TK_FILTER: doFilter(node); break;
                case TK_REDUCE: doReduce(node); break;
                case TK_SORT: doSort(node); break;
                default:
                    break;
            }
            leave();
        }
        void doAppendList(astnode* node) {
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            list = appendList(list, pop());
        }
        void doPushList(astnode* node) {
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            list = pushList(list, pop());
        }
        void getListSize(astnode* node) {
            evalExpr(node->child[0]);
            int size = 0;
            Object m = pop();
            if (m.type != AS_LIST) {
                cout<<"Error: size() expects list."<<endl;
                push(makeNil());
                return;
            }
            size = m.data.gcobj->listval->count;
            push(makeInt(size));
        }
        void getListEmpty(astnode* node) {
            evalExpr(node->child[0]);
            push(makeBool(pop().data.gcobj->listval->head == nullptr));
        }
        void getFirstListElement(astnode* node) {
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            push(list->head->info);
        }
        void getRestOfList(astnode* node) {
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            List* nl = new List();
            for (ListNode* it = list->head->next; it != nullptr; it = it->next)
                nl = appendList(nl, it->info);
            push(cxt.getAlloc().makeList(nl));
        }
        Symbol getSymbol(Object m) {
            switch (m.type) {
                case AS_INT: return TK_NUM;
                case AS_REAL: return TK_NUM;
                case AS_BOOL: return m.data.boolval ? TK_TRUE:TK_FALSE;
                case AS_STRING: return TK_ID;
                case AS_LIST: return TK_LB;
                case AS_FUNC: return TK_LAMBDA;
                default:
                    break;
            }
            return TK_NIL;
        }
        void doMap(astnode* node) {
            enter("[ Map ]");
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            Function* func = pop().data.gcobj->funcval;
            List* result = new List();
            for (ListNode* it = list->head; it != nullptr; it = it->next) {
                astnode* t = makeExprNode(CONST_EXPR, Token(getSymbol(it->info), toString(it->info)));
                funcExpression(func, t);
                result = appendList(result, pop());
            }
            push(cxt.getAlloc().makeList(result));
            leave();
        }
        void doFilter(astnode* node) {
            enter("[ Filter ]");
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            Function* func = pop().data.gcobj->funcval;
            List* result = new List();
            for (ListNode* it = list->head; it != nullptr; it = it->next) {
                astnode* t = makeExprNode(CONST_EXPR, Token(getSymbol(it->info), toString(it->info)));
                funcExpression(func, t);
                if (pop().data.boolval)
                    result = appendList(result, it->info);
            }
            push(cxt.getAlloc().makeList(result));
            leave();
        }
        void doReduce(astnode* node) {
            enter("[ Reduce ]");
            evalExpr(node->child[0]);
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            Function* func = pop().data.gcobj->funcval;
            ListNode* it = list->head; 
            Object result = it->info;
            it = it->next;
            while (it != nullptr) {
                astnode* t = makeExprNode(CONST_EXPR, Token(getSymbol(result), toString(result)));
                t->next = makeExprNode(CONST_EXPR, Token(getSymbol(it->info), toString(it->info)));
                funcExpression(func, t);
                result = pop();
                it = it->next;
            }
            push(result);
            leave();
        }
        void doSort(astnode* node) {
            
        }
        void makeAnonymousList(astnode* node) { 
            List* list = new List();
            for (astnode* it = node->child[0]; it != nullptr; it = it->next) {
                evalExpr(it);
                list = appendList(list, pop());
            }
            push(cxt.getAlloc().makeList(list));
        }
        void subscriptExpression(astnode* node) {
            evalExpr(node->child[0]);
            if (peek(0).type == AS_LIST) {
                List* list = pop().data.gcobj->listval;
                evalExpr(node->child[1]);
                int i = 0;
                int indx = pop().data.intval;
                ListNode* itr = list->head;
                while (itr != nullptr && i < indx) {
                    i++;
                    itr = itr->next;
                }
                if (itr != nullptr) push(itr->info);
            } else if (peek(0).type == AS_STRUCT) {
                Struct* st = pop().data.gcobj->structval;
                evalExpr(node->child[0]);
                string name = *(pop().data.gcobj->strval);
                push(st->fields[name]);
            }
        }
        void listComprehension(astnode* node) {
            enter("[ZF Expression]");
            evalExpr(node->child[0]);
            Object listobj = pop();
            if (listobj.type != AS_LIST) {
                cout<<"Error: list comprehensions only work on lists."<<endl;
                return;
            }
            evalExpr(node->child[1]);
            Function* func = pop().data.gcobj->funcval;
            Function* pred = nullptr;
            if (node->child[2] != nullptr) {
                evalExpr(node->child[2]);
                pred = pop().data.gcobj->funcval;
            }
            List* list = listobj.data.gcobj->listval;
            List* result = new List();
            for (ListNode* it = list->head; it != nullptr; it = it->next) {
                astnode* t = makeExprNode(CONST_EXPR, Token(getSymbol(it->info), toString(it->info)));
                if (pred != nullptr) {
                    funcExpression(pred, t);
                    if (pop().data.boolval) {
                        funcExpression(func, t);
                        result = appendList(result, pop());
                    }
                } else {
                    funcExpression(func, t);
                    result = appendList(result, pop());
                }
            }
            push(cxt.getAlloc().makeList(result));
        }
        void regularExpression(astnode* node) {
            enter("[regular expr]");
            evalExpr(node->child[0]);
            string text = *pop().data.gcobj->strval;
            evalExpr(node->child[1]);
            string pattern = *pop().data.gcobj->strval;
            push(makeBool(matchre(text, pattern)));
            leave();
        }
        void blessExpression(astnode* node) {
            enter("[bless expr]");
            string name = node->child[0]->token.strval;
            Struct* st = cxt.getInstanceType(name);
            if (st == nullptr) {
                cout<<"No such type '"<<name<<"'"<<endl;
                leave();
                return;
            } else {
                cout<<"Bingo bango: "<<st->typeName<<endl;
            }
            Struct* nextInstance = new Struct(st->typeName);
            for (auto m : st->fields) {
                nextInstance->fields[m.first] = m.second;
            }
            nextInstance->blessed = true;
            push(cxt.getAlloc().makeStruct(nextInstance));
            leave();
        }
        void booleanOperation(astnode* node) {
            if (node->token.symbol == TK_AND) {
                evalExpr(node->child[0]);
                if (peek(0).data.boolval) {
                    pop();
                    evalExpr(node->child[1]);
                }
            } else if (node->token.symbol == TK_OR) {
                evalExpr(node->child[0]);
                if (!peek(0).data.boolval) {
                    pop();
                    evalExpr(node->child[1]);
                }
            }
        }
        void evalExpr(astnode* node) {
            if (node != nullptr) {
                switch (node->type.expr) {
                    case UNOP_EXPR: unaryOperation(node); break;
                    case RELOP_EXPR: binaryOperation(node); break;
                    case BINOP_EXPR: binaryOperation(node); break;
                    case LOGIC_EXPR: booleanOperation(node); break;
                    case ASSIGN_EXPR: assignExpr(node); break;
                    case CONST_EXPR: constExpr(node); break;
                    case ID_EXPR:    idExpr(node); break;
                    case FUNC_EXPR:  functionCall(node); break;
                    case LIST_EXPR:  listExpression(node); break;
                    case SUBSCRIPT_EXPR: subscriptExpression(node); break;
                    case REG_EXPR:   regularExpression(node); break;
                    case LAMBDA_EXPR: lambdaExpression(node); break;
                    case RANGE_EXPR:  rangeExpression(node); break;
                    case ZF_EXPR:     listComprehension(node); break;
                    case BLESS_EXPR:  blessExpression(node); break;
                    default:
                        break;
                }
            }
        }
    public:
        TWVM(bool debug = false) {
            loud = debug;
            bailout = false;
        }
        void exec(astnode* node) {
            if (node != nullptr) {
                if (node->nk == STMT_NODE) {
                    switch (node->type.stmt) {
                        case LET_STMT: letStatement(node); break;
                        case IF_STMT:  ifStatement(node); break;
                        case WHILE_STMT: whileStatement(node); break;
                        case PRINT_STMT: printStatement(node); break;
                        case EXPR_STMT: expressionStatement(node); break;
                        case FUNC_DEF_STMT: defineFunction(node); break;
                        case RETURN_STMT: returnStatement(node); break;
                        case BLOCK_STMT: blockStatement(node); break;
                        case STRUCT_DEF_STMT: defineStruct(node); break;
                        default:
                            break;
                    }
                } else {
                    evalExpr(node);
                }
                if (!bailout)
                    exec(node->next);
            }
        }
};

void runScript(string filename) {
    ASTBuilder astbuilder;
    TWVM vm(true);
    vm.exec(astbuilder.buildFromFile(filename));
}


void repl() {
    bool running = true;
    string input;
    ASTBuilder astbuilder;
    TWVM vm(true);
    while (running) {
        cout<<"meh> ";
        getline(cin, input);
        if (input == "quit") {
            running = false;
        } else {
            astnode* ast = astbuilder.build(input);
            preorder(ast, 1);
            vm.exec(ast);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        repl();
    } else {
        runScript(argv[1]);
    }
    return 0;
}