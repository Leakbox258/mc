#include "parser/Parser.hpp"
#include "parser/Lexer.hpp"

using namespace parser;

void Parser::parse() {

    auto token = this->lexer.nextToken();
    while (token.type != TokenType::END_OF_FILE) {
    }
}