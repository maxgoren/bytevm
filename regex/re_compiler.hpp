#ifndef re_compiler_hpp
#define re_compiler_hpp
#include <iostream>
#include "re_parser.hpp"
#include "nfa.hpp"
using namespace std;

class NFACompiler {
    private:
        int l;
        IndexedStack<NFA> nfaStack;
        int makeStateLabel() {
            return l++;
        }
        void initNextNFA(NFA& nnfa) {
            int nstart = makeStateLabel();
            nnfa.makeState(nstart);
            nnfa.setStart(nstart);
            int nend = makeStateLabel();
            nnfa.makeState(nend);
            nnfa.setAccept(nend);
        }
        void copyTransitions(NFA& nnfa, NFA onfa) {
            for (auto state : onfa.getStates()) {
                nnfa.makeState(state.first);
                for (auto trans : onfa.getTransitions(state.first)) {
                    nnfa.addTransition(trans);
                }
            }
        }
        NFA singleTransitionNFA(Edge* edge) {
            NFA nfa;
            initNextNFA(nfa);
            nfa.addTransition(Transition(nfa.getStart(), nfa.getAccept(), edge)); 
            return nfa;
        }

        NFA emptyExpr() {
            return singleTransitionNFA(new EpsilonEdge());
        }
        /*
            A -> N(A)
        */
        NFA atomicNFA(RegExToken c) {
            return singleTransitionNFA(new CharEdge(c));
        }

        /*
             AB   -> N(N(A)->N(B)) ->
        */
        NFA concatNFA(NFA first, NFA second) {
            NFA nnfa;
            copyTransitions(nnfa, first);
            copyTransitions(nnfa, second);
            nnfa.setStart(first.getStart());
            nnfa.setAccept(second.getAccept());
            nnfa.addTransition(Transition(first.getAccept(), second.getStart(), new EpsilonEdge()));
            return nnfa;
        }
        /*
                     ___N(A)_
                    /     |  \
            A* -> N(S)<---'   N(E)->
                    \________/
                     
        */
        NFA kleeneNFA(NFA torepeat, bool mustMatch) {
            NFA nnfa;
            initNextNFA(nnfa);
            copyTransitions(nnfa, torepeat);
            nnfa.addTransition(Transition(torepeat.getAccept(), nnfa.getStart(), new EpsilonEdge()));
            nnfa.addTransition(Transition(nnfa.getStart(), torepeat.getStart(), new EpsilonEdge()));
            nnfa.addTransition(Transition(torepeat.getAccept(), nnfa.getAccept(), new EpsilonEdge()));
            if (!mustMatch)
                nnfa.addTransition(Transition(nnfa.getStart(), nnfa.getAccept(), new EpsilonEdge()));
            return nnfa;
        }
        /* 
                          __N(A)___
                         /         \
            A|B   -> N(S)           N(E) ->
                         \__     __/
                             N(B)

        */
        NFA alternateNFA(NFA first, NFA second) {
            //create new NFA with start and end state.
            NFA nnfa;
            initNextNFA(nnfa);
            //copy states and transitions from both NFA's to new NFA
            copyTransitions(nnfa, first);
            copyTransitions(nnfa, second);
            //Add new E-transitions from new start state to first and second NFAs
            nnfa.addTransition(Transition(nnfa.getStart(), first.getStart(), new EpsilonEdge()));
            nnfa.addTransition(Transition(nnfa.getStart(), second.getStart(), new EpsilonEdge()));
            //Add new E-transitions from first and second accept state to new accept state.
            nnfa.addTransition(Transition(first.getAccept(), nnfa.getAccept(), new EpsilonEdge()));
            nnfa.addTransition(Transition(second.getAccept(), nnfa.getAccept(), new EpsilonEdge()));
            return nnfa;
        }
        NFA zeroOrOnce(NFA onfa) {
            NFA nnfa;
            initNextNFA(nnfa);
            copyTransitions(nnfa, onfa);
            //wire in match choice
            nnfa.addTransition(Transition(nnfa.getStart(), onfa.getStart(), new EpsilonEdge()));
            nnfa.addTransition(Transition(onfa.getAccept(), nnfa.getAccept(), new EpsilonEdge()));
            //wire in epsilon choice.
            nnfa.addTransition(Transition(nnfa.getStart(), nnfa.getAccept(), new EpsilonEdge()));
            return nnfa;
        }

        //There is more than likely a more efficient way of doing this - but heres the plan:
        // initialize N new NFA's as N(n) where n is the nfa to be repeated, placing them into a stack.
        // Initialize our NFA. 'F' that will be the NFA(n, n, n...), store F's accept state as p.
        // While NFA's remain on the stack, remove top nfa from stack and store as t.
        // copy t's transitions into F, creating an epsilon transition from p to tmp's start state, then assigning tmps accept state as p.
        //When the stack is empty create one final epsilong transition from p to F's accept state.
        NFA repeatNTimes(NFA a, int N) {
            IndexedStack<NFA> sf;
            for (int i = 0; i < N; i++) {
                NFA tnfa;
                initNextNFA(tnfa);
                copyTransitions(tnfa, a);
                tnfa.addTransition(Transition(tnfa.getStart(), a.getStart(), new EpsilonEdge()));
                tnfa.addTransition(Transition(a.getAccept(), tnfa.getAccept(), new EpsilonEdge()));
                sf.push(tnfa);
            }
            NFA fnfa;
            initNextNFA(fnfa);
            State prev = fnfa.getStart();
            while (!sf.empty()) {
                NFA tmp = sf.pop();
                copyTransitions(fnfa, tmp);
                fnfa.addTransition(Transition(prev, tmp.getStart(), new EpsilonEdge()));
                prev = tmp.getAccept();
            }
            fnfa.addTransition(Transition(prev, fnfa.getAccept(), new EpsilonEdge()));
            return fnfa;
        }
        NFA buildOperatorNFA(RegularExpression* ast) {
            switch (ast->getSymbol().symbol) {
                case RE_CONCAT: {
                    NFA b = nfaStack.pop();
                    NFA a = nfaStack.pop();
                    return concatNFA(a, b);
                }
                case RE_OR: {
                    NFA b = nfaStack.pop();
                    NFA a = nfaStack.pop();
                    return alternateNFA(a, b);
                }
                case RE_STAR: {
                    NFA a = nfaStack.pop();
                    return kleeneNFA(a, false);
                }
                case RE_PLUS: {
                    NFA a = nfaStack.pop();
                    return kleeneNFA(a, true);
                }
                case RE_QUESTION: {
                    NFA a = nfaStack.pop();
                    return zeroOrOnce(a);
                }
                case RE_QUANTIFIER: {
                    NFA a = nfaStack.pop();
                    return repeatNTimes(a, atoi(ast->getSymbol().charachters.c_str()));
                }
                default:
                    break;
            }
            return NFA();
        }
        void gen_nfa(RegularExpression* ast) {
            if (ast != nullptr) {
                gen_nfa(ast->getLeft());
                gen_nfa(ast->getRight());
                if (!isOp(ast->getSymbol())) {
                    nfaStack.push(atomicNFA(ast->getSymbol()));
                } else {
                    nfaStack.push(buildOperatorNFA(ast));
                }
            }
        }
        bool loud;
    public:
        NFACompiler(bool trace = false) {
            l = 0;
            loud = trace;
        }
        NFA compile(string pattern) {
            REParser parser;
            string re = "(" + pattern + ")";
            auto ast = parser.parse(re);
            if (loud)
                traverse(ast, 1);
            gen_nfa(ast);
            return nfaStack.pop();
        }
};

#endif