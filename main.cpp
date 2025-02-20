#include <iostream>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "stringbuffer.hpp"
#include "tokenstream.hpp"
#include "vm.hpp"
#include "codegenerator.hpp"
#include <unordered_map>
using namespace std;

void meh() {
    bool running = true;
    string input;
    StringBuffer sb;
    Lexer lexer;
    Parser parser;
    VM vm;
    CodeGen codegen(vm.getConstantPool());
    while (running) {
        cout<<"meh> ";
        getline(cin, input);
        if (input == "quit") {
            running = false;
        } else {
            sb.init(input);
            TokenStream ts = lexer.lex(sb);
            astnode* ast = parser.parse(ts);
            preorder(ast, 1);
            vm.init(codegen.compile(ast));
            vm.exec();
        }
    }
}

void heh(string filename, bool debug) {
    astnode* ast;
    StringBuffer sb;
    TokenStream ts;
    Lexer lexer;
    Parser parser;
    VM vm(debug);
    CodeGen codegen(vm.getConstantPool());
    sb.readFromFile(filename);
    ts = lexer.lex(sb);
    /*
    for (ts.start(); !ts.done(); ts.advance()) {
        printToken(ts.get());
    }*/
    ast = parser.parse(ts);
    preorder(ast, 1);
    vm.init(codegen.compile(ast));
    vm.exec();
}

int main(int argc, char* argv[]) {
    switch (argc) {
        case 2:
            heh(argv[1], false);
            break;
        case 3:
            heh(argv[2], true);
            break;
    default:
        meh();
        break;
    }
    return 0;
}