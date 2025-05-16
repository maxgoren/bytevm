#include <iostream>
#include "astbuilder.hpp"
#include "twvm.hpp"
using namespace std;

void runScript(string filename) {
    ASTBuilder astbuilder;
    TWVM vm(false);
    vm.exec(astbuilder.buildFromFile(filename));
    if (vm.context().existsInScope("main")) {
        vm.exec(astbuilder.build("main();"));
    }
}


void repl() {
    cout<<"[OwlscriptSV 0.6b]"<<endl;
    bool running = true;
    string input;
    ASTBuilder astbuilder;
    TWVM vm(true);
    int i = 1;
    while (running) {
        cout<<"OwlScriptSV("<<i++<<")> ";
        getline(cin, input);
        if (input == "quit") {
            running = false;
        } else {
            astnode* ast = astbuilder.build(input);
            preorder(ast, 1);
            vm.exec(ast);
            cleanUpTree(ast); 
        }
    }
    cout<<"[hoot!]"<<endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        repl();
    } else {
        runScript(argv[1]);
    }
    return 0;
}