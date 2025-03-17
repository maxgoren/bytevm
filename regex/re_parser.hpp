#ifndef re_parser_hpp
#define re_parser_hpp
#include <iostream>
#include "re_tokenizer.hpp"
#include "../stack.hpp"
using namespace std;

class RegularExpression {
    public:
        virtual RegExToken getSymbol() = 0;
        virtual RegularExpression* getLeft() = 0;
        virtual RegularExpression* getRight() = 0;
}; 

class ExpressionLiteral : public RegularExpression {
    private:
        RegExToken symbol;
    public:
        ExpressionLiteral(RegExToken sym) { symbol = sym; }
        RegularExpression* getLeft() {
            return nullptr;
        }
        RegularExpression* getRight() {
            return nullptr;
        }
        RegExToken getSymbol() {
            return symbol;
        }
};

class ExpressionOperator : public RegularExpression {
    private:
        RegularExpression* left;
        RegularExpression* right;
        RegExToken symbol;
    public:
        ExpressionOperator(RegExToken c, RegularExpression* ll, RegularExpression* rr) {
            symbol = c;
            left = ll;
            right = rr;
        }
        RegExToken getSymbol() {
            return symbol;
        }
        RegularExpression* getLeft() {
            return left;
        }
        RegularExpression* getRight() {
            return right;
        }
};

void traverse(RegularExpression* h, int d) {
    if (h != nullptr) {
        traverse(h->getLeft(), d+1);
        for (int i = 0; i < d; i++) cout<<"  ";
        cout<<h->getSymbol().charachters<<endl;
        traverse(h->getRight(), d+1);
    }
}

bool isOp(char c) {
    switch (c) {
        case '|': case '@': case '?': case '+': case '*':
            return true;
        default:
            break;
    }
    return false;
}

bool isOp(RegExToken c) {
    switch (c.symbol) {
        case RE_STAR:
        case RE_PLUS:
        case RE_QUESTION: 
        case RE_QUANTIFIER:
        case RE_CONCAT:
        case RE_OR: return true;
        default:
            break;
    }
    return false;
}

class REParser {
    private:
        RegularExpression* makeTree(vector<RegExToken> postfix) {
            IndexedStack<RegularExpression*> sf;
            for (RegExToken c : postfix) {
                if (!isOp(c)) {
                    sf.push(new ExpressionLiteral(c));
                } else {
                    auto right = sf.empty() ? nullptr:sf.pop();
                    auto left = sf.empty() ? nullptr:sf.pop();
                    sf.push(new ExpressionOperator(c, left, right));
                }
            }
            return sf.pop();
        }
        int precedence(RegExToken c) {
            switch (c.symbol) {
                case RE_STAR:
                case RE_PLUS:
                case RE_QUANTIFIER:
                case RE_QUESTION: return 50;
                case RE_CONCAT: return 30;
                case RE_OR: return 20;
                default:
                    break;
            }
            return 10;
        }
        bool leftAssociative(RegExToken c) {
            switch (c.symbol) {
                case RE_STAR:
                case RE_PLUS:
                case RE_QUANTIFIER:
                case RE_QUESTION: 
                case RE_CONCAT:
                case RE_OR: return true;
                default:
                    break;
            }
            return false;
        }
        string addConcatOp(string str) {
            string fixed;
            bool inset = false;
            for (int i = 0; i < str.length(); i++) {
                fixed.push_back(str[i]);
                if (str[i] == '(' || str[i] == '|')
                    continue;
                if (str[i] == '[' || str[i] == '{') {
                    inset = true;
                    continue;
                }
                if (str[i] == ']' || str[i] == '}') {
                    inset = false;
                }
                if (i+1 < str.length() && inset == false) {
                    char p = str[i+1];
                    if (p == '|' || p == '*' || p == '+' || p == ')' || p == '?' || p == ']' || p == '{' || p == '}')
                        continue;
                    fixed.push_back('@');
                }
            }
            //cout<<fixed<<endl;
            return fixed;
        }
        vector<RegExToken> in2post(vector<RegExToken> str) {
            IndexedStack<RegExToken> ops;
            vector<RegExToken> postfix;
            for (int i = 0; i < str.size(); i++) {
                if (str[i].symbol == RE_LPAREN) {
                    ops.push(str[i]);
                } else if (isOp(str[i])) {
                        if (precedence(str[i]) < precedence(ops.top()) || (precedence(str[i]) == precedence(ops.top()) && leftAssociative(str[i]))) {
                            RegExToken c = ops.pop();
                            postfix.push_back(c);
                            ops.push(str[i]);
                        } else {
                            ops.push(str[i]);
                        }
                } else if (str[i].symbol == RE_RPAREN) {
                    while (!ops.empty()) {
                        RegExToken c = ops.pop();
                        if (c.symbol == RE_LPAREN)
                            break;
                        else postfix.push_back(c);
                    }
                } else {
                    postfix.push_back(str[i]);
                }
            }
            while (!ops.empty()) {
                RegExToken c = ops.pop();
                if (c.symbol != RE_LPAREN)
                    postfix.push_back(c);
            }
            return postfix;
        }
    public:
        REParser() {

        }
        RegularExpression* parse(string regexp) {
            Tokenizer tz;
            regexp = addConcatOp(regexp);
            auto tokens = tz.tokenize(regexp);
            int i = 0;
            vector<RegExToken> postfix = in2post(tokens);
            return makeTree(postfix);
        }
};

#endif