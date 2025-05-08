#ifndef astbuilder_hpp
#define astbuilder_hpp
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "stringbuffer.hpp"
#include "resolve.hpp"
class ASTBuilder {
    private:
        bool loud;
        StringBuffer sb;
        Lexer lexer;
        Parser parser;
        ScopeLevelResolver resolver;
    public:
        ASTBuilder(bool debug = false) {
            loud = debug;
        }
        astnode* build(string str) {
            sb.init(str);
            TokenStream ts = lexer.lex(sb);
            astnode* ast = parser.parse(ts);
            if (loud) {
                preorder(ast, 0);
            }
            return resolver.resolveScope(ast);
        }
        astnode* buildFromFile(string filename) {
            sb.readFromFile(filename);
            TokenStream ts = lexer.lex(sb);
            astnode* ast = parser.parse(ts);
            return resolver.resolveScope(ast);
        }
};

#endif