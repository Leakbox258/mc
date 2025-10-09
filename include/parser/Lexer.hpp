#ifndef PARSER_LEXER
#define PARSER_LEXER

#include "utils/ADT/StringRef.hpp"
#include <string>

namespace parser {
using StringRef = utils::ADT::StringRef;

enum class TokenType {
    UNKNOWN,
    END_OF_FILE,
    NEWLINE,

    IDENTIFIER,  // e.g., my_label (when used as an operand)
    INTEGER,     // e.g., 123, -42
    HEX_INTEGER, // e.g., 0xdeadbeef

    INSTRUCTION, // e.g., addi, lw, sd
    REGISTER,    // e.g., a0, sp, x10

    DIRECTIVE,        // e.g., .global, .section
    LABEL_DEFINITION, // e.g., loop:, main:

    COMMA,  // ,
    LPAREN, // (
    RPAREN, // )
    COLON,  // :

    COMMENT,       // # ...
    STRING_LITERAL // "..."
};

std::string to_string(TokenType type);

struct Token {
    TokenType type;
    StringRef lexeme;
    size_t line;
    size_t col;

    void print() const;
};

class Lexer {
  public:
    Lexer(StringRef source);

    Token nextToken();

  private:
    StringRef m_source;
    size_t m_cursor = 0;
    size_t m_line = 1;
    size_t m_col = 1;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;

    void skipWhitespaceAndComments();
    Token makeToken(TokenType type) const;
    Token makeToken(TokenType type, StringRef lexeme) const;
    Token errorToken(const char* message) const;

    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
};
} // namespace parser
#endif