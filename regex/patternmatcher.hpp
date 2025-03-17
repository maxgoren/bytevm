#ifndef patternmatcher_hpp
#define patternmatcher_hpp
#include <iostream>
#include "re_compiler.hpp"
#include "nfa.hpp"
using namespace std;

void printEdge(Transition& t) {
    if (t.edge->isEpsilon()) {
        cout<<'\t'<<t.from<<" - ["<<t.edge->getLabel().charachters<<"] ->"<<t.to<<endl;
    } else {
        cout<<'\t'<<t.from<<" - ("<<t.edge->getLabel().charachters<<") ->"<<t.to<<endl;
    }
}

class RegExPatternMatcher {
    private:
        NFA nfa;
        // Gathers a list of states reachable from those in 
        // currStates which have transition that consume ch
        unordered_set<State> move(unordered_set<State> currStates, char ch) {
            unordered_set<State> nextStates;
            if (loud) cout<<ch<<": "<<endl;
            for (State s : currStates) {
                for (Transition t : nfa.getTransitions(s)) {
                    if (t.edge->matches(ch) || t.edge->matches('.')) {
                        if (t.edge->isEpsilon() == false && nextStates.find(t.to) == nextStates.end()) {
                            if (loud) printEdge(t);
                            nextStates.insert(t.to);
                        } 
                    }
                }
            }
            return nextStates;
        }
        //An interesting adaptation of Depth First Search.
        //Gathers a list of states reachable from those in 
        //currStates by using _only_ epsilon transitions.
        unordered_set<State> e_closure(unordered_set<State> currStates) {
            unordered_set<State> nextStates = currStates;
            IndexedStack<State> sf;
            for (State s : currStates)
                sf.push(s);
            while (!sf.empty()) {
                State s = sf.pop();
                for (Transition t : nfa.getTransitions(s)) {
                    if (t.edge->isEpsilon()) {
                        if (nextStates.find(t.to) == nextStates.end()) {
                            if (loud) printEdge(t);
                            nextStates.insert(t.to);
                            sf.push(t.to);
                        }
                    }
                }
            }
            return nextStates;
        }
        bool loud;
    public:
        RegExPatternMatcher(NFA& fa, bool trace = false) : nfa(fa), loud(trace) {

        }
        void setNFA(NFA& fa) {
            nfa = fa;
        }
        bool match(string text) {
            unordered_set<State> curr, next;
            next.insert(nfa.getStart());
            curr = e_closure(next);
            for (int i = 0; i < text.length(); i++) {
                next = move(curr, text[i]);
                curr = e_closure(next);
            }
            return curr.find(nfa.getAccept()) != curr.end();
        }
};

bool matchre(string text, string pattern) {
    NFACompiler compiler;
    NFA nfa = compiler.compile(pattern);
    RegExPatternMatcher pm(nfa);
    return pm.match(text);
}

#endif