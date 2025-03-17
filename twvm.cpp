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
        Object nilVal;
        void push(Object info) {
            rtStack.push(info);
        }
        Object pop() {
            if (rtStack.empty()) {
                cout<<"Error: Stack Underflow."<<endl;
                return nilVal;
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
            Object m = cxt.getAlloc().makeFunction(func);
            cxt.insert(func->name, m);
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
            if (node->child[0]->token.symbol == TK_ID) {
                cxt.put(node->child[0]->token.strval, node->child[0]->token.depth, pop());
            } else {
                string id = node->child[0]->child[0]->token.strval;
                int depth = node->child[0]->child[0]->token.depth;
                evalExpr(node->child[0]->child[1]);
                int idx = pop().data.intval;
                ListNode* it = cxt.get(id, depth).data.gcobj->listval->head;
                int i = 0;
                while (it != nullptr && i < idx) {
                    it = it->next;
                    i++;
                }
                if (it != nullptr) {
                    it->info = pop();
                }
            }
            leave();
        }
        void functionCall(astnode* node) {
            enter("[Function call]");
            Object m = cxt.get(node->child[0]->token.strval, node->child[0]->token.depth);
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
            if (!cxt.getStack().empty())
                func->freeVars = cxt.getStack().top().locals;
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
            for (auto m : func->freeVars) {
                cxt.insert(m.first, m.second);
            }
            Scope env;
            evalFunctionArguments(params, func->params, env);
            cxt.openScope(env);
            say("[Applying Function]");
            exec(func->body);
            bailout = false;
            for (auto m : func->freeVars) {
                func->freeVars[m.first] = cxt.get(m.first, 1);
            }
            cxt.closeScope();
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
            push(makeInt(pop().data.gcobj->listval->count));
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
            List* list = pop().data.gcobj->listval;
            evalExpr(node->child[1]);
            int i = 0;
            int indx = pop().data.intval;
            ListNode* itr = list->head;
            while (itr != nullptr && i < indx) {
                i++;
                itr = itr->next;
            }
            if (itr != nullptr)
                push(itr->info);
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
        void evalExpr(astnode* node) {
            if (node != nullptr) {
                switch (node->type.expr) {
                    case UNOP_EXPR: unaryOperation(node); break;
                    case RELOP_EXPR: binaryOperation(node); break;
                    case BINOP_EXPR: binaryOperation(node); break;
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