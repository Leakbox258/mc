#include "parser/Lexer.hpp"
#include "mc/MCOpCode.hpp"
#include "mc/MCOperand.hpp"
#include <cctype>

using namespace parser;

std::string to_string(TokenType type) {
    switch (type) {
    case TokenType::UNKNOWN:
        return "UNKNOWN";
    case TokenType::END_OF_FILE:
        return "END_OF_FILE";
    case TokenType::NEWLINE:
        return "NEWLINE";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::INTEGER:
        return "INTEGER";
    case TokenType::HEX_INTEGER:
        return "HEX_INTEGER";
    case TokenType::INSTRUCTION:
        return "INSTRUCTION";
    case TokenType::REGISTER:
        return "REGISTER";
    case TokenType::DIRECTIVE:
        return "DIRECTIVE";
    case TokenType::LABEL_DEFINITION:
        return "LABEL_DEFINITION";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::LPAREN:
        return "LPAREN";
    case TokenType::RPAREN:
        return "RPAREN";
    case TokenType::COLON:
        return "COLON";
    case TokenType::COMMENT:
        return "COMMENT";
    case TokenType::STRING_LITERAL:
        return "STRING_LITERAL";
    default:
        return "TYPE_ERROR";
    }
}

void Token::print() const {
    std::cout << "Token(" << to_string(type) << ", lexeme: '" << lexeme << "', "
              << "line: " << line << ", col: " << col << ")\n";
}

Lexer::Lexer(StringRef source) : m_source(source) {}

bool Lexer::isAtEnd() const { return m_cursor >= m_source.size(); }

char Lexer::advance() {
    if (!isAtEnd()) {
        m_col++;
        return m_source[m_cursor++];
    }
    return '\0';
}

char Lexer::peek() const {
    if (isAtEnd())
        return '\0';
    return m_source[m_cursor];
}

char Lexer::peekNext() const {
    if (m_cursor + 1 >= m_source.size())
        return '\0';
    return m_source[m_cursor + 1];
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '#': // Comment goes to the end of the line
            while (peek() != '\n' && !isAtEnd()) {
                advance();
            }
            break;
        default:
            return;
        }
    }
}

Token Lexer::makeToken(TokenType type) const {
    // For single-character tokens
    return {type, m_source.slice(m_cursor - 1, 1), m_line, m_col - 1};
}

Token Lexer::makeToken(TokenType type, StringRef lexeme) const {
    return {type, lexeme, m_line, m_col - lexeme.size()};
}

Token Lexer::scanIdentifier() {
    size_t start = m_cursor - 1;
    while (isalnum(peek()) || peek() == '_' || peek() == '.') {
        advance();
    }
    StringRef lexeme = m_source.slice(start, m_cursor - start);

    // Check if it's a label definition
    if (peek() == ':') {
        advance(); // consume the ':'
        return makeToken(TokenType::LABEL_DEFINITION,
                         m_source.slice(start, m_cursor - start));
    }

    if (MnemonicContain(lexeme.c_str())) {
        return makeToken(TokenType::INSTRUCTION, lexeme);
    }
    if (mc::Registers.find(lexeme.str())) {
        return makeToken(TokenType::REGISTER, lexeme);
    }

    return makeToken(TokenType::IDENTIFIER, lexeme);
}

Token Lexer::scanNumber() {
    size_t start = m_cursor - 1;
    // Check for hexadecimal
    if (m_source[start] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume 'x'
        while (isxdigit(peek())) {
            advance();
        }
        return makeToken(TokenType::HEX_INTEGER,
                         m_source.slice(start, m_cursor - start));
    }

    // Handle negative numbers at the start
    if (m_source[start] == '-') {
        if (!isdigit(peek())) {
            return makeToken(TokenType::UNKNOWN, m_source.slice(start, 1));
        }
    }

    // Decimal
    while (isdigit(peek())) {
        advance();
    }
    return makeToken(TokenType::INTEGER,
                     m_source.slice(start, m_cursor - start));
}

Token Lexer::scanString() {
    size_t start = m_cursor; // Start after the opening quote
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') { // Unterminated string
            m_line++;
            m_col = 1;
        }
        advance();
    }

    if (isAtEnd()) {
        return makeToken(TokenType::UNKNOWN, "Unterminated string");
    }

    advance(); // Consume the closing quote
    StringRef lexeme = m_source.slice(start, m_cursor - start - 1);
    return makeToken(TokenType::STRING_LITERAL, lexeme);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return {TokenType::END_OF_FILE, "", m_line, m_col};
    }

    char c = advance();

    if (isalpha(c) || c == '_' || c == '.') {
        if (c == '.') { // Must be a directive
            size_t start = m_cursor - 1;
            while (isalnum(peek()) || peek() == '_') {
                advance();
            }
            return makeToken(TokenType::DIRECTIVE,
                             m_source.slice(start, m_cursor - start));
        }
        return scanIdentifier();
    }

    if (isdigit(c) || (c == '-' && isdigit(peek()))) {
        return scanNumber();
    }

    switch (c) {
    case '\n': {
        Token token = makeToken(TokenType::NEWLINE);
        m_line++;
        m_col = 1;
        return token;
    }
    case ',':
        return makeToken(TokenType::COMMA);
    case '(':
        return makeToken(TokenType::LPAREN);
    case ')':
        return makeToken(TokenType::RPAREN);
    case '"':
        return scanString();
    case ':':
        return makeToken(TokenType::COLON); // e.g. .section .text :
    }

    return makeToken(TokenType::UNKNOWN, m_source.slice(m_cursor - 1, 1));
}