#include <iostream>
#include <vector>
#include "../lexer.hpp"
#include "../parser.hpp"
#include "../ast.hpp"
#include "../object.hpp"
#include "../stringbuffer.hpp"
#include "../tokenstream.hpp"
#include "codegenerator.hpp"
#include "vm.hpp"
#include <unordered_map>
using namespace std;



void meh() {
    bool running = true;
    string input;
    StringBuffer sb;
    Lexer lexer;
    Parser parser;
    VM vm;
    CodeGen codegen(vm.getConstantPool(), vm.getAllocator());
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


int main(int argc, char* argv[]) {
    meh();
    return 0;
}