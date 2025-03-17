#ifndef nfa_hpp
#define nfa_hpp
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../stack.hpp"
#include "re_tokenizer.hpp"
using namespace std;

class Edge {
    public:
        virtual RegExToken getLabel() = 0;
        virtual bool matches(char c) = 0;
        virtual bool positionIs(int index) = 0;
        virtual bool isEpsilon() = 0;
        virtual ~Edge() { }
};

class CharEdge : public Edge {
    private:
        RegExToken label;
        bool checkInRange(char c) {
            char lo, hi;
            bool is_good = false;
            for (int i = 1; i < label.charachters.size()-1; i++) {
                if (i+1 < label.charachters.size() && label.charachters[i] == '-') {
                    lo = label.charachters[i-1];
                    hi = label.charachters[i+1];
                    if (hi < lo) {
                        char tmp = hi;
                        hi = lo;
                        lo = tmp;
                    }
                    if (c >= lo && c <= hi) {
                        is_good = true;
                        break;
                    }
                }
            }
            return is_good;
        }
    public:
        CharEdge(RegExToken c) {
            label = c;
        }
        ~CharEdge() {

        }
        bool isEpsilon() {
            return false;
        }
        bool positionIs(int i) {
            return true;
        }
        bool matches(char c) {
            if (label.symbol == RE_SPECIFIEDSET) {
                for (char m : label.charachters) {
                    if (c == m)
                        return true;
                }
                return false;
            } else if (label.symbol == RE_SPECIFIEDRANGE) {
                return checkInRange(c);
            }
            return label.charachters[0] == c;
        }
        RegExToken getLabel() {
            return label;
        }
};

class EpsilonEdge : public Edge {
    public:
        EpsilonEdge() { }
        ~EpsilonEdge() { }
        bool matches(char c) {
            return true;
        }
        bool positionIs(int i) {
            return true;
        }
        bool isEpsilon() {
            return true;
        }
        RegExToken getLabel() {
            return RegExToken(RE_NONE, "&");
        }
};


typedef int State;

struct Transition {
    State from;
    State to;
    Edge* edge;
    Transition(State s, State t, Edge* e) {
        from = s; to = t; edge = e;
    }
    Transition(const Transition& t) {
        from = t.from;
        to = t.to;
        if (t.edge->isEpsilon()) {
            edge = new EpsilonEdge();
        } else {
            edge = new CharEdge(t.edge->getLabel());
        }
    }
    ~Transition() {
        delete edge;
    }
    Transition& operator=(const Transition& t) {
        if (this != &t) {
            from = t.from;
            to = t.to;
            if (t.edge->isEpsilon()) {
                edge = new EpsilonEdge();
            } else {
                edge = new CharEdge(t.edge->getLabel());
            }
        }
        return *this;
    }
};

bool operator==(const Transition& s, const Transition& t) {
    return s.from == t.from && s.to == t.to && s.edge == t.edge;
}

bool operator!=(const Transition& s, const Transition& t) {
    return !(s == t);
}

namespace std {
    template <> struct hash<Transition> {
        std::size_t operator()(const Transition& t) const noexcept {
            string tmp = to_string(t.from);
            tmp += t.edge->getLabel().charachters;
            tmp += to_string(t.to);
            return std::hash<string>()(tmp);
        }
    };
}

class NFA {
    private:
        State start;
        State accept;
        unordered_map<State, unordered_set<Transition>> states;
    public:
        NFA() {
            start = 0;
            accept = 0;
        }
        NFA(const NFA& nfa) {
            start = nfa.start;
            accept = nfa.accept;
            for (auto m : nfa.states) {
                makeState(m.first);
                for (auto t : m.second) {
                    addTransition(t);
                }
            }
        }
        void makeState(State name) {
            if (states.find(name) == states.end()) {
                states.insert(make_pair(name, unordered_set<Transition>()));
            }
        }
        void setStart(State ss) {
            start = ss;
        }
        void setAccept(State as) {
            accept = as;
        }
        State getStart() {
            return start;
        }
        State getAccept() {
            return accept;
        }
        void addTransition(Transition t) {
            states[t.from].insert(t);
        }
        int size() {
            return states.size();
        }
        unordered_map<State, unordered_set<Transition>>& getStates() {
            return states;
        }
        unordered_set<Transition>& getTransitions(State state) {
            return states[state];
        }
        NFA& operator=(const NFA& nfa) {
            if (this != &nfa) {
                start = nfa.start;
                accept = nfa.accept;
                for (auto m : nfa.states) {
                    makeState(m.first);
                    for (auto t : m.second) {
                        addTransition(t);
                    }
                }
            }
            return *this;
        }
};

#endif