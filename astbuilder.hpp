#ifndef astbuilder_hpp
#define astbuilder_hpp
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "stringbuffer.hpp"
#include "resolve.hpp"
class ASTBuilder {
    private:
        StringBuffer sb;
        Lexer lexer;
        Parser parser;
        ScopeLevelResolver resolver;
    public:
        ASTBuilder() {

        }
        astnode* build(string str) {
            sb.init(str);
            TokenStream ts = lexer.lex(sb);
            astnode* ast = parser.parse(ts);
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